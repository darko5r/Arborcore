; Arborcore Retrofit D4 prepared exact-route-index benchmark
; Output: metric<TAB>iterations<TAB>total_ns

%define SYS_EXIT 60
%define ITERATIONS 800000

extern router_find_exact
extern route_index_prepare
extern route_index_find
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

metric_linear_first: db "linear_first"
metric_linear_first_len equ $ - metric_linear_first
metric_index_first: db "index_first"
metric_index_first_len equ $ - metric_index_first
metric_linear_last: db "linear_last"
metric_linear_last_len equ $ - metric_linear_last
metric_index_last: db "index_last"
metric_index_last_len equ $ - metric_index_last
metric_linear_miss: db "linear_miss"
metric_linear_miss_len equ $ - metric_linear_miss
metric_index_miss: db "index_miss"
metric_index_miss_len equ $ - metric_index_miss

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
alignb 16
slots: resq 32

section .text
_start:
    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel slots]
    mov ecx, 32
    call route_index_prepare
    test rax, rax
    jnz fail

    lea rdi, [rel path0]
    mov esi, 3
    lea rdx, [rel metric_linear_first]
    mov ecx, metric_linear_first_len
    mov r8d, 1
    call run_linear
    test rax, rax
    jnz fail

    lea rdi, [rel path0]
    mov esi, 3
    lea rdx, [rel metric_index_first]
    mov ecx, metric_index_first_len
    mov r8d, 1
    call run_index
    test rax, rax
    jnz fail

    lea rdi, [rel path15]
    mov esi, 7
    lea rdx, [rel metric_linear_last]
    mov ecx, metric_linear_last_len
    mov r8d, 1
    call run_linear
    test rax, rax
    jnz fail

    lea rdi, [rel path15]
    mov esi, 7
    lea rdx, [rel metric_index_last]
    mov ecx, metric_index_last_len
    mov r8d, 1
    call run_index
    test rax, rax
    jnz fail

    lea rdi, [rel path_missing]
    mov esi, 8
    lea rdx, [rel metric_linear_miss]
    mov ecx, metric_linear_miss_len
    xor r8d, r8d
    call run_linear
    test rax, rax
    jnz fail

    lea rdi, [rel path_missing]
    mov esi, 8
    lea rdx, [rel metric_index_miss]
    mov ecx, metric_index_miss_len
    xor r8d, r8d
    call run_index
    test rax, rax
    jnz fail

    xor edi, edi
    jmp exit

; RDI path, RSI len, RDX metric ptr, RCX metric len, R8 expected found
run_linear:
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

    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel method_get]
    mov ecx, method_get_len
    mov r8, r12
    mov r9, r13
    call router_find_exact
    mov rcx, [rsp + 0]
    test rcx, rcx
    jz .linear_expect_miss
    test rax, rax
    jz .bad
    jmp .linear_probe_ok
.linear_expect_miss:
    test rax, rax
    jnz .bad
.linear_probe_ok:

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp + 8], rax
    mov ebx, ITERATIONS
.loop:
    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel method_get]
    mov ecx, method_get_len
    mov r8, r12
    mov r9, r13
    call router_find_exact
    dec ebx
    jnz .loop

    call bench_now_ns
    test rax, rax
    js .bad
    sub rax, [rsp + 8]
    mov rcx, rax
    mov rdi, r14
    mov rsi, r15
    mov edx, ITERATIONS
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

run_index:
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

    lea rdi, [rel slots]
    mov esi, 32
    lea rdx, [rel method_get]
    mov ecx, method_get_len
    mov r8, r12
    mov r9, r13
    call route_index_find
    mov rcx, [rsp + 0]
    test rcx, rcx
    jz .index_expect_miss
    test rax, rax
    jz .bad
    jmp .index_probe_ok
.index_expect_miss:
    test rax, rax
    jnz .bad
.index_probe_ok:

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp + 8], rax
    mov ebx, ITERATIONS
.loop:
    lea rdi, [rel slots]
    mov esi, 32
    lea rdx, [rel method_get]
    mov ecx, method_get_len
    mov r8, r12
    mov r9, r13
    call route_index_find
    dec ebx
    jnz .loop

    call bench_now_ns
    test rax, rax
    js .bad
    sub rax, [rsp + 8]
    mov rcx, rax
    mov rdi, r14
    mov rsi, r15
    mov edx, ITERATIONS
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

bench_handler:
    mov eax, 200
    ret

fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
