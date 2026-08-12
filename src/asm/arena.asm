; Arborcore VM allocation + linear arena engine
;
; Arena layout (24 bytes):
;   +0   base pointer
;   +8   capacity
;   +16  current offset
;
; Status convention for arena operations:
;   RAX = 0      success
;   RAX = -22    EINVAL
;   RAX = -28    ENOSPC
;   RAX = -75    EOVERFLOW
;   RDX = result pointer where applicable, otherwise 0
;
; vm_map_rw returns the mapped address directly in RAX or a negative
; Linux errno. vm_unmap returns 0 or a negative Linux errno.
;
; Alignment is defined on the ABSOLUTE returned address, not merely on
; the arena offset. Therefore arenas created over unaligned caller
; storage still produce correctly aligned allocations.

%define SYS_MMAP    9
%define SYS_MUNMAP 11

%define PROT_READ   1
%define PROT_WRITE  2
%define MAP_PRIVATE 2
%define MAP_ANONYMOUS 0x20

%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75

%define ARENA_SIZE     24
%define ARENA_BASE      0
%define ARENA_CAPACITY  8
%define ARENA_OFFSET   16

global vm_map_rw:function
global vm_unmap:function
global arena_init:function
global arena_alloc:function
global arena_alloc_aligned:function
global arena_mark:function
global arena_rewind:function
global arena_reset:function

section .text

; vm_map_rw(length) -> RAX=address or negative errno
vm_map_rw:
    test rdi, rdi
    jz .invalid
    mov rsi, rdi                   ; length
    xor edi, edi                   ; address hint = NULL
    mov edx, PROT_READ | PROT_WRITE
    mov r10d, MAP_PRIVATE | MAP_ANONYMOUS
    mov r8, -1
    xor r9d, r9d
    mov eax, SYS_MMAP
    syscall
    ret
.invalid:
    mov rax, ERR_EINVAL
    ret

; vm_unmap(address, length) -> RAX=0 or negative errno
vm_unmap:
    test rdi, rdi
    jz .invalid
    test rsi, rsi
    jz .invalid
    mov eax, SYS_MUNMAP
    syscall
    ret
.invalid:
    mov rax, ERR_EINVAL
    ret

; arena_init(arena, base, capacity)
arena_init:
    test rdi, rdi
    jz .invalid

    test rdx, rdx
    jz .store

    test rsi, rsi
    jz .invalid

    ; Establish a representable storage-range invariant once.
    mov rax, rsi
    add rax, rdx
    jc .overflow

.store:
    mov [rdi + ARENA_BASE], rsi
    mov [rdi + ARENA_CAPACITY], rdx
    mov qword [rdi + ARENA_OFFSET], 0
    xor eax, eax
    xor edx, edx
    ret

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret

; arena_alloc(arena, size)
; RDI=arena*, RSI=size; RDX=allocated pointer on success.
arena_alloc:
    mov edx, 1
    jmp arena_alloc_aligned

; arena_alloc_aligned(arena, size, alignment)
; RDI=arena*, RSI=size, RDX=alignment (power-of-two, nonzero)
; RDX=allocated pointer on success.
;
; A zero-size allocation still honors alignment and may advance the
; allocation frontier to the next aligned address.  This behavior is
; intentional and is part of the qualified arena contract.
arena_alloc_aligned:
    test rdi, rdi
    jz .invalid
    test rdx, rdx
    jz .invalid

    mov r8, rdx
    dec r8                         ; alignment mask
    test rdx, r8
    jnz .invalid

    mov r9, [rdi + ARENA_CAPACITY]
    mov r10, [rdi + ARENA_OFFSET]
    cmp r10, r9
    ja .invalid

    mov r11, [rdi + ARENA_BASE]
    test r9, r9
    jz .base_ready
    test r11, r11
    jz .invalid

.base_ready:
    ; current_address = base + current_offset
    mov rax, r11
    add rax, r10
    jc .overflow

    ; Align the ABSOLUTE current address upward.
    add rax, r8
    jc .overflow
    not r8
    and rax, r8                    ; aligned absolute address

    ; aligned_offset = aligned_address - base
    cmp rax, r11
    jb .overflow
    mov rcx, rax
    sub rcx, r11
    cmp rcx, r9
    ja .no_space

    ; end_offset = aligned_offset + size
    mov r8, rcx
    add r8, rsi
    jc .overflow
    cmp r8, r9
    ja .no_space

    mov [rdi + ARENA_OFFSET], r8
    mov rdx, rax
    xor eax, eax
    ret

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

.no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret

; arena_mark(arena) -> RAX=current offset, or 0 for NULL
arena_mark:
    test rdi, rdi
    jz .zero
    mov rax, [rdi + ARENA_OFFSET]
    ret
.zero:
    xor eax, eax
    ret

; arena_rewind(arena, mark)
; Restores the logical allocation frontier only; bytes above the new
; frontier are not cleared or restored.
arena_rewind:
    test rdi, rdi
    jz .invalid
    mov rax, [rdi + ARENA_CAPACITY]
    mov rcx, [rdi + ARENA_OFFSET]
    cmp rcx, rax
    ja .invalid
    cmp rsi, rcx
    ja .invalid
    cmp rsi, rax
    ja .invalid
    mov [rdi + ARENA_OFFSET], rsi
    xor eax, eax
    xor edx, edx
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

; arena_reset(arena)
; Restores the logical allocation frontier to zero; backing bytes are
; intentionally left untouched.
arena_reset:
    test rdi, rdi
    jz .invalid
    mov qword [rdi + ARENA_OFFSET], 0
    xor eax, eax
    xor edx, edx
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
