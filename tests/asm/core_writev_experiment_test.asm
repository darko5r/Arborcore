; Retrofit E8 iovec-preparation equivalence experiment
%define SYS_EXIT 60
%define SYS_OPEN 2
%define SYS_CLOSE 3
%define O_WRONLY 1
extern response_iovec_prepare_200
extern iovec_write_retry_all
extern http_response_serialize
extern buffer_init
extern memory_copy

global _start

section .rodata
devnull: db "/dev/null",0
body: times 128 db 'x'
section .bss
alignb 16
iov: resb 112
digits: resb 20
flat: resb 512
serialized: resb 512
buf: resb 24
devnull_fd: resq 1

section .text
_start:
    lea rdi, [rel devnull]
    mov esi, O_WRONLY
    xor edx, edx
    mov eax, SYS_OPEN
    syscall
    test rax, rax
    js fail
    mov [rel devnull_fd], rax

    xor r14d, r14d
.case_loop:
    cmp r14d, 2
    jae pass
    lea rdi, [rel iov]
    lea rsi, [rel digits]
    xor edx, edx
    xor ecx, ecx
    test r14d, r14d
    jz .prep
    lea rdx, [rel body]
    mov ecx, 128
.prep:
    mov r8d, 1
    call response_iovec_prepare_200
    test rax, rax
    jnz fail
    call flatten
    mov r15, rax

    lea rdi, [rel buf]
    lea rsi, [rel serialized]
    mov edx, 512
    call buffer_init
    test rax, rax
    jnz fail
    lea rdi, [rel buf]
    mov esi, 200
    xor edx, edx
    xor ecx, ecx
    test r14d, r14d
    jz .serialize
    lea rdx, [rel body]
    mov ecx, 128
.serialize:
    mov r8d, 1
    call http_response_serialize
    test rax, rax
    jnz fail
    cmp rdx, r15
    jne fail
    lea rdi, [rel flat]
    lea rsi, [rel serialized]
    mov rdx, r15
    call compare
    test eax, eax
    jz fail_close

    ; Exercise the real writev syscall path.  The experimental emitter mutates
    ; iovec progress state and must report exactly the serialized byte count.
    mov rdi, [rel devnull_fd]
    lea rsi, [rel iov]
    mov edx, 7
    call iovec_write_retry_all
    test rax, rax
    jnz fail_close
    cmp rdx, r15
    jne fail_close

    inc r14d
    jmp .case_loop

flatten:
    push rbx
    push r12
    push r13
    xor r12d, r12d
    xor r13d, r13d
.loop:
    cmp r12d, 7
    jae .done
    lea r8, [rel iov]
    mov rax, r12
    shl rax, 4
    mov rsi, [r8 + rax + 0]
    mov rdx, [r8 + rax + 8]
    test rdx, rdx
    jz .next
    lea rdi, [rel flat]
    add rdi, r13
    mov rbx, rdx
    call memory_copy
    add r13, rbx
.next:
    inc r12d
    jmp .loop
.done:
    mov rax, r13
    pop r13
    pop r12
    pop rbx
    ret

compare:
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
pass:
    xor r14d, r14d
    jmp close_file
fail_close:
    mov r14d, 1
close_file:
    mov rdi, [rel devnull_fd]
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
