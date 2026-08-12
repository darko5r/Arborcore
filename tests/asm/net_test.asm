; Arborcore TCP socket Polish Gate #2 tests
; Local loopback only; no external network access.
; exit 0 pass, 1 fail

%define SYS_EXIT         60
%define SYS_CONNECT      42
%define SYS_GETSOCKNAME  51
%define SYS_FCNTL        72

%define F_GETFD           1
%define F_GETFL           3
%define FD_CLOEXEC         1
%define SOCK_CLOEXEC  0x80000
%define O_NONBLOCK    0x800
%define SOCK_NONBLOCK 0x800
%define SHUT_RDWR          2

%define ERR_EBADF         -9
%define ERR_EAGAIN       -11

extern net_socket_tcp4
extern net_socket_tcp4_flags
extern net_bind
extern net_listen
extern net_accept4
extern net_shutdown
extern net_close
extern io_set_nonblocking

global _start

section .data
align 8
sockaddr4:
    dw 2                         ; AF_INET
    dw 0                         ; kernel selects ephemeral port
    db 127, 0, 0, 1             ; 127.0.0.1
    dq 0
sockaddr4_len equ $ - sockaddr4

sockaddr4_actual_len:
    dd sockaddr4_len

section .text
_start:
    mov r12, -1                  ; listener
    mov r13, -1                  ; client
    mov r14, -1                  ; accepted

    ; Invalid descriptors propagate EBADF.
    mov edi, -1
    lea rsi, [rel sockaddr4]
    mov edx, sockaddr4_len
    call net_bind
    cmp rax, ERR_EBADF
    jne net_test_fail

    mov edi, -1
    mov esi, 8
    call net_listen
    cmp rax, ERR_EBADF
    jne net_test_fail

    mov edi, -1
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call net_accept4
    cmp rax, ERR_EBADF
    jne net_test_fail

    mov edi, -1
    xor esi, esi
    call net_shutdown
    cmp rax, ERR_EBADF
    jne net_test_fail

    mov edi, -1
    call net_close
    cmp rax, ERR_EBADF
    jne net_test_fail

    ; Linux socket type/flag input is an int domain: high transport bits are
    ; rejected rather than silently truncated through EDI.
    mov rdi, 0x100000000
    call net_socket_tcp4_flags
    cmp rax, -22
    jne net_test_fail

    ; Atomic listener-style socket creation qualifies NONBLOCK+CLOEXEC.
    mov edi, SOCK_NONBLOCK | SOCK_CLOEXEC
    call net_socket_tcp4_flags
    test rax, rax
    js net_test_fail
    mov r15, rax
    mov rdi, r15
    mov eax, SYS_FCNTL
    mov esi, F_GETFL
    xor edx, edx
    syscall
    test eax, O_NONBLOCK
    jz net_test_fail_atomic
    mov rdi, r15
    mov eax, SYS_FCNTL
    mov esi, F_GETFD
    xor edx, edx
    syscall
    test eax, FD_CLOEXEC
    jz net_test_fail_atomic
    mov rdi, r15
    call net_close
    test rax, rax
    jnz net_test_fail
    mov r15, -1

    ; Listener on loopback with kernel-assigned port.
    call net_socket_tcp4
    test rax, rax
    js net_test_fail
    mov r12, rax

    mov rdi, r12
    lea rsi, [rel sockaddr4]
    mov edx, sockaddr4_len
    call net_bind
    test rax, rax
    jnz net_test_cleanup

    mov rdi, r12
    mov esi, 8
    call net_listen
    test rax, rax
    jnz net_test_cleanup

    ; Read back the actual ephemeral port.
    mov rdi, r12
    lea rsi, [rel sockaddr4]
    lea rdx, [rel sockaddr4_actual_len]
    mov eax, SYS_GETSOCKNAME
    syscall
    test rax, rax
    js net_test_cleanup
    cmp dword [rel sockaddr4_actual_len], sockaddr4_len
    jne net_test_cleanup
    cmp word [rel sockaddr4 + 2], 0
    je net_test_cleanup

    ; Create a local client and connect it to the listener.
    call net_socket_tcp4
    test rax, rax
    js net_test_cleanup
    mov r13, rax

    mov rdi, r13
    lea rsi, [rel sockaddr4]
    mov edx, sockaddr4_len
    mov eax, SYS_CONNECT
    syscall
    test rax, rax
    jnz net_test_cleanup

    ; Successful accept with NONBLOCK+CLOEXEC requested atomically.
    mov rdi, r12
    xor esi, esi
    xor edx, edx
    mov ecx, SOCK_NONBLOCK | SOCK_CLOEXEC
    call net_accept4
    test rax, rax
    js net_test_cleanup
    mov r14, rax

    ; Accepted descriptor really is nonblocking.
    mov rdi, r14
    mov eax, SYS_FCNTL
    mov esi, F_GETFL
    xor edx, edx
    syscall
    test rax, rax
    js net_test_cleanup
    test eax, O_NONBLOCK
    jz net_test_cleanup
    mov rdi, r14
    mov eax, SYS_FCNTL
    mov esi, F_GETFD
    xor edx, edx
    syscall
    test rax, rax
    js net_test_cleanup
    test eax, FD_CLOEXEC
    jz net_test_cleanup

    ; Connected socket shutdown wrapper succeeds.
    mov rdi, r14
    mov esi, SHUT_RDWR
    call net_shutdown
    test rax, rax
    jnz net_test_cleanup

    mov rdi, r14
    call net_close
    test rax, rax
    jnz net_test_cleanup
    mov r14, -1

    mov rdi, r13
    call net_close
    test rax, rax
    jnz net_test_cleanup
    mov r13, -1

    ; With no pending client, a nonblocking listener returns EAGAIN.
    mov rdi, r12
    call io_set_nonblocking
    test rax, rax
    jnz net_test_cleanup

    mov rdi, r12
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call net_accept4
    cmp rax, ERR_EAGAIN
    jne net_test_cleanup

    mov rdi, r12
    call net_close
    test rax, rax
    jnz net_test_fail
    mov r12, -1

    xor edi, edi
    jmp net_test_exit

net_test_fail_atomic:
    mov rdi, r15
    call net_close
    mov r15, -1
    jmp net_test_fail

net_test_cleanup:
    cmp r14, 0
    jl net_test_cleanup_client
    mov rdi, r14
    call net_close
    mov r14, -1

net_test_cleanup_client:
    cmp r13, 0
    jl net_test_cleanup_listener
    mov rdi, r13
    call net_close
    mov r13, -1

net_test_cleanup_listener:
    cmp r12, 0
    jl net_test_fail
    mov rdi, r12
    call net_close
    mov r12, -1

net_test_fail:
    mov edi, 1

net_test_exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
