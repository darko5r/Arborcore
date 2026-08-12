; Arborcore Retrofit E1/E5 accept transaction + pristine reuse qualification
%define SYS_EXIT 60
%define SYS_CONNECT 42
%define SYS_GETSOCKNAME 51
%define SYS_FCNTL 72
%define F_GETFD 1
%define ERR_EBADF -9
%define CLOSED 8

extern buffer_init
extern arena_init
extern net_socket_tcp4
extern net_close
extern server_open_listener
extern server_accept_connection

global _start

section .data
align 8
listener_addr:
    dw 2
    dw 0
    dd 0x0100007f
    dq 0
client_addr: times 16 db 0
addr_len: dd 16

section .bss
alignb 16
inbuf: resb 24
outbuf: resb 24
in_storage: resb 256
out_storage: resb 256
arena: resb 24
arena_storage: resb 128
conn: resb 96

section .text
_start:
    mov r12, -1
    mov r13, -1

    lea rdi, [rel inbuf]
    lea rsi, [rel in_storage]
    mov edx, 256
    call buffer_init
    test rax, rax
    jnz fail
    lea rdi, [rel outbuf]
    lea rsi, [rel out_storage]
    mov edx, 256
    call buffer_init
    test rax, rax
    jnz fail
    lea rdi, [rel arena]
    lea rsi, [rel arena_storage]
    mov edx, 128
    call arena_init
    test rax, rax
    jnz fail

    ; Deliberately dirty reusable storage. Accept preparation must reset it.
    mov qword [rel inbuf + 8], 7
    mov qword [rel outbuf + 8], 9
    mov qword [rel arena + 16], 11

    lea rdi, [rel listener_addr]
    mov esi, 16
    mov edx, 8
    call server_open_listener
    test rax, rax
    js fail
    mov r12, rax

    mov rdi, r12
    lea rsi, [rel client_addr]
    lea rdx, [rel addr_len]
    mov eax, SYS_GETSOCKNAME
    syscall
    test rax, rax
    js cleanup_fail

    call net_socket_tcp4
    test rax, rax
    js cleanup_fail
    mov r13, rax
    mov rdi, r13
    lea rsi, [rel client_addr]
    mov edx, 16
    mov eax, SYS_CONNECT
    syscall
    test rax, rax
    js cleanup_fail

    ; Invalid epfd forces failure AFTER accept+connection_init.  The accepted
    ; fd must be closed and the connection rolled back to CLOSED.
    mov rdi, r12
    mov rsi, -1
    lea rdx, [rel conn]
    lea rcx, [rel inbuf]
    lea r8, [rel outbuf]
    lea r9, [rel arena]
    call server_accept_connection
    cmp rax, ERR_EBADF
    jne cleanup_fail
    cmp qword [rel conn + 8], CLOSED
    jne cleanup_fail
    cmp qword [rel inbuf + 8], 0
    jne cleanup_fail
    cmp qword [rel outbuf + 8], 0
    jne cleanup_fail
    cmp qword [rel arena + 16], 0
    jne cleanup_fail
    cmp qword [rel conn + 80], 0
    jne cleanup_fail
    cmp qword [rel conn + 88], 0
    jne cleanup_fail

    mov rdi, [rel conn + 0]
    mov esi, F_GETFD
    xor edx, edx
    mov eax, SYS_FCNTL
    syscall
    cmp rax, ERR_EBADF
    jne cleanup_fail

    xor r14d, r14d
    jmp cleanup
cleanup_fail:
    mov r14d, 1
cleanup:
    cmp r13, 0
    jl .listener
    mov rdi, r13
    call net_close
.listener:
    cmp r12, 0
    jl .done
    mov rdi, r12
    call net_close
.done:
    mov edi, r14d
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
