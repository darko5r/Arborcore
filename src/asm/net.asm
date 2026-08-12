; Arborcore Linux TCP socket primitives
;
; Linux x86-64 syscall wrappers, no libc.
; Descriptor close delegates to the shared I/O layer.
;
; net_socket_tcp4() -> RAX=blocking fd or negative errno
; net_socket_tcp4_flags(flags) -> RAX=fd or negative errno
;   flags are limited to SOCK_NONBLOCK | SOCK_CLOEXEC.
; net_bind(fd, sockaddr*, length) -> RAX=0 or negative errno
; net_listen(fd, backlog) -> RAX=0 or negative errno
; net_accept4(fd, sockaddr*, socklen_t*, flags) -> RAX=fd or negative errno
; net_shutdown(fd, how) -> RAX=0 or negative errno
; net_close(fd) -> RAX=0 or negative errno

%define SYS_SOCKET    41
%define SYS_ACCEPT4  288
%define SYS_BIND      49
%define SYS_LISTEN    50
%define SYS_SHUTDOWN  48

%define AF_INET        2
%define SOCK_STREAM    1
%define SOCK_NONBLOCK  0x800
%define SOCK_CLOEXEC   0x80000
%define IPPROTO_TCP    6
%define ERR_EINVAL    -22


global net_socket_tcp4:function
global net_socket_tcp4_flags:function
global net_bind:function
global net_listen:function
global net_accept4:function
global net_shutdown:function
global net_close:function

extern io_close

section .text

net_socket_tcp4:
    xor edi, edi
    jmp net_socket_tcp4_flags

; RDI = SOCK_NONBLOCK | SOCK_CLOEXEC subset.
net_socket_tcp4_flags:
    mov rax, rdi
    shr rax, 32
    jnz .invalid
    mov eax, edi
    and eax, ~(SOCK_NONBLOCK | SOCK_CLOEXEC)
    jnz .invalid
    mov esi, edi
    or esi, SOCK_STREAM
    mov edi, AF_INET
    mov edx, IPPROTO_TCP
    mov eax, SYS_SOCKET
    syscall
    ret
.invalid:
    mov rax, ERR_EINVAL
    ret

net_bind:
    mov eax, SYS_BIND
    syscall
    ret

net_listen:
    mov eax, SYS_LISTEN
    syscall
    ret

net_accept4:
    mov r10, rcx
    mov eax, SYS_ACCEPT4
    syscall
    ret

net_shutdown:
    mov eax, SYS_SHUTDOWN
    syscall
    ret

net_close:
    jmp io_close

section .note.GNU-stack noalloc noexec nowrite progbits
