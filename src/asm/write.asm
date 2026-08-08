; Arborcore blocking write primitive
;
; write_all(fd, buffer, length)
;
; Input:
;   RDI = file descriptor
;   RSI = address of first byte
;   RDX = number of bytes to write
;
; Return:
;   RAX = 0       complete success
;   RAX < 0       Linux-style negative error
;
; Behavior:
;   - zero length succeeds without issuing a syscall
;   - handles partial writes
;   - retries EINTR
;   - returns other kernel errors unchanged
;   - converts an unexpected zero-progress write into -EIO
;
; Clobbers:
;   RAX
;   RSI
;   RDX
;   RCX
;   R11
;   condition flags
;
; Scope:
;   This is a blocking-I/O primitive.
;   Nonblocking socket handling belongs in the future network layer.


%define SYS_WRITE         1

%define EINTR             4
%define EIO               5


global write_all:function


section .text


write_all:
    test rdx, rdx
    jz .success


.retry:
    mov eax, SYS_WRITE
    syscall

    test rax, rax
    js .syscall_error
    jz .no_progress

    add rsi, rax
    sub rdx, rax

    jnz .retry


.success:
    xor eax, eax
    ret


.syscall_error:
    cmp rax, -EINTR
    je .retry

    ret


.no_progress:
    ; Arborcore-generated error:
    ; bytes remained but the operation made no progress.

    mov rax, -EIO
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
