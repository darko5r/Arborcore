; Arborcore Retrofit E9 epoll batch-delivery qualification
%define SYS_EXIT 60
%define SYS_EVENTFD2 290
%define SYS_WRITE 1
%define SYS_CLOSE 3
%define EPOLLIN 0x001

extern event_epoll_create
extern event_epoll_add
extern event_epoll_wait

global _start

section .bss
alignb 16
events: resb 96                 ; 8 packed epoll_event records × 12 bytes
fds: resd 8
value: resq 1
seen: resb 8

section .text
_start:
    xor edi, edi
    call event_epoll_create
    test rax, rax
    js fail
    mov r12, rax

    xor r13d, r13d
.create_loop:
    cmp r13d, 8
    jae .signal_all
    xor edi, edi
    xor esi, esi
    mov eax, SYS_EVENTFD2
    syscall
    test rax, rax
    js fail_close_epoll
    lea r8, [rel fds]
    mov [r8 + r13*4], eax
    mov rdi, r12
    mov esi, eax
    mov edx, EPOLLIN
    mov rcx, r13
    inc rcx                         ; tags 1..8
    call event_epoll_add
    test rax, rax
    js fail_close_fds
    inc r13d
    jmp .create_loop

.signal_all:
    mov qword [rel value], 1
    xor r13d, r13d
.signal_loop:
    cmp r13d, 8
    jae .wait_batch
    lea r8, [rel fds]
    mov edi, [r8 + r13*4]
    lea rsi, [rel value]
    mov edx, 8
    mov eax, SYS_WRITE
    syscall
    cmp rax, 8
    jne fail_close_fds
    inc r13d
    jmp .signal_loop

.wait_batch:
    mov rdi, r12
    lea rsi, [rel events]
    mov edx, 8
    mov ecx, 1000
    call event_epoll_wait
    cmp rax, 8
    jne fail_close_fds

    xor r13d, r13d
.verify_loop:
    cmp r13d, 8
    jae .verify_seen
    mov rax, r13
    imul rax, rax, 12
    lea r8, [rel events]
    test dword [r8 + rax], EPOLLIN
    jz fail_close_fds
    mov rcx, [r8 + rax + 4]
    test rcx, rcx
    jz fail_close_fds
    cmp rcx, 8
    ja fail_close_fds
    dec rcx
    lea r9, [rel seen]
    cmp byte [r9 + rcx], 0
    jne fail_close_fds             ; duplicate tag
    mov byte [r9 + rcx], 1
    inc r13d
    jmp .verify_loop

.verify_seen:
    xor r13d, r13d
.seen_loop:
    cmp r13d, 8
    jae pass_close
    lea r8, [rel seen]
    cmp byte [r8 + r13], 1
    jne fail_close_fds
    inc r13d
    jmp .seen_loop

pass_close:
    xor r14d, r14d
    jmp close_fds
fail_close_fds:
    mov r14d, 1
close_fds:
    xor r13d, r13d
.close_loop:
    cmp r13d, 8
    jae .close_epoll
    lea r8, [rel fds]
    mov edi, [r8 + r13*4]
    test edi, edi
    jl .next
    mov eax, SYS_CLOSE
    syscall
.next:
    inc r13d
    jmp .close_loop
.close_epoll:
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
