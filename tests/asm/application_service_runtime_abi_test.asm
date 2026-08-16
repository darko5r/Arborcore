; Arborcore AF3 Application-service runtime ABI qualification helpers.
; Linux x86-64 System V AMD64 ABI.

default rel

section .text

global af3_asm_prepare:function
global af3_asm_rollback:function
global af3_asm_stop:function
global af3_asm_typed_method:function
global af3_asm_call_c_typed:function
global af3_asm_call_c_typed_preserve:function

%define ERR_EINVAL -22
%define ERR_EFAULT -14

; test_trace:
;   +0  count
;   +8  events[0]
;
; asm_lifecycle_context:
;   +0  trace*
;   +8  id
;   +16 prepare_calls
;   +24 rollback_calls
;   +32 stop_calls
;   +40 prepare_return
;   +48 stop_return
;   +56 stack_errors
;
; event encoding: prepare=100+id, rollback=200+id, stop=300+id

%macro CHECK_ENTRY_ALIGNMENT 2
    mov rax, rsp
    and eax, 15
    cmp eax, 8
    je %%ok
    test %1, %1
    jz %%ok
    inc qword [%1 + 56]
    mov %2, ERR_EINVAL
    jmp %%done
%%ok:
    xor %2, %2
%%done:
%endmacro

%macro APPEND_EVENT 2
    mov r8, [%1 + 0]
    test r8, r8
    jz %%done
    mov r9, [r8 + 0]
    cmp r9, 64
    jae %%done
    mov r10, [%1 + 8]
    add r10, %2
    mov [r8 + 8 + r9*8], r10
    inc qword [r8 + 0]
%%done:
%endmacro

; int64_t af3_asm_prepare(void *module_context, const void *prepare_context)
af3_asm_prepare:
    test rdi, rdi
    jz .prepare_invalid
    test rsi, rsi
    jz .prepare_invalid

    CHECK_ENTRY_ALIGNMENT rdi, r11
    test r11, r11
    jnz .prepare_return_r11

    inc qword [rdi + 16]
    APPEND_EVENT rdi, 100
    mov rax, [rdi + 40]
    ret

.prepare_return_r11:
    mov rax, r11
    ret
.prepare_invalid:
    mov rax, ERR_EINVAL
    ret

; void af3_asm_rollback(void *module_context)
af3_asm_rollback:
    test rdi, rdi
    jz .rollback_ret
    mov rax, rsp
    and eax, 15
    cmp eax, 8
    je .rollback_aligned
    inc qword [rdi + 56]
.rollback_aligned:
    inc qword [rdi + 24]
    APPEND_EVENT rdi, 200
.rollback_ret:
    ret

; int64_t af3_asm_stop(void *module_context)
af3_asm_stop:
    test rdi, rdi
    jz .stop_invalid

    CHECK_ENTRY_ALIGNMENT rdi, r11
    test r11, r11
    jnz .stop_return_r11

    inc qword [rdi + 32]
    APPEND_EVENT rdi, 300
    mov rax, [rdi + 48]
    ret

.stop_return_r11:
    mov rax, r11
    ret
.stop_invalid:
    mov rax, ERR_EINVAL
    ret

; asm_typed_provider_context:
;   +0  bias
;   +8  native_return
;   +16 calls
;   +24 stack_errors
;
; input/result both contain one uint64_t at offset 0.
;
; int64_t af3_asm_typed_method(void *provider_context,
;                              const void *input,
;                              void *output)
af3_asm_typed_method:
    test rdi, rdi
    jz .typed_invalid
    test rsi, rsi
    jz .typed_invalid
    test rdx, rdx
    jz .typed_invalid

    mov rax, rsp
    and eax, 15
    cmp eax, 8
    je .typed_aligned
    inc qword [rdi + 24]
    mov rax, ERR_EINVAL
    ret
.typed_aligned:
    inc qword [rdi + 16]
    mov rax, [rdi + 8]
    test rax, rax
    jnz .typed_ret

    mov r8, [rsi + 0]
    add r8, [rdi + 0]
    mov [rdx + 0], r8
    xor eax, eax
.typed_ret:
    ret
.typed_invalid:
    mov rax, ERR_EINVAL
    ret

; int64_t af3_asm_call_c_typed(fn, provider_context, input, output)
;   RDI=fn RSI=context RDX=input RCX=output
;   Calls fn(context,input,output) using RDI/RSI/RDX.
af3_asm_call_c_typed:
    test rdi, rdi
    jz .call_invalid
    mov r11, rdi
    mov rdi, rsi
    mov rsi, rdx
    mov rdx, rcx
    sub rsp, 8
    call r11
    add rsp, 8
    ret
.call_invalid:
    mov rax, ERR_EINVAL
    ret

; Same as above but additionally proves the called C function preserves all
; System V callee-saved registers. Returns -EFAULT if any sentinel is clobbered;
; otherwise returns the C function's RAX.
af3_asm_call_c_typed_preserve:
    test rdi, rdi
    jz .preserve_invalid

    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov r11, rdi
    mov r10, rsi
    mov r9, rdx
    mov r8, rcx

    mov rbx, 0x1122334455667788
    mov rbp, 0x2233445566778899
    mov r12, 0x33445566778899aa
    mov r13, 0x445566778899aabb
    mov r14, 0x5566778899aabbcc
    mov r15, 0x66778899aabbccdd

    mov rdi, r10
    mov rsi, r9
    mov rdx, r8

    ; Six pushes keep RSP%16 == 8 at this point; subtract 8 before CALL.
    sub rsp, 8
    call r11
    add rsp, 8
    mov r10, rax

    mov rax, 0x1122334455667788
    cmp rbx, rax
    jne .preserve_failed
    mov rax, 0x2233445566778899
    cmp rbp, rax
    jne .preserve_failed
    mov rax, 0x33445566778899aa
    cmp r12, rax
    jne .preserve_failed
    mov rax, 0x445566778899aabb
    cmp r13, rax
    jne .preserve_failed
    mov rax, 0x5566778899aabbcc
    cmp r14, rax
    jne .preserve_failed
    mov rax, 0x66778899aabbccdd
    cmp r15, rax
    jne .preserve_failed

    mov rax, r10
    jmp .preserve_restore

.preserve_failed:
    mov rax, ERR_EFAULT

.preserve_restore:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

.preserve_invalid:
    mov rax, ERR_EINVAL
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
