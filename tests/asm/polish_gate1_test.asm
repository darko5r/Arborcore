; Arborcore Polish Gate #1 integration test
;
; Integrates arena allocation + buffer composition + numeric formatting
; + hexadecimal encoding + byte-span equality.
; exit 0 pass, 1 fail

%define SYS_EXIT 60

extern arena_init
extern arena_alloc
extern arena_alloc_aligned
extern arena_mark
extern arena_reset
extern buffer_init
extern buffer_append
extern u64_format_decimal
extern bytes_encode_hex
extern bytes_equal

global _start

section .rodata
prefix: db "id="
prefix_len equ $ - prefix
middle: db "&hex="
middle_len equ $ - middle
binary: db 0xAB, 0xCD
binary_len equ $ - binary
expected: db "id=12345&hex=abcd"
expected_len equ $ - expected

section .bss
align 16
arena_struct: resb 24
buffer_struct: resb 24
arena_storage: resb 256

section .text
_start:
    lea rdi, [rel arena_struct]
    lea rsi, [rel arena_storage]
    mov edx, 256
    call arena_init
    test rax, rax
    jnz .fail

    ; Main output span.
    lea rdi, [rel arena_struct]
    mov esi, 96
    mov edx, 16
    call arena_alloc_aligned
    test rax, rax
    jnz .fail
    mov r12, rdx

    lea rdi, [rel buffer_struct]
    mov rsi, r12
    mov edx, 96
    call buffer_init
    test rax, rax
    jnz .fail

    lea rdi, [rel buffer_struct]
    lea rsi, [rel prefix]
    mov edx, prefix_len
    call buffer_append
    test rax, rax
    jnz .fail

    ; Temporary numeric span from same arena.
    lea rdi, [rel arena_struct]
    mov esi, 20
    call arena_alloc
    test rax, rax
    jnz .fail
    mov r13, rdx

    mov edi, 12345
    mov rsi, r13
    mov edx, 20
    call u64_format_decimal
    test rax, rax
    jnz .fail
    mov r14, rdx

    lea rdi, [rel buffer_struct]
    mov rsi, r13
    mov rdx, r14
    call buffer_append
    test rax, rax
    jnz .fail

    lea rdi, [rel buffer_struct]
    lea rsi, [rel middle]
    mov edx, middle_len
    call buffer_append
    test rax, rax
    jnz .fail

    ; Temporary encoded binary span.
    lea rdi, [rel arena_struct]
    mov esi, 4
    call arena_alloc
    test rax, rax
    jnz .fail
    mov r13, rdx

    lea rdi, [rel binary]
    mov esi, binary_len
    mov rdx, r13
    mov ecx, 4
    call bytes_encode_hex
    test rax, rax
    jnz .fail
    cmp rdx, 4
    jne .fail

    lea rdi, [rel buffer_struct]
    mov rsi, r13
    mov rdx, 4
    call buffer_append
    test rax, rax
    jnz .fail
    cmp rdx, expected_len
    jne .fail

    mov rdi, r12
    mov esi, expected_len
    lea rdx, [rel expected]
    mov ecx, expected_len
    call bytes_equal
    cmp rax, 1
    jne .fail

    ; Gate ends with a full arena reset.
    lea rdi, [rel arena_struct]
    call arena_reset
    test rax, rax
    jnz .fail
    lea rdi, [rel arena_struct]
    call arena_mark
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
