; Arborcore Linux epoll event engine
;
; event_epoll_wait uses a TOTAL monotonic duration.  EINTR retries recompute
; the remaining duration instead of restarting the caller's full timeout.

%define SYS_EPOLL_WAIT     232
%define SYS_EPOLL_CTL      233
%define SYS_CLOCK_GETTIME  228
%define SYS_EPOLL_CREATE1  291

%define CLOCK_MONOTONIC 1
%define EPOLL_CTL_ADD 1
%define EPOLL_CTL_DEL 2
%define EPOLL_CTL_MOD 3

%define ERR_EINTR     -4
%define ERR_EINVAL   -22
%define ERR_EOVERFLOW -75
%define INT_MAX 2147483647

global event_epoll_create:function
global event_epoll_add:function
global event_epoll_modify:function
global event_epoll_remove:function
global event_epoll_wait:function
global event_monotonic_ms:function
global event_deadline_remaining_ms:function

section .text

event_epoll_create:
    mov eax, SYS_EPOLL_CREATE1
    syscall
    ret

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

event_epoll_remove:
    mov rdx, rsi
    mov esi, EPOLL_CTL_DEL
    xor r10d, r10d
    mov eax, SYS_EPOLL_CTL
    syscall
    ret

; event_monotonic_ms() -> RAX=monotonic milliseconds or negative errno.
event_monotonic_ms:
    sub rsp, 16
    mov edi, CLOCK_MONOTONIC
    mov rsi, rsp
    mov eax, SYS_CLOCK_GETTIME
    syscall
    test rax, rax
    js .return
    mov r8, [rsp + 0]
    imul r8, r8, 1000
    jo .overflow
    mov rax, [rsp + 8]
    xor edx, edx
    mov ecx, 1000000
    div rcx
    add rax, r8
    jc .overflow
    jmp .return
.overflow:
    mov rax, ERR_EOVERFLOW
.return:
    add rsp, 16
    ret

; event_deadline_remaining_ms(deadline_ms, now_ms)
; -> RAX=0 when expired, otherwise remaining duration clamped to INT_MAX.
event_deadline_remaining_ms:
    cmp rsi, rdi
    jae .expired
    mov rax, rdi
    sub rax, rsi
    cmp rax, INT_MAX
    jbe .return
    mov eax, INT_MAX
.return:
    ret
.expired:
    xor eax, eax
    ret

; RDI=epfd RSI=events* RDX=maxevents RCX=timeout_ms
; timeout domain is Linux int: -1 (infinite) through INT_MAX.
event_epoll_wait:
    test rsi, rsi
    jz .invalid_direct
    test rdx, rdx
    jz .invalid_direct
    cmp rdx, INT_MAX
    ja .invalid_direct
    cmp rcx, -1
    jl .invalid_direct
    cmp rcx, INT_MAX
    jg .invalid_direct

    cmp rcx, 0
    jle .simple_retry

    push r12
    push r13
    push r14
    push r15
    sub rsp, 8
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx

    call event_monotonic_ms
    test rax, rax
    js .timed_return
    add r15, rax
    jc .timed_overflow

.timed_retry:
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    ; Initial call can use the caller duration; retries overwrite R10 below.
    ; timeout was validated <= INT_MAX and CLOCK_MONOTONIC never moves
    ; backward, so deadline-now cannot exceed the caller's original timeout.
    mov r10, r15
    sub r10, rax
    mov eax, SYS_EPOLL_WAIT
    syscall
    cmp rax, ERR_EINTR
    jne .timed_return

    call event_monotonic_ms
    test rax, rax
    js .timed_return
    cmp rax, r15
    jae .timed_expired
    jmp .timed_retry

.timed_expired:
    xor eax, eax
    jmp .timed_return
.timed_overflow:
    mov rax, ERR_EOVERFLOW
.timed_return:
    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    ret

.simple_retry:
    mov r8, rcx
.retry_simple:
    mov r10, r8
    mov eax, SYS_EPOLL_WAIT
    syscall
    cmp rax, ERR_EINTR
    je .retry_simple
    ret

.invalid_direct:
    mov rax, ERR_EINVAL
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
