; Retrofit E8 experimental iovec response preparation (status 200 only)
%define ERR_EINVAL -22
extern u64_format_decimal

global response_iovec_prepare_200:function

section .rodata
status: db "HTTP/1.1 200 OK",13,10
status_len equ $ - status
cl_prefix: db "Content-Length: "
cl_prefix_len equ $ - cl_prefix
crlf: db 13,10
keep: db "Connection: keep-alive",13,10
keep_len equ $ - keep
close: db "Connection: close",13,10
close_len equ $ - close

section .text
; RDI=iov[7], RSI=20-byte digit scratch, RDX=body, RCX=body_len, R8=keepalive
; Returns RAX=0, RDX=7 on success.
response_iovec_prepare_200:
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx
    mov rbx, r8
    cmp rbx, 1
    ja .invalid
    test r15, r15
    jz .body_ok
    test r14, r14
    jz .invalid
.body_ok:
    mov rdi, r15
    mov rsi, r13
    mov edx, 20
    call u64_format_decimal
    test rax, rax
    jnz .return
    mov r11, rdx

    lea rax, [rel status]
    mov [r12 + 0], rax
    mov qword [r12 + 8], status_len
    lea rax, [rel cl_prefix]
    mov [r12 + 16], rax
    mov qword [r12 + 24], cl_prefix_len
    mov [r12 + 32], r13
    mov [r12 + 40], r11
    lea rax, [rel crlf]
    mov [r12 + 48], rax
    mov qword [r12 + 56], 2
    test rbx, rbx
    jz .close
    lea rax, [rel keep]
    mov [r12 + 64], rax
    mov qword [r12 + 72], keep_len
    jmp .conn_done
.close:
    lea rax, [rel close]
    mov [r12 + 64], rax
    mov qword [r12 + 72], close_len
.conn_done:
    lea rax, [rel crlf]
    mov [r12 + 80], rax
    mov qword [r12 + 88], 2
    mov [r12 + 96], r14
    mov [r12 + 104], r15
    mov edx, 7
    xor eax, eax
    jmp .return
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
section .note.GNU-stack noalloc noexec nowrite progbits
