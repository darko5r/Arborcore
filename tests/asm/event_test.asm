; Arborcore epoll engine test; exit 0 pass, 1 fail
%define SYS_EXIT    60
%define SYS_EVENTFD2 290
%define SYS_WRITE    1
%define SYS_CLOSE    3
%define EPOLLIN      0x001
%define EPOLLOUT     0x004
%define EPOLL_CLOEXEC 0x80000

global _start
extern event_epoll_create
extern event_epoll_add
extern event_epoll_modify
extern event_epoll_remove
extern event_epoll_wait

section .bss
align 16
events: resb 48
value:  resq 1

section .text
_start:
    mov edi, EPOLL_CLOEXEC
    call event_epoll_create
    test rax, rax
    js fail
    mov r12, rax

    xor edi, edi
    xor esi, esi
    mov eax, SYS_EVENTFD2
    syscall
    test rax, rax
    js fail_close_epoll
    mov r13, rax

    mov rdi, r12
    mov rsi, r13
    mov edx, EPOLLIN
    mov rcx, 0x1122334455667788
    call event_epoll_add
    test rax, rax
    js fail_close_both

    ; No readiness yet.
    mov rdi, r12
    lea rsi, [rel events]
    mov edx, 4
    xor ecx, ecx
    call event_epoll_wait
    test rax, rax
    jnz fail_close_both

    mov qword [rel value], 1
    mov rdi, r13
    lea rsi, [rel value]
    mov edx, 8
    mov eax, SYS_WRITE
    syscall
    cmp rax, 8
    jne fail_close_both

    mov rdi, r12
    lea rsi, [rel events]
    mov edx, 4
    mov ecx, 1000
    call event_epoll_wait
    cmp rax, 1
    jne fail_close_both
    test dword [rel events + 0], EPOLLIN
    jz fail_close_both
    mov rax, [rel events + 4]
    mov rdx, 0x1122334455667788
    cmp rax, rdx
    jne fail_close_both

    ; Modify is accepted and remove closes the registration boundary.
    mov rdi, r12
    mov rsi, r13
    mov edx, EPOLLIN | EPOLLOUT
    mov rcx, 0x8877665544332211
    call event_epoll_modify
    test rax, rax
    js fail_close_both

    mov rdi, r12
    mov rsi, r13
    call event_epoll_remove
    test rax, rax
    js fail_close_both

    ; Invalid wait arguments are rejected before syscall.
    mov rdi, r12
    xor esi, esi
    mov edx, 1
    xor ecx, ecx
    call event_epoll_wait
    cmp rax, -22
    jne fail_close_both

    xor edi, edi
    jmp close_both
fail_close_both:
    mov edi, 1
close_both:
    mov r14d, edi
    mov rdi, r13
    mov eax, SYS_CLOSE
    syscall
    mov rdi, r12
    mov eax, SYS_CLOSE
    syscall
    mov edi, r14d
    jmp exit
fail_close_epoll:
    mov rdi, r12
    mov eax, SYS_CLOSE
    syscall
fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
