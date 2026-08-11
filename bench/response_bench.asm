; Arborcore HTTP response serialization benchmark
; Output: metric<TAB>iterations<TAB>total_ns

%define SYS_EXIT 60
%define ITERATIONS 250000

extern http_response_serialize
extern buffer_init
extern bench_now_ns
extern bench_emit_result

global _start

section .rodata
metric_200_empty: db "response_200_empty"
metric_200_empty_len equ $ - metric_200_empty
metric_404_empty: db "response_404_empty"
metric_404_empty_len equ $ - metric_404_empty
metric_200_128: db "response_200_body128"
metric_200_128_len equ $ - metric_200_128
metric_200_1024: db "response_200_body1024"
metric_200_1024_len equ $ - metric_200_1024
body128: times 128 db 'A'
body1024: times 1024 db 'B'

section .bss
align 16
buffer: resb 24
storage: resb 2048

section .text
_start:
    lea rdi, [rel buffer]
    lea rsi, [rel storage]
    mov edx, 2048
    call buffer_init
    test rax, rax
    jnz fail

    mov esi, 200
    xor edx, edx
    xor ecx, ecx
    lea r8, [rel metric_200_empty]
    mov r9d, metric_200_empty_len
    call run_case
    test rax, rax
    jnz fail

    mov esi, 404
    xor edx, edx
    xor ecx, ecx
    lea r8, [rel metric_404_empty]
    mov r9d, metric_404_empty_len
    call run_case
    test rax, rax
    jnz fail

    mov esi, 200
    lea rdx, [rel body128]
    mov ecx, 128
    lea r8, [rel metric_200_128]
    mov r9d, metric_200_128_len
    call run_case
    test rax, rax
    jnz fail

    mov esi, 200
    lea rdx, [rel body1024]
    mov ecx, 1024
    lea r8, [rel metric_200_1024]
    mov r9d, metric_200_1024_len
    call run_case
    test rax, rax
    jnz fail

    xor edi, edi
    jmp exit

; run_case(status, body, body_len, metric, metric_len)
; Input intentionally shifted because RDI is unused by caller:
; RSI=status, RDX=body, RCX=body_len, R8=metric, R9=metric_len
run_case:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 32

    mov r12, rsi                 ; status
    mov r13, rdx                 ; body
    mov r14, rcx                 ; body length
    mov r15, r8                  ; metric
    mov [rsp + 0], r9            ; metric length
    mov qword [rsp + 8], ITERATIONS

    ; Correctness probe.
    mov qword [rel buffer + 8], 0
    lea rdi, [rel buffer]
    mov rsi, r12
    mov rdx, r13
    mov rcx, r14
    mov r8d, 1
    call http_response_serialize
    test rax, rax
    jnz .bad

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp + 16], rax
    mov ebx, ITERATIONS

.loop:
    mov qword [rel buffer + 8], 0
    lea rdi, [rel buffer]
    mov rsi, r12
    mov rdx, r13
    mov rcx, r14
    mov r8d, 1
    call http_response_serialize
    dec rbx
    jnz .loop

    call bench_now_ns
    test rax, rax
    js .bad
    sub rax, [rsp + 16]
    mov rcx, rax
    mov rdi, r15
    mov rsi, [rsp + 0]
    mov rdx, [rsp + 8]
    call bench_emit_result
    jmp .return
.bad:
    mov rax, -1
.return:
    add rsp, 32
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
