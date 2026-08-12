; Arborcore Retrofit E8 experimental synchronous writev emitter
;
; iovec_write_retry_all(fd, iov*, count)
;   RAX=0 on complete emission
;   RAX=-errno on failure / EAGAIN
;   RDX=bytes emitted during this call
;
; The iovec array is mutable progress state: fully consumed entries have
; length zero; a partially consumed entry has base/length advanced.  That
; makes a subsequent call with the same original iov pointer/count resume
; correctly after EAGAIN.  Production adoption still requires connection-
; lifetime storage for the iovec array and digit scratch across readiness.

%define SYS_WRITEV 20
%define ERR_EINTR -4
%define ERR_EIO -5
%define ERR_EINVAL -22
%define ERR_EOVERFLOW -75
%define IOV_MAX 1024

global iovec_write_retry_all:function

section .text

; RDI=fd RSI=iov* RDX=count
iovec_write_retry_all:
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    xor r15d, r15d                 ; total emitted this invocation

    test r14, r14
    jz .success
    test r13, r13
    jz .invalid
    cmp r14, IOV_MAX
    ja .invalid

.next_write:
    ; Skip entries already consumed by a prior partial call.
.skip_zero:
    test r14, r14
    jz .success
    cmp qword [r13 + 8], 0
    jne .syscall
    add r13, 16
    dec r14
    jmp .skip_zero

.syscall:
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    mov eax, SYS_WRITEV
    syscall
    cmp rax, ERR_EINTR
    je .syscall
    test rax, rax
    js .error
    jz .no_progress

    add r15, rax
    jc .overflow
    mov rbx, rax

.consume:
    test rbx, rbx
    jz .next_write
    test r14, r14
    jz .invalid                    ; kernel cannot report beyond supplied iov
    mov rcx, [r13 + 8]
    test rcx, rcx
    jz .consume_full
    cmp rbx, rcx
    jb .consume_partial
    sub rbx, rcx
    mov qword [r13 + 8], 0
.consume_full:
    add r13, 16
    dec r14
    jmp .consume

.consume_partial:
    add [r13 + 0], rbx
    sub [r13 + 8], rbx
    xor ebx, ebx
    jmp .next_write

.success:
    mov rdx, r15
    xor eax, eax
    jmp .return
.invalid:
    mov rdx, r15
    mov rax, ERR_EINVAL
    jmp .return
.no_progress:
    mov rdx, r15
    mov rax, ERR_EIO
    jmp .return
.overflow:
    mov rdx, r15
    mov rax, ERR_EOVERFLOW
.error:
    mov rdx, r15
.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
