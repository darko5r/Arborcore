; Arborcore real loopback keep-alive benchmark
; One persistent TCP connection, sequential HTTP requests.
; Output: metric<TAB>iterations<TAB>total_ns

%define SYS_EXIT        60
%define SYS_CONNECT     42
%define SYS_GETSOCKNAME 51
%define SYS_CLOSE        3
%define ERR_EAGAIN     -11
%define ITERATIONS      1500

extern buffer_init
extern arena_init
extern net_socket_tcp4
extern io_write_retry
extern io_read_retry
extern event_epoll_wait
extern server_open_listener
extern server_create_epoll
extern server_accept_connection
extern server_handle_http_once
extern server_close_connection
extern bench_now_ns
extern bench_emit_result

global _start

section .rodata
method_get: db "GET"
path_ok: db "/ok/:id"
request:
    db "GET /ok/42 HTTP/1.1",13,10
    db "Host: localhost",13,10
    db "Content-Length: 0",13,10
    db 13,10
request_len equ $ - request
response:
    db "HTTP/1.1 200 OK",13,10
    db "Content-Length: 0",13,10
    db "Connection: keep-alive",13,10
    db 13,10
response_len equ $ - response
metric_loopback: db "loopback_keepalive"
metric_loopback_len equ $ - metric_loopback

section .data
align 8
routes: dq method_get, 3, path_ok, 7, bench_handler
context: dq 0x123456789abcdef0
listener_addr:
    dw 2
    dw 0
    dd 0x0100007f
    dq 0
client_addr:
    times 16 db 0
addr_len: dd 16

section .bss
align 16
events: resb 48
inbuf: resb 24
outbuf: resb 24
in_storage: resb 4096
out_storage: resb 4096
arena: resb 24
arena_storage: resb 1024
conn: resb 80
request_out: resb 96
client_read: resb 256

section .text
_start:
    push r12
    push r13
    push r14
    push r15

    ; Listener and epoll.
    lea rdi, [rel listener_addr]
    mov esi, 16
    mov edx, 128
    call server_open_listener
    test rax, rax
    js fail_setup
    mov r12, rax

    mov rdi, r12
    call server_create_epoll
    test rax, rax
    js fail_listener
    mov r13, rax

    mov rdi, r12
    lea rsi, [rel client_addr]
    lea rdx, [rel addr_len]
    mov eax, SYS_GETSOCKNAME
    syscall
    test rax, rax
    js fail_epoll

    call net_socket_tcp4
    test rax, rax
    js fail_epoll
    mov r14, rax

    mov rdi, r14
    lea rsi, [rel client_addr]
    mov edx, 16
    mov eax, SYS_CONNECT
    syscall
    test rax, rax
    js fail_client

    ; Listener readiness.
    mov rdi, r13
    lea rsi, [rel events]
    mov edx, 4
    mov ecx, 1000
    call event_epoll_wait
    cmp rax, 1
    jl fail_client

    lea rdi, [rel inbuf]
    lea rsi, [rel in_storage]
    mov edx, 4096
    call buffer_init
    test rax, rax
    jnz fail_client

    lea rdi, [rel outbuf]
    lea rsi, [rel out_storage]
    mov edx, 4096
    call buffer_init
    test rax, rax
    jnz fail_client

    lea rdi, [rel arena]
    lea rsi, [rel arena_storage]
    mov edx, 1024
    call arena_init
    test rax, rax
    jnz fail_client

    mov rdi, r12
    mov rsi, r13
    lea rdx, [rel conn]
    lea rcx, [rel inbuf]
    lea r8, [rel outbuf]
    lea r9, [rel arena]
    call server_accept_connection
    test rax, rax
    js fail_client

    ; One untimed warm request.
    call one_request
    test rax, rax
    jnz fail_conn

    call bench_now_ns
    test rax, rax
    js fail_conn
    mov r15, rax
    mov ebx, ITERATIONS

.loop:
    call one_request
    test rax, rax
    jnz fail_conn
    dec rbx
    jnz .loop

    call bench_now_ns
    test rax, rax
    js fail_conn
    sub rax, r15
    mov rcx, rax

    lea rdi, [rel metric_loopback]
    mov esi, metric_loopback_len
    mov edx, ITERATIONS
    call bench_emit_result
    test rax, rax
    js fail_conn

    xor r15d, r15d
    jmp cleanup_conn

one_request:
    sub rsp, 8
    mov rdi, r14
    lea rsi, [rel request]
    mov edx, request_len
    call write_exact
    test rax, rax
    jnz .done

    call wait_connection
    test rax, rax
    js .done

.retry_server:
    lea rdi, [rel conn]
    lea rsi, [rel request_out]
    lea rdx, [rel routes]
    mov ecx, 1
    lea r8, [rel context]
    mov r9, r13
    call server_handle_http_once
    cmp rax, ERR_EAGAIN
    jne .server_done
    call wait_connection
    test rax, rax
    js .done
    jmp .retry_server
.server_done:
    test rax, rax
    jnz .done

    mov rdi, r14
    lea rsi, [rel client_read]
    mov edx, response_len
    call read_exact
.done:
    add rsp, 8
    ret

wait_connection:
    sub rsp, 8
    mov rdi, r13
    lea rsi, [rel events]
    mov edx, 4
    mov ecx, 1000
    call event_epoll_wait
    cmp rax, 1
    jl .bad
    xor eax, eax
    add rsp, 8
    ret
.bad:
    mov rax, -1
    add rsp, 8
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
    jz .zero
    add r13, rax
    sub r14, rax
    jmp .loop
.ok:
    xor eax, eax
    jmp .return
.zero:
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
    jz .zero
    add r13, rax
    sub r14, rax
    jmp .loop
.ok:
    xor eax, eax
    jmp .return
.zero:
    mov rax, -5
.return:
    pop r14
    pop r13
    pop r12
    ret

bench_handler:
    mov eax, 200
    ret

fail_conn:
    mov r15d, 1
cleanup_conn:
    mov rdi, r13
    lea rsi, [rel conn]
    call server_close_connection
    mov rdi, r14
    mov eax, SYS_CLOSE
    syscall
    mov rdi, r13
    mov eax, SYS_CLOSE
    syscall
    mov rdi, r12
    mov eax, SYS_CLOSE
    syscall
    mov edi, r15d
    jmp exit

fail_client:
    mov rdi, r14
    mov eax, SYS_CLOSE
    syscall
fail_epoll:
    mov rdi, r13
    mov eax, SYS_CLOSE
    syscall
fail_listener:
    mov rdi, r12
    mov eax, SYS_CLOSE
    syscall
fail_setup:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
