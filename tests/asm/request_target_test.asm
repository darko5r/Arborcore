; Arborcore request-target test; exit 0 pass, 1 fail
%define SYS_EXIT 60

global _start
extern request_target_split

section .rodata
t1: db "/users/42?full=1&sort=name"
t1_len equ $ - t1
root: db "/"
asterisk: db "*"
bad_fragment: db "/x#frag"
bad_absolute: db "http://example.com/"

section .bss
align 16
out: resb 32

section .text
_start:
    lea rdi, [rel t1]
    mov esi, t1_len
    lea rdx, [rel out]
    call request_target_split
    test rax, rax
    jnz fail
    lea rax, [rel t1]
    cmp [rel out + 0], rax
    jne fail
    cmp qword [rel out + 8], 9     ; /users/42
    jne fail
    lea rax, [rel t1 + 10]
    cmp [rel out + 16], rax
    jne fail
    cmp qword [rel out + 24], t1_len - 10
    jne fail

    lea rdi, [rel root]
    mov esi, 1
    lea rdx, [rel out]
    call request_target_split
    test rax, rax
    jnz fail
    cmp qword [rel out + 8], 1
    jne fail
    cmp qword [rel out + 24], 0
    jne fail

    lea rdi, [rel asterisk]
    mov esi, 1
    lea rdx, [rel out]
    call request_target_split
    test rax, rax
    jnz fail
    cmp qword [rel out + 8], 1
    jne fail

    lea rdi, [rel bad_fragment]
    mov esi, 7
    lea rdx, [rel out]
    call request_target_split
    cmp rax, -22
    jne fail

    lea rdi, [rel bad_absolute]
    mov esi, 19
    lea rdx, [rel out]
    call request_target_split
    cmp rax, -22
    jne fail

    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
