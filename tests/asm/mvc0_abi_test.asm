; Arborcore MVC0 real NASM ABI qualification helpers.
; Linux x86-64 System V AMD64.

default rel

%define ERR_EINVAL -22
%define MVC_CONTINUE 0
%define MVC_RESPOND 1
%define KEEP_ALIVE_FLAG 1

; mvc0_asm_callback_context layout:
; +0  controller_calls
; +8  presenter_calls
; +16 before_calls
; +24 after_calls
; +32 stack_errors
; +40 body_ptr
; +48 body_len
; +56 middleware_action
; +64 response_status
; +72 keep_alive

section .text

global mvc0_asm_controller:function
global mvc0_asm_presenter:function
global mvc0_asm_before:function
global mvc0_asm_after:function
global mvc0_asm_call_request_validate:function
global mvc0_asm_transport_dispatch:function

extern arbor_mvc_request_validate
extern http_response_serialize

%macro CHECK_STACK 1
    mov rax, rsp
    and eax, 15
    cmp eax, 8
    je %%ok
    inc qword [%1 + 32]
%%ok:
%endmacro

; int64_t controller(request*, context*, result_out*)
mvc0_asm_controller:
    test rdi, rdi
    jz mvc0_controller_invalid
    test rsi, rsi
    jz mvc0_controller_invalid
    test rdx, rdx
    jz mvc0_controller_invalid
    CHECK_STACK rsi
    inc qword [rsi + 0]
    mov dword [rdx + 0], 77
    mov dword [rdx + 4], 0
    mov rax, [rsi + 40]
    mov [rdx + 8], rax
    mov rax, [rsi + 48]
    mov [rdx + 16], rax
    xor eax, eax
    ret
mvc0_controller_invalid:
    mov rax, ERR_EINVAL
    ret

; int64_t presenter(request*, context*, result*, response_out*)
mvc0_asm_presenter:
    test rdi, rdi
    jz mvc0_presenter_invalid
    test rsi, rsi
    jz mvc0_presenter_invalid
    test rdx, rdx
    jz mvc0_presenter_invalid
    test rcx, rcx
    jz mvc0_presenter_invalid
    CHECK_STACK rsi
    inc qword [rsi + 8]
    mov rax, [rsi + 64]
    mov [rcx + 0], rax
    mov rax, [rdx + 8]
    mov [rcx + 8], rax
    mov rax, [rdx + 16]
    mov [rcx + 16], rax
    xor eax, eax
    cmp qword [rsi + 72], 0
    je mvc0_presenter_flags_done
    mov eax, KEEP_ALIVE_FLAG
mvc0_presenter_flags_done:
    mov [rcx + 24], rax
    xor eax, eax
    ret
mvc0_presenter_invalid:
    mov rax, ERR_EINVAL
    ret

; int64_t before(request*, context*, before_result_out*)
mvc0_asm_before:
    test rdi, rdi
    jz mvc0_before_invalid
    test rsi, rsi
    jz mvc0_before_invalid
    test rdx, rdx
    jz mvc0_before_invalid
    CHECK_STACK rsi
    inc qword [rsi + 16]
    mov rax, [rsi + 56]
    cmp rax, MVC_CONTINUE
    je mvc0_before_continue
    cmp rax, MVC_RESPOND
    jne mvc0_before_invalid
    mov dword [rdx + 0], MVC_RESPOND
    mov dword [rdx + 4], 0
    mov rax, [rsi + 64]
    mov [rdx + 8], rax
    mov rax, [rsi + 40]
    mov [rdx + 16], rax
    mov rax, [rsi + 48]
    mov [rdx + 24], rax
    xor eax, eax
    cmp qword [rsi + 72], 0
    je mvc0_before_flags_done
    mov eax, KEEP_ALIVE_FLAG
mvc0_before_flags_done:
    mov [rdx + 32], rax
    xor eax, eax
    ret
mvc0_before_continue:
    mov dword [rdx + 0], MVC_CONTINUE
    mov dword [rdx + 4], 0
    xor eax, eax
    mov [rdx + 8], rax
    mov [rdx + 16], rax
    mov [rdx + 24], rax
    mov [rdx + 32], rax
    ret
mvc0_before_invalid:
    mov rax, ERR_EINVAL
    ret

; int64_t after(request*, context*, current*, response_out*)
mvc0_asm_after:
    test rdi, rdi
    jz mvc0_after_invalid
    test rsi, rsi
    jz mvc0_after_invalid
    test rdx, rdx
    jz mvc0_after_invalid
    test rcx, rcx
    jz mvc0_after_invalid
    CHECK_STACK rsi
    inc qword [rsi + 24]
    mov rax, [rdx + 0]
    mov [rcx + 0], rax
    mov rax, [rdx + 8]
    mov [rcx + 8], rax
    mov rax, [rdx + 16]
    mov [rcx + 16], rax
    mov rax, [rdx + 24]
    mov [rcx + 24], rax
    xor eax, eax
    ret
mvc0_after_invalid:
    mov rax, ERR_EINVAL
    ret

; arbor_status mvc0_asm_call_request_validate(const arbor_mvc_request *request)
; Preserve the C aggregate return in RAX:RDX.
mvc0_asm_call_request_validate:
    sub rsp, 8
    call arbor_mvc_request_validate
    add rsp, 8
    ret

; Separate transport-dispatch context layout:
; +0 calls
; +8 stack_errors
; +16 body_ptr
; +24 body_len
; +32 status
; +40 keep_alive
;
; int64_t transport_dispatch(request*, output*, arena*, context*, keep_alive_out*)
mvc0_asm_transport_dispatch:
    test rdi, rdi
    jz mvc0_transport_dispatch_invalid
    test rsi, rsi
    jz mvc0_transport_dispatch_invalid
    test rdx, rdx
    jz mvc0_transport_dispatch_invalid
    test rcx, rcx
    jz mvc0_transport_dispatch_invalid
    test r8, r8
    jz mvc0_transport_dispatch_invalid

    mov rax, rsp
    and eax, 15
    cmp eax, 8
    je mvc0_transport_dispatch_aligned
    inc qword [rcx + 8]
mvc0_transport_dispatch_aligned:
    inc qword [rcx + 0]

    push r12
    push r13
    sub rsp, 8
    mov r12, rcx
    mov r13, r8

    mov rdi, rsi
    mov rsi, [r12 + 32]
    mov rdx, [r12 + 16]
    mov rcx, [r12 + 24]
    mov r8, [r12 + 40]
    call http_response_serialize
    test rax, rax
    js mvc0_transport_dispatch_return
    mov rcx, [r12 + 40]
    mov [r13], rcx
mvc0_transport_dispatch_return:
    add rsp, 8
    pop r13
    pop r12
    ret
mvc0_transport_dispatch_invalid:
    mov rax, ERR_EINVAL
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
