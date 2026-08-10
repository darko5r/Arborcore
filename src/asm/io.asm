; Arborcore Linux file-descriptor I/O primitives
;
; Linux x86-64, no libc.
;
; io_read_retry(fd, buffer, length)
;   -> RAX = bytes read or negative errno
;   retries EINTR; zero length returns 0 without a syscall.
;
; io_write_retry(fd, buffer, length)
;   -> RAX = bytes written or negative errno
;   retries EINTR; unlike write_all this performs one successful write.
;
; io_close(fd)
;   -> RAX = 0 or negative errno. Close is not retried after EINTR.
;
; io_set_nonblocking(fd)
;   -> RAX = 0 or negative errno.

%define SYS_READ   0
%define SYS_WRITE  1
%define SYS_CLOSE  3
%define SYS_FCNTL 72

%define F_GETFL 3
%define F_SETFL 4
%define O_NONBLOCK 0x800

%define ERR_EINTR -4


global io_read_retry:function
global io_write_retry:function
global io_close:function
global io_set_nonblocking:function

section .text

io_read_retry:
    test rdx, rdx
    jz .zero
.retry:
    mov eax, SYS_READ
    syscall
    cmp rax, ERR_EINTR
    je .retry
    ret
.zero:
    xor eax, eax
    ret

io_write_retry:
    test rdx, rdx
    jz .zero
.retry:
    mov eax, SYS_WRITE
    syscall
    cmp rax, ERR_EINTR
    je .retry
    ret
.zero:
    xor eax, eax
    ret

io_close:
    mov eax, SYS_CLOSE
    syscall
    ret

io_set_nonblocking:
    mov r8, rdi
    mov eax, SYS_FCNTL
    mov esi, F_GETFL
    xor edx, edx
    syscall
    test rax, rax
    js .done

    or eax, O_NONBLOCK
    mov edx, eax
    mov rdi, r8
    mov eax, SYS_FCNTL
    mov esi, F_SETFL
    syscall
.done:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
