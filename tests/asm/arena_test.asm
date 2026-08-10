; Arborcore VM + arena tests
; exit 0 pass, 1 fail

%define SYS_EXIT       60
%define ERR_EINVAL    -22
%define ERR_ENOSPC    -28
%define ERR_EOVERFLOW -75

extern vm_map_rw
extern vm_unmap
extern arena_init
extern arena_alloc
extern arena_alloc_aligned
extern arena_mark
extern arena_rewind
extern arena_reset

global _start

section .bss
align 16
arena_struct: resb 24
unaligned_storage: resb 96

section .text
_start:
    ; Map one page.
    mov edi, 4096
    call vm_map_rw
    test rax, rax
    js .fail
    mov r12, rax

    ; Mapping is writable.
    mov byte [r12], 0xA5
    cmp byte [r12], 0xA5
    jne .unmap_fail

    ; Initialize arena over the mapping.
    lea rdi, [rel arena_struct]
    mov rsi, r12
    mov edx, 4096
    call arena_init
    test rax, rax
    jnz .unmap_fail


    ; Synthetic near-wrap arena storage must be rejected at init time.
    lea rdi, [rel arena_struct]
    mov rsi, -8
    mov edx, 16
    call arena_init
    cmp rax, ERR_EOVERFLOW
    jne .unmap_fail
    test rdx, rdx
    jnz .unmap_fail

    ; Alignment is defined on the absolute pointer, not just the offset.
    ; Deliberately initialize an arena at an unaligned base address.
    lea rdi, [rel arena_struct]
    lea rsi, [rel unaligned_storage + 1]
    mov edx, 95
    call arena_init
    test rax, rax
    jnz .unmap_fail

    lea rdi, [rel arena_struct]
    mov esi, 8
    mov edx, 16
    call arena_alloc_aligned
    test rax, rax
    jnz .unmap_fail
    test rdx, 15
    jnz .unmap_fail
    lea rcx, [rel unaligned_storage + 1]
    cmp rdx, rcx
    jb .unmap_fail
    lea rcx, [rel unaligned_storage + 96]
    cmp rdx, rcx
    jae .unmap_fail

    ; Restore the mmap-backed arena for the remaining tests.
    lea rdi, [rel arena_struct]
    mov rsi, r12
    mov edx, 4096
    call arena_init
    test rax, rax
    jnz .unmap_fail

    ; 7-byte allocation starts at base.
    lea rdi, [rel arena_struct]
    mov esi, 7
    call arena_alloc
    test rax, rax
    jnz .unmap_fail
    cmp rdx, r12
    jne .unmap_fail

    ; Mark == 7.
    lea rdi, [rel arena_struct]
    call arena_mark
    cmp rax, 7
    jne .unmap_fail
    mov r13, rax

    ; 16-byte aligned allocation.
    lea rdi, [rel arena_struct]
    mov esi, 32
    mov edx, 16
    call arena_alloc_aligned
    test rax, rax
    jnz .unmap_fail
    test rdx, 15
    jnz .unmap_fail
    mov r14, rdx

    ; The allocation is writable and inside mapping.
    mov byte [r14], 0x5A
    cmp byte [r14], 0x5A
    jne .unmap_fail

    ; Invalid non-power-of-two alignment.
    lea rdi, [rel arena_struct]
    mov esi, 1
    mov edx, 3
    call arena_alloc_aligned
    cmp rax, ERR_EINVAL
    jne .unmap_fail
    test rdx, rdx
    jnz .unmap_fail

    ; Rewind to mark.
    lea rdi, [rel arena_struct]
    mov rsi, r13
    call arena_rewind
    test rax, rax
    jnz .unmap_fail
    lea rdi, [rel arena_struct]
    call arena_mark
    cmp rax, 7
    jne .unmap_fail

    ; Huge allocation cannot fit.
    lea rdi, [rel arena_struct]
    mov rsi, -1
    call arena_alloc
    cmp rax, ERR_ENOSPC
    je .huge_ok
    cmp rax, ERR_EOVERFLOW
    jne .unmap_fail
.huge_ok:
    test rdx, rdx
    jnz .unmap_fail

    ; Reset and allocate from base again.
    lea rdi, [rel arena_struct]
    call arena_reset
    test rax, rax
    jnz .unmap_fail
    lea rdi, [rel arena_struct]
    mov esi, 8
    mov edx, 8
    call arena_alloc_aligned
    test rax, rax
    jnz .unmap_fail
    cmp rdx, r12
    jne .unmap_fail

    ; Unmap successfully.
    mov rdi, r12
    mov esi, 4096
    call vm_unmap
    test rax, rax
    jnz .fail

    xor edi, edi
    jmp .exit

.unmap_fail:
    mov rdi, r12
    mov esi, 4096
    call vm_unmap
.fail:
    mov edi, 1
.exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
