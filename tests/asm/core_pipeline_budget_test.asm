; Arborcore Retrofit E4/E7/E9 pipeline draining, immediate write, work budget
%define SYS_EXIT 60
%define SYS_SOCKETPAIR 53
%define SYS_CLOSE 3
%define AF_UNIX 1
%define SOCK_STREAM 1
%define EPOLLIN 0x001
%define EPOLLERR 0x008
%define EPOLLHUP 0x010
%define EPOLLRDHUP 0x2000
%define ERR_EAGAIN -11
%define SERVER_MORE_WORK 1

extern buffer_init
extern arena_init
extern connection_init
extern connection_transition
extern io_set_nonblocking
extern io_write_retry
extern io_read_retry
extern event_epoll_create
extern event_epoll_add
extern server_handle_http_once

global _start

section .rodata
method_get: db "GET"
path_root: db "/"
one_request: db "GET / HTTP/1.1",13,10,13,10
one_request_len equ $ - one_request
response:
    db "HTTP/1.1 200 OK",13,10
    db "Content-Length: 0",13,10
    db "Connection: keep-alive",13,10
    db 13,10
response_len equ $ - response
pipeline:
%rep 10
    db "GET / HTTP/1.1",13,10,13,10
%endrep
pipeline_len equ $ - pipeline
responses_total equ response_len * 10

section .data
align 8
routes: dq method_get, 3, path_root, 1, ok_handler
context: dq 0x1234

section .bss
alignb 16
fds: resd 2
inbuf: resb 24
outbuf: resb 24
in_storage: resb 4096
out_storage: resb 1024
arena: resb 24
arena_storage: resb 512
conn: resb 96
request_out: resb 96
client_read: resb 1024

section .text
_start:
    mov edi, AF_UNIX
    mov esi, SOCK_STREAM
    xor edx, edx
    lea r10, [rel fds]
    mov eax, SYS_SOCKETPAIR
    syscall
    test rax, rax
    js fail

    mov edi, [rel fds]
    call io_set_nonblocking
    test rax, rax
    js fail_close

    lea rdi, [rel inbuf]
    lea rsi, [rel in_storage]
    mov edx, 4096
    call buffer_init
    test rax, rax
    jnz fail_close
    lea rdi, [rel outbuf]
    lea rsi, [rel out_storage]
    mov edx, 1024
    call buffer_init
    test rax, rax
    jnz fail_close
    lea rdi, [rel arena]
    lea rsi, [rel arena_storage]
    mov edx, 512
    call arena_init
    test rax, rax
    jnz fail_close

    lea rdi, [rel conn]
    mov esi, [rel fds]
    lea rdx, [rel inbuf]
    lea rcx, [rel outbuf]
    lea r8, [rel arena]
    call connection_init
    test rax, rax
    jnz fail_close
    lea rdi, [rel conn]
    mov esi, 2
    call connection_transition
    test rax, rax
    jnz fail_close

    xor edi, edi
    call event_epoll_create
    test rax, rax
    js fail_close
    mov r12, rax
    mov rdi, r12
    mov esi, [rel fds]
    mov edx, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP
    lea rcx, [rel conn]
    call event_epoll_add
    test rax, rax
    js fail_close_all

    mov edi, [rel fds + 4]
    lea rsi, [rel pipeline]
    mov edx, pipeline_len
    call write_exact
    test rax, rax
    jne fail_close_all

    ; First call is capped at 8 complete requests and explicitly reports local
    ; work remaining instead of forcing epoll_wait.
    call handle
    cmp rax, SERVER_MORE_WORK
    jne fail_close_all
    cmp rdx, 8
    jne fail_close_all
    cmp qword [rel conn + 64], 8
    jne fail_close_all

    ; Immediate continuation drains the final two and then reaches read EAGAIN.
    call handle
    test rax, rax
    jnz fail_close_all
    cmp rdx, 10
    jne fail_close_all
    cmp qword [rel conn + 64], 10
    jne fail_close_all
    cmp qword [rel conn + 16], 0      ; EPOLLOUT was never armed on fast writes
    jne fail_close_all
    cmp qword [rel inbuf + 8], 0
    jne fail_close_all
    cmp qword [rel arena + 16], 0
    jne fail_close_all
    cmp qword [rel conn + 80], 0
    jne fail_close_all
    cmp qword [rel conn + 88], 0
    jne fail_close_all

    mov edi, [rel fds + 4]
    lea rsi, [rel client_read]
    mov edx, responses_total
    call read_exact
    test rax, rax
    jne fail_close_all

    xor r14d, r14d
    jmp close_all

handle:
    sub rsp, 8
    lea rdi, [rel conn]
    lea rsi, [rel request_out]
    lea rdx, [rel routes]
    mov ecx, 1
    lea r8, [rel context]
    mov r9, r12
    call server_handle_http_once
    add rsp, 8
    ret

ok_handler:
    mov eax, 200
    ret

write_exact:
    push r12
    push r13
    push r14
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
.loop:
    test r14, r14
    jz .ok
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    call io_write_retry
    test rax, rax
    js .return
    jz .bad
    add r13, rax
    sub r14, rax
    jmp .loop
.ok:
    xor eax, eax
    jmp .return
.bad:
    mov rax, -5
.return:
    pop r14
    pop r13
    pop r12
    ret

read_exact:
    push r12
    push r13
    push r14
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
.loop:
    test r14, r14
    jz .ok
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    call io_read_retry
    test rax, rax
    js .return
    jz .bad
    add r13, rax
    sub r14, rax
    jmp .loop
.ok:
    xor eax, eax
    jmp .return
.bad:
    mov rax, -5
.return:
    pop r14
    pop r13
    pop r12
    ret

fail_close_all:
    mov r14d, 1
close_all:
    mov rdi, r12
    mov eax, SYS_CLOSE
    syscall
fail_close:
    mov edi, [rel fds]
    mov eax, SYS_CLOSE
    syscall
    mov edi, [rel fds + 4]
    mov eax, SYS_CLOSE
    syscall
    mov edi, r14d
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
