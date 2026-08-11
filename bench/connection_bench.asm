; Arborcore local TCP connection benchmark
; measures socket + connect + accept4 + close for one connection lifecycle.
; Output: metric<TAB>iterations<TAB>total_ns

%define SYS_EXIT        60
%define SYS_CONNECT     42
%define SYS_GETSOCKNAME 51
%define ITERATIONS      1000
%define SOCK_CLOEXEC    0x80000

extern net_socket_tcp4
extern net_bind
extern net_listen
extern net_accept4
extern net_close
extern io_set_nonblocking
extern bench_now_ns
extern bench_emit_result

global _start

section .rodata
metric_conn: db "tcp_connect_accept_close"
metric_conn_len equ $ - metric_conn

section .data
listener_addr:
    dw 2
    dw 0
    dd 0x0100007f
    dq 0
client_addr:
    times 16 db 0
addr_len: dd 16

section .text
_start:
    push r12
    push r13
    push r14
    push r15

    call net_socket_tcp4
    test rax, rax
    js fail_setup
    mov r12, rax                  ; listener

    mov rdi, r12
    call io_set_nonblocking
    test rax, rax
    js fail_listener

    mov rdi, r12
    lea rsi, [rel listener_addr]
    mov edx, 16
    call net_bind
    test rax, rax
    js fail_listener

    mov rdi, r12
    mov esi, 128
    call net_listen
    test rax, rax
    js fail_listener

    mov rdi, r12
    lea rsi, [rel client_addr]
    lea rdx, [rel addr_len]
    mov eax, SYS_GETSOCKNAME
    syscall
    test rax, rax
    js fail_listener

    ; One untimed correctness/warm probe.
    call one_connection
    test rax, rax
    jnz fail_listener

    call bench_now_ns
    test rax, rax
    js fail_listener
    mov r13, rax
    mov r15d, ITERATIONS

.loop:
    call one_connection
    test rax, rax
    jnz fail_listener
    dec r15
    jnz .loop

    call bench_now_ns
    test rax, rax
    js fail_listener
    sub rax, r13
    mov rcx, rax

    lea rdi, [rel metric_conn]
    mov esi, metric_conn_len
    mov edx, ITERATIONS
    call bench_emit_result
    test rax, rax
    js fail_listener

    mov rdi, r12
    call net_close
    xor edi, edi
    jmp exit

; Uses persistent listener in R12.
one_connection:
    push rbx
    push r14
    sub rsp, 8

    call net_socket_tcp4
    test rax, rax
    js .return
    mov rbx, rax                  ; client

    mov rdi, rbx
    lea rsi, [rel client_addr]
    mov edx, 16
    mov eax, SYS_CONNECT
    syscall
    test rax, rax
    js .close_client_error

    mov rdi, r12
    xor esi, esi
    xor edx, edx
    mov ecx, SOCK_CLOEXEC
    call net_accept4
    test rax, rax
    js .close_client_error
    mov r14, rax                  ; accepted

    mov rdi, r14
    call net_close
    test rax, rax
    js .close_client_error

    mov rdi, rbx
    call net_close
    test rax, rax
    js .return

    xor eax, eax
    jmp .return

.close_client_error:
    mov r14, rax
    mov rdi, rbx
    call net_close
    mov rax, r14
.return:
    add rsp, 8
    pop r14
    pop rbx
    ret

fail_listener:
    mov rdi, r12
    call net_close
fail_setup:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
