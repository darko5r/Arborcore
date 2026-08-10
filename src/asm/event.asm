; Arborcore Linux epoll event engine
;
; Linux x86-64, no libc.
;
; epoll_event ABI used here is the Linux packed 12-byte layout:
;   +0  uint32 events
;   +4  uint64 data
;
; event_epoll_create(flags) -> RAX=epoll fd or negative errno
; event_epoll_add(epfd, fd, events, data) -> RAX=0 or negative errno
; event_epoll_modify(epfd, fd, events, data) -> RAX=0 or negative errno
; event_epoll_remove(epfd, fd) -> RAX=0 or negative errno
; event_epoll_wait(epfd, events*, maxevents, timeout_ms)
;   -> RAX=count or negative errno; retries EINTR only.

%define SYS_EPOLL_WAIT     232
%define SYS_EPOLL_CTL      233
%define SYS_EPOLL_CREATE1  291

%define EPOLL_CTL_ADD 1
%define EPOLL_CTL_DEL 2
%define EPOLL_CTL_MOD 3

%define ERR_EINTR  -4
%define ERR_EINVAL -22

global event_epoll_create:function
global event_epoll_add:function
global event_epoll_modify:function
global event_epoll_remove:function
global event_epoll_wait:function

section .text

event_epoll_create:
    mov eax, SYS_EPOLL_CREATE1
    syscall
    ret

; RDI=epfd RSI=fd RDX=events RCX=data
event_epoll_add:
    mov r8, rsi
    sub rsp, 16
    mov dword [rsp + 0], edx
    mov qword [rsp + 4], rcx
    mov esi, EPOLL_CTL_ADD
    mov rdx, r8
    mov r10, rsp
    mov eax, SYS_EPOLL_CTL
    syscall
    add rsp, 16
    ret

; RDI=epfd RSI=fd RDX=events RCX=data
event_epoll_modify:
    mov r8, rsi
    sub rsp, 16
    mov dword [rsp + 0], edx
    mov qword [rsp + 4], rcx
    mov esi, EPOLL_CTL_MOD
    mov rdx, r8
    mov r10, rsp
    mov eax, SYS_EPOLL_CTL
    syscall
    add rsp, 16
    ret

; RDI=epfd RSI=fd
event_epoll_remove:
    mov rdx, rsi
    mov esi, EPOLL_CTL_DEL
    xor r10d, r10d
    mov eax, SYS_EPOLL_CTL
    syscall
    ret

; RDI=epfd RSI=events* RDX=maxevents RCX=timeout_ms
event_epoll_wait:
    test rsi, rsi
    jz .invalid
    test rdx, rdx
    jz .invalid
    mov r8, rcx                  ; syscall clobbers RCX; preserve timeout for EINTR retry
.retry:
    mov r10, r8
    mov eax, SYS_EPOLL_WAIT
    syscall
    cmp rax, ERR_EINTR
    je .retry
    ret
.invalid:
    mov rax, ERR_EINVAL
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
