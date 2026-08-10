; Arborcore buffer engine tests
; exit 0 pass, 1 fail

%define SYS_EXIT      60
%define ERR_EINVAL   -22
%define ERR_ENOSPC   -28
%define ERR_EOVERFLOW -75

extern buffer_init
extern buffer_reset
extern buffer_length
extern buffer_remaining
extern buffer_append
extern buffer_append_byte
extern buffer_consume

global _start

section .rodata
abc: db "abc"
xyz: db "xyz"

section .bss
align 16
buffer_struct: resb 24
storage:       resb 10
after_guard:   resb 1

section .text
_start:
    ; Initialize 10-byte storage.
    mov byte [rel after_guard], 0xA5
    lea rdi, [rel buffer_struct]
    lea rsi, [rel storage]
    mov edx, 10
    call buffer_init
    test rax, rax
    jnz .fail
    test rdx, rdx
    jnz .fail

    lea rdi, [rel buffer_struct]
    call buffer_length
    test rax, rax
    jnz .fail

    lea rdi, [rel buffer_struct]
    call buffer_remaining
    cmp rax, 10
    jne .fail


    ; A nonzero storage range that wraps the address space is invalid
    ; at initialization time, before any memory access.
    lea rdi, [rel buffer_struct]
    mov rsi, -8
    mov edx, 16
    call buffer_init
    cmp rax, ERR_EOVERFLOW
    jne .fail
    test rdx, rdx
    jnz .fail

    ; Restore the valid buffer after the synthetic range test.
    lea rdi, [rel buffer_struct]
    lea rsi, [rel storage]
    mov edx, 10
    call buffer_init
    test rax, rax
    jnz .fail

    ; Append "abc".
    lea rdi, [rel buffer_struct]
    lea rsi, [rel abc]
    mov edx, 3
    call buffer_append
    test rax, rax
    jnz .fail
    cmp rdx, 3
    jne .fail
    cmp byte [rel storage], 'a'
    jne .fail
    cmp byte [rel storage + 2], 'c'
    jne .fail

    ; Append a byte.
    lea rdi, [rel buffer_struct]
    mov esi, '!'
    call buffer_append_byte
    test rax, rax
    jnz .fail
    cmp rdx, 4
    jne .fail
    cmp byte [rel storage + 3], '!'
    jne .fail

    ; Zero-length append is NULL-safe and preserves length.
    lea rdi, [rel buffer_struct]
    xor esi, esi
    xor edx, edx
    call buffer_append
    test rax, rax
    jnz .fail
    cmp rdx, 4
    jne .fail

    ; Consume first two bytes: "c!" remains.
    lea rdi, [rel buffer_struct]
    mov esi, 2
    call buffer_consume
    test rax, rax
    jnz .fail
    cmp rdx, 2
    jne .fail
    cmp byte [rel storage], 'c'
    jne .fail
    cmp byte [rel storage + 1], '!'
    jne .fail

    ; Append 3 more bytes => "c!xyz".
    lea rdi, [rel buffer_struct]
    lea rsi, [rel xyz]
    mov edx, 3
    call buffer_append
    test rax, rax
    jnz .fail
    cmp rdx, 5
    jne .fail

    ; Request beyond remaining capacity fails before writing.
    mov byte [rel storage + 5], 0x5A
    lea rdi, [rel buffer_struct]
    lea rsi, [rel xyz]
    mov edx, 6
    call buffer_append
    cmp rax, ERR_ENOSPC
    jne .fail
    test rdx, rdx
    jnz .fail
    cmp byte [rel storage + 5], 0x5A
    jne .fail
    cmp byte [rel after_guard], 0xA5
    jne .fail

    ; Consume beyond length is invalid.
    lea rdi, [rel buffer_struct]
    mov esi, 6
    call buffer_consume
    cmp rax, ERR_EINVAL
    jne .fail

    ; Reset.
    lea rdi, [rel buffer_struct]
    call buffer_reset
    test rax, rax
    jnz .fail
    lea rdi, [rel buffer_struct]
    call buffer_length
    test rax, rax
    jnz .fail

    xor edi, edi
    jmp .exit
.fail:
    mov edi, 1
.exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
