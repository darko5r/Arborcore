; Arborcore routing benchmark
; Output: metric<TAB>iterations<TAB>total_ns

%define SYS_EXIT 60
%define EXACT_ITERATIONS 800000
%define PARAM_ITERATIONS 400000

extern router_find_exact
extern route_pattern_match
extern bench_now_ns
extern bench_emit_result

global _start

section .rodata
method_get: db "GET"
method_get_len equ $ - method_get

path0:  db "/r0"
path1:  db "/r1"
path2:  db "/r2"
path3:  db "/r3"
path4:  db "/r4"
path5:  db "/r5"
path6:  db "/r6"
path7:  db "/r7"
path8:  db "/r8"
path9:  db "/r9"
path10: db "/r10"
path11: db "/r11"
path12: db "/r12"
path13: db "/r13"
path14: db "/r14"
path15: db "/target"
path_missing: db "/missing"

pattern_param: db "/users/:id/posts/:post"
pattern_param_len equ $ - pattern_param
path_param: db "/users/42/posts/9001"
path_param_len equ $ - path_param

metric_first: db "router_exact_first"
metric_first_len equ $ - metric_first
metric_last: db "router_exact_last"
metric_last_len equ $ - metric_last
metric_miss: db "router_exact_miss"
metric_miss_len equ $ - metric_miss
metric_param: db "router_param_two"
metric_param_len equ $ - metric_param

section .data
align 8
routes:
    dq method_get, method_get_len, path0, 3, bench_handler
    dq method_get, method_get_len, path1, 3, bench_handler
    dq method_get, method_get_len, path2, 3, bench_handler
    dq method_get, method_get_len, path3, 3, bench_handler
    dq method_get, method_get_len, path4, 3, bench_handler
    dq method_get, method_get_len, path5, 3, bench_handler
    dq method_get, method_get_len, path6, 3, bench_handler
    dq method_get, method_get_len, path7, 3, bench_handler
    dq method_get, method_get_len, path8, 3, bench_handler
    dq method_get, method_get_len, path9, 3, bench_handler
    dq method_get, method_get_len, path10, 4, bench_handler
    dq method_get, method_get_len, path11, 4, bench_handler
    dq method_get, method_get_len, path12, 4, bench_handler
    dq method_get, method_get_len, path13, 4, bench_handler
    dq method_get, method_get_len, path14, 4, bench_handler
    dq method_get, method_get_len, path15, 7, bench_handler
route_count equ 16

section .bss
align 16
params_out: resb 128

section .text
_start:
    lea rdi, [rel path0]
    mov esi, 3
    lea rdx, [rel metric_first]
    mov ecx, metric_first_len
    mov r8d, 1                    ; expected found
    call run_exact
    test rax, rax
    jnz fail

    lea rdi, [rel path15]
    mov esi, 7
    lea rdx, [rel metric_last]
    mov ecx, metric_last_len
    mov r8d, 1
    call run_exact
    test rax, rax
    jnz fail

    lea rdi, [rel path_missing]
    mov esi, 8
    lea rdx, [rel metric_miss]
    mov ecx, metric_miss_len
    xor r8d, r8d                  ; expected missing
    call run_exact
    test rax, rax
    jnz fail

    call run_param
    test rax, rax
    jnz fail

    xor edi, edi
    jmp exit

; run_exact(path, path_len, metric, metric_len, expected_found)
run_exact:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 32

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx
    mov [rsp + 0], r8
    mov qword [rsp + 8], EXACT_ITERATIONS

    ; Correctness probe outside timed region.
    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel method_get]
    mov ecx, method_get_len
    mov r8, r12
    mov r9, r13
    call router_find_exact
    mov rcx, [rsp + 0]
    test rcx, rcx
    jz .expect_missing
    test rax, rax
    jz .bad
    jmp .probe_ok
.expect_missing:
    test rax, rax
    jnz .bad
.probe_ok:

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp + 16], rax
    mov rbx, [rsp + 8]

.loop:
    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel method_get]
    mov ecx, method_get_len
    mov r8, r12
    mov r9, r13
    call router_find_exact
    dec rbx
    jnz .loop

    call bench_now_ns
    test rax, rax
    js .bad
    sub rax, [rsp + 16]
    mov rcx, rax
    mov rdi, r14
    mov rsi, r15
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

run_param:
    push rbx
    sub rsp, 16

    ; Correctness probe.
    lea rdi, [rel pattern_param]
    mov esi, pattern_param_len
    lea rdx, [rel path_param]
    mov ecx, path_param_len
    lea r8, [rel params_out]
    mov r9d, 4
    call route_pattern_match
    cmp rax, 1
    jne .bad
    cmp rdx, 2
    jne .bad

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp], rax
    mov ebx, PARAM_ITERATIONS

.loop:
    lea rdi, [rel pattern_param]
    mov esi, pattern_param_len
    lea rdx, [rel path_param]
    mov ecx, path_param_len
    lea r8, [rel params_out]
    mov r9d, 4
    call route_pattern_match
    dec rbx
    jnz .loop

    call bench_now_ns
    test rax, rax
    js .bad
    sub rax, [rsp]
    mov rcx, rax
    lea rdi, [rel metric_param]
    mov esi, metric_param_len
    mov edx, PARAM_ITERATIONS
    call bench_emit_result
    jmp .return
.bad:
    mov rax, -1
.return:
    add rsp, 16
    pop rbx
    ret

bench_handler:
    mov eax, 200
    ret

fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
