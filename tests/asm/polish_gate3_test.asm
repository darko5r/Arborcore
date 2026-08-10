; Arborcore Polish Gate #3 real loopback HTTP lifecycle test
; exit 0 pass, 1 fail
%define SYS_EXIT        60
%define SYS_CONNECT     42
%define SYS_GETSOCKNAME 51
%define SYS_CLOSE        3
%define EPOLLIN       0x001
%define ERR_EAGAIN    -11

extern buffer_init
extern arena_init
extern arena_alloc
extern net_socket_tcp4
extern io_write_retry
extern io_read_retry
extern event_epoll_wait
extern server_open_listener
extern server_create_epoll
extern server_accept_connection
extern server_handle_http_once
extern server_close_connection

global _start

section .rodata
method_get: db "GET"
path_ok: db "/ok/:id"
req1: db "GET /ok/42 HTTP/1.1",13,10,"Host: localhost",13,10,13,10
req1_len equ $ - req1
req2: db "GET /missing HTTP/1.1",13,10,13,10
req2_len equ $ - req2
resp200:
    db "HTTP/1.1 200 OK",13,10
    db "Content-Length: 0",13,10
    db "Connection: keep-alive",13,10
    db 13,10
resp200_len equ $ - resp200
resp404:
    db "HTTP/1.1 404 Not Found",13,10
    db "Content-Length: 0",13,10
    db "Connection: keep-alive",13,10
    db 13,10
resp404_len equ $ - resp404

section .data
align 8
routes: dq method_get, 3, path_ok, 7, gate3_handler
route_count equ 1
context: dq 0x0badf00dcafebabe

; sockaddr_in: family, network-order port, 127.0.0.1, padding
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
in_storage: resb 2048
out_storage: resb 2048
arena: resb 24
arena_storage: resb 512
conn: resb 80
request_out: resb 96
client_read: resb 256

section .text
_start:
    mov r15d, 1                  ; default failure status for cleanup paths
    ; Listener on loopback ephemeral port.
    lea rdi, [rel listener_addr]
    mov esi, 16
    mov edx, 16
    call server_open_listener
    test rax, rax
    js gate3_fail
    mov r12, rax

    mov rdi, r12
    call server_create_epoll
    test rax, rax
    js gate3_close_listener_fail
    mov r13, rax

    ; Discover selected port/address and copy sockaddr to client buffer.
    mov rdi, r12
    lea rsi, [rel client_addr]
    lea rdx, [rel addr_len]
    mov eax, SYS_GETSOCKNAME
    syscall
    test rax, rax
    js gate3_close_event_fail

    ; Blocking client connect to local listener.
    call net_socket_tcp4
    test rax, rax
    js gate3_close_event_fail
    mov r14, rax
    mov rdi, r14
    lea rsi, [rel client_addr]
    mov edx, 16
    mov eax, SYS_CONNECT
    syscall
    test rax, rax
    js gate3_close_client_fail

    ; Listener readiness arrives through epoll.
    mov rdi, r13
    lea rsi, [rel events]
    mov edx, 4
    mov ecx, 1000
    call event_epoll_wait
    cmp rax, 1
    jl gate3_close_client_fail

    ; Prepare request-lifetime structures before accept.
    lea rdi, [rel inbuf]
    lea rsi, [rel in_storage]
    mov edx, 2048
    call buffer_init
    test rax, rax
    jnz gate3_close_client_fail
    lea rdi, [rel outbuf]
    lea rsi, [rel out_storage]
    mov edx, 2048
    call buffer_init
    test rax, rax
    jnz gate3_close_client_fail
    lea rdi, [rel arena]
    lea rsi, [rel arena_storage]
    mov edx, 512
    call arena_init
    test rax, rax
    jnz gate3_close_client_fail

    mov rdi, r12
    mov rsi, r13
    lea rdx, [rel conn]
    lea rcx, [rel inbuf]
    lea r8, [rel outbuf]
    lea r9, [rel arena]
    call server_accept_connection
    test rax, rax
    js gate3_close_client_fail

    ; Dirty the request arena; completed lifecycle must reset it to offset 0.
    lea rdi, [rel arena]
    mov esi, 32
    call arena_alloc
    test rax, rax
    jnz gate3_close_conn_fail
    cmp qword [rel arena + 16], 32
    jne gate3_close_conn_fail

    ; Request #1.
    mov rdi, r14
    lea rsi, [rel req1]
    mov edx, req1_len
    call write_exact
    test rax, rax
    jne gate3_close_conn_fail
    call wait_connection
    test rax, rax
    js gate3_close_conn_fail
    call handle_server
    test rax, rax
    jnz gate3_close_conn_fail
    cmp qword [rel arena + 16], 0
    jne gate3_close_conn_fail
    mov rdi, r14
    lea rsi, [rel client_read]
    mov edx, resp200_len
    call read_exact
    test rax, rax
    jne gate3_close_conn_fail
    lea rdi, [rel client_read]
    lea rsi, [rel resp200]
    mov edx, resp200_len
    call bytes_match
    cmp eax, 1
    jne gate3_close_conn_fail

    ; Dirty request arena again; second lifecycle must reset it too.
    lea rdi, [rel arena]
    mov esi, 48
    call arena_alloc
    test rax, rax
    jnz gate3_close_conn_fail

    ; Request #2 over the SAME TCP connection proves keep-alive reuse.
    mov rdi, r14
    lea rsi, [rel req2]
    mov edx, req2_len
    call write_exact
    test rax, rax
    jne gate3_close_conn_fail
    call wait_connection
    test rax, rax
    js gate3_close_conn_fail
    call handle_server
    test rax, rax
    jnz gate3_close_conn_fail
    cmp qword [rel arena + 16], 0
    jne gate3_close_conn_fail
    mov rdi, r14
    lea rsi, [rel client_read]
    mov edx, resp404_len
    call read_exact
    test rax, rax
    jne gate3_close_conn_fail
    lea rdi, [rel client_read]
    lea rsi, [rel resp404]
    mov edx, resp404_len
    call bytes_match
    cmp eax, 1
    jne gate3_close_conn_fail
    cmp qword [rel conn + 64], 2
    jne gate3_close_conn_fail

    xor r15d, r15d
    jmp gate3_close_conn

gate3_handler:
    mov rax, 0x0badf00dcafebabe
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

wait_connection:
    sub rsp, 8                    ; align nested call
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

handle_server:
    sub rsp, 8                    ; align nested calls
.retry:
    lea rdi, [rel conn]
    lea rsi, [rel request_out]
    lea rdx, [rel routes]
    mov ecx, route_count
    lea r8, [rel context]
    mov r9, r13
    call server_handle_http_once
    cmp rax, ERR_EAGAIN
    jne .done
    call wait_connection
    test rax, rax
    js .wait_fail
    jmp .retry
.wait_fail:
    mov rax, -1
.done:
    add rsp, 8
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

gate3_close_conn_fail:
    mov r15d, 1
gate3_close_conn:
    mov rdi, r13
    lea rsi, [rel conn]
    call server_close_connection
    test rax, rax
    jz gate3_close_client_fail
    mov r15d, 1
gate3_close_client_fail:
    mov rdi, r14
    mov eax, SYS_CLOSE
    syscall
gate3_close_event_fail:
    mov rdi, r13
    mov eax, SYS_CLOSE
    syscall
gate3_close_listener_fail:
    mov rdi, r12
    mov eax, SYS_CLOSE
    syscall
    mov edi, r15d
    jmp gate3_exit
gate3_fail:
    mov edi, 1
gate3_exit:
    mov eax, SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
