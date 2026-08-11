; Arborcore in-process request lifecycle benchmark
; parse -> route -> handler -> response serialization
; Output: metric<TAB>iterations<TAB>total_ns

%define SYS_EXIT 60
%define ITERATIONS 120000

extern http_parse_request
extern router_dispatch
extern route_pattern_dispatch
extern http_response_serialize
extern buffer_init
extern bench_now_ns
extern bench_emit_result

global _start

section .rodata
method_get: db "GET"
path_static: db "/static"
pattern_user: db "/users/:id"

req_static:
    db "GET /static HTTP/1.1",13,10
    db "Host: localhost",13,10
    db "Content-Length: 0",13,10
    db 13,10
req_static_len equ $ - req_static

req_param:
    db "GET /users/42 HTTP/1.1",13,10
    db "Host: localhost",13,10
    db "Content-Length: 0",13,10
    db 13,10
req_param_len equ $ - req_param

metric_exact: db "lifecycle_exact"
metric_exact_len equ $ - metric_exact
metric_param: db "lifecycle_param"
metric_param_len equ $ - metric_param

section .data
align 8
routes_exact:
    dq method_get, 3, path_static, 7, exact_handler
routes_param:
    dq method_get, 3, pattern_user, 10, param_handler
context: dq 0x1122334455667788

section .bss
align 16
request_out: resb 96
params_out: resb 128
response_buffer: resb 24
response_storage: resb 2048

section .text
_start:
    lea rdi, [rel response_buffer]
    lea rsi, [rel response_storage]
    mov edx, 2048
    call buffer_init
    test rax, rax
    jnz fail

    call run_exact
    test rax, rax
    jnz fail

    call run_param
    test rax, rax
    jnz fail

    xor edi, edi
    jmp exit

run_exact:
    push rbx
    sub rsp, 16

    ; Correctness probe.
    call exact_once
    test rax, rax
    jnz .bad

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp], rax
    mov ebx, ITERATIONS
.loop:
    call exact_once
    test rax, rax
    jnz .bad
    dec rbx
    jnz .loop
    call bench_now_ns
    test rax, rax
    js .bad
    sub rax, [rsp]
    mov rcx, rax
    lea rdi, [rel metric_exact]
    mov esi, metric_exact_len
    mov edx, ITERATIONS
    call bench_emit_result
    jmp .return
.bad:
    mov rax, -1
.return:
    add rsp, 16
    pop rbx
    ret

run_param:
    push rbx
    sub rsp, 16

    call param_once
    test rax, rax
    jnz .bad

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp], rax
    mov ebx, ITERATIONS
.loop:
    call param_once
    test rax, rax
    jnz .bad
    dec rbx
    jnz .loop
    call bench_now_ns
    test rax, rax
    js .bad
    sub rax, [rsp]
    mov rcx, rax
    lea rdi, [rel metric_param]
    mov esi, metric_param_len
    mov edx, ITERATIONS
    call bench_emit_result
    jmp .return
.bad:
    mov rax, -1
.return:
    add rsp, 16
    pop rbx
    ret

exact_once:
    sub rsp, 8
    lea rdi, [rel req_static]
    mov esi, req_static_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz .done

    lea rdi, [rel routes_exact]
    mov esi, 1
    lea rdx, [rel request_out]
    lea rcx, [rel context]
    call router_dispatch
    cmp rax, 200
    jne .error

    mov qword [rel response_buffer + 8], 0
    lea rdi, [rel response_buffer]
    mov esi, 200
    xor edx, edx
    xor ecx, ecx
    mov r8d, 1
    call http_response_serialize
    jmp .done
.error:
    mov rax, -1
.done:
    add rsp, 8
    ret

param_once:
    sub rsp, 8
    lea rdi, [rel req_param]
    mov esi, req_param_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz .done

    lea rdi, [rel routes_param]
    mov esi, 1
    lea rdx, [rel request_out]
    lea rcx, [rel context]
    lea r8, [rel params_out]
    mov r9d, 4
    call route_pattern_dispatch
    cmp rax, 200
    jne .error

    mov qword [rel response_buffer + 8], 0
    lea rdi, [rel response_buffer]
    mov esi, 200
    xor edx, edx
    xor ecx, ecx
    mov r8d, 1
    call http_response_serialize
    jmp .done
.error:
    mov rax, -1
.done:
    add rsp, 8
    ret

exact_handler:
    mov eax, 200
    ret
param_handler:
    mov eax, 200
    ret

fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
