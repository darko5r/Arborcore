; Arborcore Linux I/O Polish Gate #2 tests
; exit 0 pass, 1 fail

%define SYS_EXIT     60
%define SYS_FCNTL    72
%define SYS_PIPE2   293

%define F_GETFL       3
%define O_NONBLOCK 0x800

%define ERR_EBADF    -9
%define ERR_EAGAIN  -11

extern io_read_retry
extern io_write_retry
extern io_close
extern io_set_nonblocking

global _start

section .rodata
message: db "io-gate2"
message_len equ $ - message

section .bss
align 8
pipefd:  resd 2
readbuf: resb 32

section .text
_start:
    ; Zero-length operations are syscall-free even with invalid FDs.
    mov edi, -1
    xor esi, esi
    xor edx, edx
    call io_read_retry
    test rax, rax
    jnz io_test_fail

    mov edi, -1
    xor esi, esi
    xor edx, edx
    call io_write_retry
    test rax, rax
    jnz io_test_fail

    ; Nonzero operations propagate EBADF.
    mov edi, -1
    lea rsi, [rel readbuf]
    mov edx, 1
    call io_read_retry
    cmp rax, ERR_EBADF
    jne io_test_fail

    mov edi, -1
    lea rsi, [rel message]
    mov edx, 1
    call io_write_retry
    cmp rax, ERR_EBADF
    jne io_test_fail

    mov edi, -1
    call io_set_nonblocking
    cmp rax, ERR_EBADF
    jne io_test_fail

    mov edi, -1
    call io_close
    cmp rax, ERR_EBADF
    jne io_test_fail

    ; Build a local pipe.
    lea rdi, [rel pipefd]
    xor esi, esi
    mov eax, SYS_PIPE2
    syscall
    test rax, rax
    js io_test_fail

    ; One successful write and read.
    mov edi, [rel pipefd + 4]
    lea rsi, [rel message]
    mov edx, message_len
    call io_write_retry
    cmp rax, message_len
    jne io_test_close_fail

    mov edi, [rel pipefd]
    lea rsi, [rel readbuf]
    mov edx, 32
    call io_read_retry
    cmp rax, message_len
    jne io_test_close_fail

    lea r8, [rel message]
    lea r9, [rel readbuf]
    xor ecx, ecx
io_test_compare:
    cmp ecx, message_len
    jae io_test_compare_done
    mov al, [r8 + rcx]
    cmp al, [r9 + rcx]
    jne io_test_close_fail
    inc ecx
    jmp io_test_compare
io_test_compare_done:

    ; Record existing status flags.
    mov edi, [rel pipefd]
    mov eax, SYS_FCNTL
    mov esi, F_GETFL
    xor edx, edx
    syscall
    test rax, rax
    js io_test_close_fail
    mov r12, rax

    ; Set O_NONBLOCK while preserving the existing status flags.
    mov edi, [rel pipefd]
    call io_set_nonblocking
    test rax, rax
    jnz io_test_close_fail

    mov edi, [rel pipefd]
    mov eax, SYS_FCNTL
    mov esi, F_GETFL
    xor edx, edx
    syscall
    test rax, rax
    js io_test_close_fail
    mov r13, r12
    or r13, O_NONBLOCK
    cmp rax, r13
    jne io_test_close_fail

    ; Empty nonblocking read returns EAGAIN; it is not busy-retried.
    mov edi, [rel pipefd]
    lea rsi, [rel readbuf]
    mov edx, 1
    call io_read_retry
    cmp rax, ERR_EAGAIN
    jne io_test_close_fail

    ; Close both descriptors.
    mov edi, [rel pipefd]
    call io_close
    test rax, rax
    jnz io_test_close_write_fail
    mov dword [rel pipefd], -1

    mov edi, [rel pipefd + 4]
    call io_close
    test rax, rax
    jnz io_test_fail
    mov dword [rel pipefd + 4], -1

    xor edi, edi
    jmp io_test_exit

io_test_close_fail:
    mov edi, [rel pipefd]
    call io_close
    mov dword [rel pipefd], -1

io_test_close_write_fail:
    mov edi, [rel pipefd + 4]
    call io_close
    mov dword [rel pipefd + 4], -1

io_test_fail:
    mov edi, 1

io_test_exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
