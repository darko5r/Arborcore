; Arborcore HTTP parser benchmark
; Output: metric<TAB>iterations<TAB>total_ns

%define SYS_EXIT 60
%define ITERATIONS 300000

extern http_parse_request
extern bench_now_ns
extern bench_emit_result

global _start

section .rodata
metric_minimal: db "parser_minimal"
metric_minimal_len equ $ - metric_minimal
metric_headers: db "parser_headers"
metric_headers_len equ $ - metric_headers
metric_post: db "parser_post_body"
metric_post_len equ $ - metric_post

req_minimal:
    db "GET / HTTP/1.1",13,10,13,10
req_minimal_len equ $ - req_minimal

req_headers:
    db "GET /users/42?full=1 HTTP/1.1",13,10
    db "Host: localhost",13,10
    db "User-Agent: ArborcoreBench",13,10
    db "Accept: */*",13,10
    db "Content-Length: 0",13,10
    db 13,10
req_headers_len equ $ - req_headers

req_post:
    db "POST /submit HTTP/1.1",13,10
    db "Host: localhost",13,10
    db "Content-Length: 32",13,10
    db 13,10
    db "0123456789abcdef0123456789abcdef"
req_post_len equ $ - req_post

section .bss
align 16
request_out: resb 96

section .text
_start:
    lea rdi, [rel req_minimal]
    mov esi, req_minimal_len
    lea rdx, [rel metric_minimal]
    mov ecx, metric_minimal_len
    mov r8d, ITERATIONS
    call run_case
    test rax, rax
    jnz fail

    lea rdi, [rel req_headers]
    mov esi, req_headers_len
    lea rdx, [rel metric_headers]
    mov ecx, metric_headers_len
    mov r8d, ITERATIONS
    call run_case
    test rax, rax
    jnz fail

    lea rdi, [rel req_post]
    mov esi, req_post_len
    lea rdx, [rel metric_post]
    mov ecx, metric_post_len
    mov r8d, ITERATIONS
    call run_case
    test rax, rax
    jnz fail

    xor edi, edi
    jmp exit

; run_case(request, len, metric, metric_len, iterations)
run_case:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 16

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx
    mov rbx, r8
    mov [rsp + 8], r8

    ; Correctness probe outside timed region.
    mov rdi, r12
    mov rsi, r13
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz .bad

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp], rax

.loop:
    mov rdi, r12
    mov rsi, r13
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz .bad
    dec rbx
    jnz .loop

    call bench_now_ns
    test rax, rax
    js .bad
    sub rax, [rsp]
    mov rcx, rax
    mov rdi, r14
    mov rsi, r15
    mov rdx, [rsp + 8]
    call bench_emit_result
    jmp .return

.bad:
    mov rax, -1
.return:
    add rsp, 16
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
