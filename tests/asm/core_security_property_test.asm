; Arborcore Assembly Security S1/S2 property qualification
; Exit 0 pass, nonzero fail.

%define SYS_EXIT 60
%define MAX_LEN 256
%define GUARD 0xA5

extern memory_secure_clear
extern memory_equal_constant_time

global _start

section .bss
alignb 64
left_guard:   resb 1
left_buf:     resb MAX_LEN
left_tail:    resb 1
right_guard:  resb 1
right_buf:    resb MAX_LEN
right_tail:   resb 1

section .text
_start:
    call test_secure_clear
    test eax, eax
    jnz .fail_clear

    call test_constant_time_equality
    test eax, eax
    jnz .fail_equal

    xor edi, edi
    jmp .exit
.fail_clear:
    mov edi, 1
    jmp .exit
.fail_equal:
    mov edi, 2
.exit:
    mov eax, SYS_EXIT
    syscall

; Exhaust lengths 0..256. Guards on both sides must remain untouched.
test_secure_clear:
    sub rsp, 8
    xor r12d, r12d
.len_loop:
    cmp r12d, MAX_LEN + 1
    jae .pass

    mov byte [rel left_guard], GUARD
    mov byte [rel left_tail], GUARD
    lea r8, [rel left_buf]
    xor ecx, ecx
.fill:
    cmp ecx, MAX_LEN
    jae .invoke
    mov eax, ecx
    imul eax, eax, 37
    add eax, 11
    or al, 1
    mov [r8 + rcx], al
    inc ecx
    jmp .fill

.invoke:
    lea rdi, [rel left_buf]
    mov r13, rdi
    mov rsi, r12
    call memory_secure_clear
    cmp rax, r13
    jne .fail
    cmp byte [rel left_guard], GUARD
    jne .fail
    cmp byte [rel left_tail], GUARD
    jne .fail

    lea r8, [rel left_buf]
    xor ecx, ecx
.check_zero:
    cmp ecx, r12d
    jae .check_suffix
    cmp byte [r8 + rcx], 0
    jne .fail
    inc ecx
    jmp .check_zero

.check_suffix:
    cmp ecx, MAX_LEN
    jae .next_len
    ; Bytes outside the requested span keep the deterministic nonzero fill.
    mov eax, ecx
    imul eax, eax, 37
    add eax, 11
    or al, 1
    cmp [r8 + rcx], al
    jne .fail
    inc ecx
    jmp .check_suffix

.next_len:
    inc r12d
    jmp .len_loop
.pass:
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

; For every length 0..256, equal buffers must return 1.  For every nonzero
; length, move one mismatch through every byte position; every call must read
; the full requested domain observationally and return 0.
test_constant_time_equality:
    sub rsp, 8
    xor r12d, r12d
.length_loop:
    cmp r12d, MAX_LEN + 1
    jae .pass

    lea r8, [rel left_buf]
    lea r9, [rel right_buf]
    xor ecx, ecx
.init:
    cmp ecx, MAX_LEN
    jae .equal_call
    mov eax, ecx
    imul eax, eax, 29
    add eax, 7
    mov [r8 + rcx], al
    mov [r9 + rcx], al
    inc ecx
    jmp .init

.equal_call:
    lea rdi, [rel left_buf]
    lea rsi, [rel right_buf]
    mov rdx, r12
    call memory_equal_constant_time
    cmp eax, 1
    jne .fail
    test r12d, r12d
    jz .next_length

    xor r13d, r13d
.mismatch_loop:
    cmp r13d, r12d
    jae .next_length
    lea r8, [rel right_buf]
    xor byte [r8 + r13], 0x5A

    lea rdi, [rel left_buf]
    lea rsi, [rel right_buf]
    mov rdx, r12
    call memory_equal_constant_time
    test eax, eax
    jnz .fail

    lea r8, [rel right_buf]
    xor byte [r8 + r13], 0x5A
    inc r13d
    jmp .mismatch_loop

.next_length:
    inc r12d
    jmp .length_loop
.pass:
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
