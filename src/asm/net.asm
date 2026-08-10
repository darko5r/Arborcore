; Arborcore Linux TCP socket primitives
;
; Linux x86-64 syscall wrappers, no libc.
; Descriptor close delegates to the shared I/O layer.
;
; net_socket_tcp4() -> RAX=fd or negative errno
; net_bind(fd, sockaddr*, length) -> RAX=0 or negative errno
; net_listen(fd, backlog) -> RAX=0 or negative errno
; net_accept4(fd, sockaddr*, socklen_t*, flags) -> RAX=fd or negative errno
; net_shutdown(fd, how) -> RAX=0 or negative errno
; net_close(fd) -> RAX=0 or negative errno
;
; sockaddr layout is intentionally supplied by the caller so the
; primitive boundary remains compatible with Linux sockaddr types.

%define SYS_SOCKET    41
%define SYS_ACCEPT4  288
%define SYS_BIND      49
%define SYS_LISTEN    50
%define SYS_SHUTDOWN  48

%define AF_INET        2
%define SOCK_STREAM    1
%define IPPROTO_TCP    6


global net_socket_tcp4:function
global net_bind:function
global net_listen:function
global net_accept4:function
global net_shutdown:function
global net_close:function

extern io_close

section .text

net_socket_tcp4:
    mov edi, AF_INET
    mov esi, SOCK_STREAM
    mov edx, IPPROTO_TCP
    mov eax, SYS_SOCKET
    syscall
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
