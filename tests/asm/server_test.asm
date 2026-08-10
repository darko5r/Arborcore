; Arborcore server lifecycle unit integration over local socketpair
; exit 0 pass, 1 fail
%define SYS_EXIT       60
%define SYS_SOCKETPAIR 53
%define SYS_CLOSE       3
%define AF_UNIX         1
%define SOCK_STREAM     1
%define EPOLLIN       0x001
%define EPOLLERR      0x008
%define EPOLLHUP      0x010
%define EPOLLRDHUP    0x2000
%define ERR_EAGAIN      -11

global _start
extern buffer_init
extern arena_init
extern connection_init
extern connection_transition
extern io_set_nonblocking
extern io_write_retry
extern io_read_retry
extern event_epoll_create
extern event_epoll_add
extern event_epoll_wait
extern server_handle_http_once

section .rodata
method_get: db "GET"
path_ok: db "/ok/:id"
request1: db "GET /ok/42 HTTP/1.1",13,10,13,10
request1_len equ $ - request1
request2: db "GET /missing HTTP/1.1",13,10,13,10
request2_len equ $ - request2
pipeline_requests:
    db "GET /ok/42 HTTP/1.1",13,10,13,10
    db "GET /missing HTTP/1.1",13,10,13,10
pipeline_requests_len equ $ - pipeline_requests
expected_200:
    db "HTTP/1.1 200 OK",13,10
    db "Content-Length: 0",13,10
    db "Connection: keep-alive",13,10
    db 13,10
expected_200_len equ $ - expected_200
expected_404:
    db "HTTP/1.1 404 Not Found",13,10
    db "Content-Length: 0",13,10
    db "Connection: keep-alive",13,10
    db 13,10
expected_404_len equ $ - expected_404

section .data
align 8
routes: dq method_get, 3, path_ok, 7, ok_handler
route_count equ 1
context: dq 0x55aa55aa55aa55aa

section .bss
align 16
fds: resd 2
inbuf: resb 24
outbuf: resb 24
in_storage: resb 1024
out_storage: resb 1024
arena: resb 24
arena_storage: resb 256
conn: resb 80
request_out: resb 96
events: resb 24
client_read: resb 256

section .text
_start:
    mov r14d, 1                  ; default failure status for cleanup paths
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
    mov edx, 1024
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
    mov edx, 256
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

    ; Pipeline two requests in one client write. The server must consume
    ; exactly one message at a time while preserving buffered remainder.
    mov edi, [rel fds + 4]
    lea rsi, [rel pipeline_requests]
    mov edx, pipeline_requests_len
    call write_exact
    test rax, rax
    jne fail_close_all

    ; First pipelined request -> 200.
    call handle_once
    test rax, rax
    jnz fail_close_all
    mov edi, [rel fds + 4]
    lea rsi, [rel client_read]
    mov edx, expected_200_len
    call read_exact
    test rax, rax
    jne fail_close_all
    lea rdi, [rel client_read]
    lea rsi, [rel expected_200]
    mov edx, expected_200_len
    call bytes_match
    cmp eax, 1
    jne fail_close_all

    ; Second pipelined request is already buffered/kernel-pending -> 404.
    call handle_once
    test rax, rax
    jnz fail_close_all
    mov edi, [rel fds + 4]
    lea rsi, [rel client_read]
    mov edx, expected_404_len
    call read_exact
    test rax, rax
    jne fail_close_all
    lea rdi, [rel client_read]
    lea rsi, [rel expected_404]
    mov edx, expected_404_len
    call bytes_match
    cmp eax, 1
    jne fail_close_all

    cmp qword [rel conn + 64], 2
    jne fail_close_all
    xor r14d, r14d
    jmp close_all

handle_once:
    sub rsp, 8                    ; align nested calls
.retry:
    lea rdi, [rel conn]
    lea rsi, [rel request_out]
    lea rdx, [rel routes]
    mov ecx, route_count
    lea r8, [rel context]
    mov r9, r12
    call server_handle_http_once
    cmp rax, ERR_EAGAIN
    jne .done
    mov rdi, r12
    lea rsi, [rel events]
    mov edx, 2
    mov ecx, 1000
    call event_epoll_wait
    cmp rax, 1
    jl .wait_fail
    jmp .retry
.wait_fail:
    mov rax, -1
.done:
    add rsp, 8
    ret

ok_handler:
    mov rax, 0x55aa55aa55aa55aa
    cmp [rsi], rax
    jne .bad
    cmp rcx, 1
    jne .bad
    cmp qword [rdx + 24], 2
    jne .bad
    mov rax, [rdx + 16]
    cmp byte [rax], '4'
    jne .bad
    cmp byte [rax + 1], '2'
    jne .bad
    mov eax, 200
    ret
.bad:
    mov rax, -99
    ret

write_exact:
    push r12
    push r13
    push r14
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
.write_loop:
    test r14, r14
    jz .write_ok
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    call io_write_retry
    test rax, rax
    js .write_return
    jz .write_zero
    add r13, rax
    sub r14, rax
    jmp .write_loop
.write_ok:
    xor eax, eax
    jmp .write_return
.write_zero:
    mov rax, -5
.write_return:
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
.read_loop:
    test r14, r14
    jz .read_ok
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    call io_read_retry
    test rax, rax
    js .read_return
    jz .read_zero
    add r13, rax
    sub r14, rax
    jmp .read_loop
.read_ok:
    xor eax, eax
    jmp .read_return
.read_zero:
    mov rax, -5
.read_return:
    pop r14
    pop r13
    pop r12
    ret

bytes_match:
    test rdx, rdx
    jz .yes
.loop:
    mov al, [rdi]
    cmp al, [rsi]
    jne .no
    inc rdi
    inc rsi
    dec rdx
    jnz .loop
.yes:
    mov eax, 1
    ret
.no:
    xor eax, eax
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
