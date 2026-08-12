; Minimal ABI v1 consumer used for static/shared library readiness.
; Exit 0 pass, nonzero fail.

%define SYS_EXIT 60

extern memory_secure_clear
extern memory_equal_constant_time
extern arena_init
extern arena_alloc_aligned
extern buffer_init
extern buffer_append
extern buffer_length

global _start

section .bss
alignb 32
left:         resb 32
right:        resb 32
arena_obj:    resb 24
arena_bytes:  resb 64
buffer_obj:   resb 24
buffer_bytes: resb 64

section .rodata
payload: db "ABI-v1"
payload_len equ $ - payload

section .text
_start:
    ; Security equality is callable through the public ABI.
    lea rdi, [rel left]
    lea rsi, [rel right]
    mov edx, 32
    call memory_equal_constant_time
    cmp eax, 1
    jne .fail

    lea rdi, [rel left]
    mov esi, 32
    call memory_secure_clear

    ; Freeze zero-size arena semantics: alignment may advance the frontier.
    lea rdi, [rel arena_obj]
    lea rsi, [rel arena_bytes + 1]
    mov edx, 63
    call arena_init
    test rax, rax
    jnz .fail
    lea rdi, [rel arena_obj]
    xor esi, esi
    mov edx, 16
    call arena_alloc_aligned
    test rax, rax
    jnz .fail
    test rdx, 15
    jnz .fail

    ; Basic buffer capability through the stable ABI.
    lea rdi, [rel buffer_obj]
    lea rsi, [rel buffer_bytes]
    mov edx, 64
    call buffer_init
    test rax, rax
    jnz .fail
    lea rdi, [rel buffer_obj]
    lea rsi, [rel payload]
    mov edx, payload_len
    call buffer_append
    test rax, rax
    jnz .fail
    lea rdi, [rel buffer_obj]
    call buffer_length
    cmp rax, payload_len
    jne .fail

    xor edi, edi
    jmp .exit
.fail:
    mov edi, 1
.exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
