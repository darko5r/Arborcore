; Arborcore Retrofit E8 response preparation experiment
%define SYS_EXIT 60
%define SYS_OPEN 2
%define SYS_CLOSE 3
%define O_WRONLY 1
%define ITERATIONS 200000
extern response_iovec_prepare_200
extern iovec_write_retry_all
extern write_all
extern http_response_serialize
extern buffer_init
extern buffer_reset
extern bench_now_ns
extern bench_emit_result

global _start

section .rodata
devnull: db "/dev/null",0
body128: times 128 db 'x'
body1024: times 1024 db 'y'
m_ser_empty: db "serialize_empty"
m_ser_empty_len equ $-m_ser_empty
m_iov_empty: db "iovec_empty"
m_iov_empty_len equ $-m_iov_empty
m_ser_128: db "serialize_body128"
m_ser_128_len equ $-m_ser_128
m_iov_128: db "iovec_body128"
m_iov_128_len equ $-m_iov_128
m_ser_1024: db "serialize_body1024"
m_ser_1024_len equ $-m_ser_1024
m_iov_1024: db "iovec_body1024"
m_iov_1024_len equ $-m_iov_1024

section .bss
alignb 16
iov: resb 112
digits: resb 20
buf: resb 24
storage: resb 2048
devnull_fd: resq 1

section .text
_start:
    lea rdi,[rel devnull]
    mov esi,O_WRONLY
    xor edx,edx
    mov eax,SYS_OPEN
    syscall
    test rax,rax
    js fail
    mov [rel devnull_fd],rax

    lea rdi,[rel buf]
    lea rsi,[rel storage]
    mov edx,2048
    call buffer_init
    test rax,rax
    jnz fail

    xor edx,edx
    xor ecx,ecx
    lea r8,[rel m_ser_empty]
    mov r9d,m_ser_empty_len
    call run_serialize
    test rax,rax
    jnz fail
    xor edx,edx
    xor ecx,ecx
    lea r8,[rel m_iov_empty]
    mov r9d,m_iov_empty_len
    call run_iovec
    test rax,rax
    jnz fail

    lea rdx,[rel body128]
    mov ecx,128
    lea r8,[rel m_ser_128]
    mov r9d,m_ser_128_len
    call run_serialize
    test rax,rax
    jnz fail
    lea rdx,[rel body128]
    mov ecx,128
    lea r8,[rel m_iov_128]
    mov r9d,m_iov_128_len
    call run_iovec
    test rax,rax
    jnz fail

    lea rdx,[rel body1024]
    mov ecx,1024
    lea r8,[rel m_ser_1024]
    mov r9d,m_ser_1024_len
    call run_serialize
    test rax,rax
    jnz fail
    lea rdx,[rel body1024]
    mov ecx,1024
    lea r8,[rel m_iov_1024]
    mov r9d,m_iov_1024_len
    call run_iovec
    test rax,rax
    jnz fail

    xor r12d,r12d
    jmp close_file

; RDX body RCX len R8 metric R9 metric_len
run_serialize:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp,16
    mov r12,rdx
    mov r13,rcx
    mov r14,r8
    mov r15,r9
    call bench_now_ns
    test rax,rax
    js .bad
    mov [rsp],rax
    mov ebx,ITERATIONS
.loop:
    lea rdi,[rel buf]
    call buffer_reset
    lea rdi,[rel buf]
    mov esi,200
    mov rdx,r12
    mov rcx,r13
    mov r8d,1
    call http_response_serialize
    test rax,rax
    jnz .bad
    mov rdi,[rel devnull_fd]
    lea rsi,[rel storage]
    mov rdx,[rel buf + 8]
    call write_all
    test rax,rax
    jnz .bad
    dec ebx
    jnz .loop
    call bench_now_ns
    test rax,rax
    js .bad
    sub rax,[rsp]
    mov rcx,rax
    mov rdi,r14
    mov rsi,r15
    mov edx,ITERATIONS
    call bench_emit_result
    jmp .ret
.bad:
    mov rax,-1
.ret:
    add rsp,16
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

run_iovec:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp,16
    mov r12,rdx
    mov r13,rcx
    mov r14,r8
    mov r15,r9
    call bench_now_ns
    test rax,rax
    js .bad
    mov [rsp],rax
    mov ebx,ITERATIONS
.loop:
    lea rdi,[rel iov]
    lea rsi,[rel digits]
    mov rdx,r12
    mov rcx,r13
    mov r8d,1
    call response_iovec_prepare_200
    test rax,rax
    jnz .bad
    mov rdi,[rel devnull_fd]
    lea rsi,[rel iov]
    mov edx,7
    call iovec_write_retry_all
    test rax,rax
    jnz .bad
    dec ebx
    jnz .loop
    call bench_now_ns
    test rax,rax
    js .bad
    sub rax,[rsp]
    mov rcx,rax
    mov rdi,r14
    mov rsi,r15
    mov edx,ITERATIONS
    call bench_emit_result
    jmp .ret
.bad:
    mov rax,-1
.ret:
    add rsp,16
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

fail:
    mov r12d,1
close_file:
    mov rdi,[rel devnull_fd]
    mov eax,SYS_CLOSE
    syscall
    mov edi,r12d
exit:
    mov eax,SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
