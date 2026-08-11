; Arborcore percent-codec microbenchmark
; Output: metric<TAB>iterations<TAB>total_ns
;
; Workloads deliberately isolate the RFC 3986 unreserved classifier:
;   unreserved: all bytes are emitted literally
;   escaped:    all bytes require %HH encoding
;   mixed:      alternating unreserved/reserved bytes
;
; Correctness probes execute outside timed regions.

%define SYS_EXIT 60
%define ITERATIONS 300000
%define DEST_CAPACITY 256

extern percent_encoded_length
extern percent_encode
extern bench_now_ns
extern bench_emit_result

global _start

section .rodata
metric_length_unreserved: db "percent_length_unreserved"
metric_length_unreserved_len equ $ - metric_length_unreserved
metric_length_escaped: db "percent_length_escaped"
metric_length_escaped_len equ $ - metric_length_escaped
metric_length_mixed: db "percent_length_mixed"
metric_length_mixed_len equ $ - metric_length_mixed
metric_encode_unreserved: db "percent_encode_unreserved"
metric_encode_unreserved_len equ $ - metric_encode_unreserved
metric_encode_escaped: db "percent_encode_escaped"
metric_encode_escaped_len equ $ - metric_encode_escaped
metric_encode_mixed: db "percent_encode_mixed"
metric_encode_mixed_len equ $ - metric_encode_mixed

source_unreserved:
    db "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~"
source_unreserved_len equ $ - source_unreserved
source_unreserved_encoded_len equ source_unreserved_len

source_escaped:
    times 4 db "/?:@&=+$,#[]!()*"
source_escaped_len equ $ - source_escaped
source_escaped_encoded_len equ source_escaped_len * 3

source_mixed:
    times 32 db "A/"
source_mixed_len equ $ - source_mixed
source_mixed_encoded_len equ 32 + (32 * 3)

section .bss
align 16
destination: resb DEST_CAPACITY

section .text
_start:
    cld

    lea rdi, [rel source_unreserved]
    mov esi, source_unreserved_len
    lea rdx, [rel metric_length_unreserved]
    mov ecx, metric_length_unreserved_len
    mov r8d, ITERATIONS
    mov r9d, source_unreserved_encoded_len
    call run_length_case
    test rax, rax
    jnz .fail

    lea rdi, [rel source_escaped]
    mov esi, source_escaped_len
    lea rdx, [rel metric_length_escaped]
    mov ecx, metric_length_escaped_len
    mov r8d, ITERATIONS
    mov r9d, source_escaped_encoded_len
    call run_length_case
    test rax, rax
    jnz .fail

    lea rdi, [rel source_mixed]
    mov esi, source_mixed_len
    lea rdx, [rel metric_length_mixed]
    mov ecx, metric_length_mixed_len
    mov r8d, ITERATIONS
    mov r9d, source_mixed_encoded_len
    call run_length_case
    test rax, rax
    jnz .fail

    lea rdi, [rel source_unreserved]
    mov esi, source_unreserved_len
    lea rdx, [rel metric_encode_unreserved]
    mov ecx, metric_encode_unreserved_len
    mov r8d, ITERATIONS
    mov r9d, source_unreserved_encoded_len
    call run_encode_case
    test rax, rax
    jnz .fail

    lea rdi, [rel source_escaped]
    mov esi, source_escaped_len
    lea rdx, [rel metric_encode_escaped]
    mov ecx, metric_encode_escaped_len
    mov r8d, ITERATIONS
    mov r9d, source_escaped_encoded_len
    call run_encode_case
    test rax, rax
    jnz .fail

    lea rdi, [rel source_mixed]
    mov esi, source_mixed_len
    lea rdx, [rel metric_encode_mixed]
    mov ecx, metric_encode_mixed_len
    mov r8d, ITERATIONS
    mov r9d, source_mixed_encoded_len
    call run_encode_case
    test rax, rax
    jnz .fail

    xor edi, edi
    jmp .exit

.fail:
    mov edi, 1
.exit:
    mov eax, SYS_EXIT
    syscall

; run_length_case(source, length, metric, metric_len, iterations, expected_length)
run_length_case:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 24

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx
    mov rbx, r8
    mov [rsp + 8], r8
    mov [rsp + 16], r9

    ; Correctness probe outside timed region.
    mov rdi, r12
    mov rsi, r13
    call percent_encoded_length
    test rax, rax
    jnz .bad
    cmp rdx, [rsp + 16]
    jne .bad

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp], rax

.loop:
    mov rdi, r12
    mov rsi, r13
    call percent_encoded_length
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
    add rsp, 24
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; run_encode_case(source, length, metric, metric_len, iterations, expected_length)
run_encode_case:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 24

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx
    mov rbx, r8
    mov [rsp + 8], r8
    mov [rsp + 16], r9

    ; Correctness probe outside timed region.
    mov rdi, r12
    mov rsi, r13
    lea rdx, [rel destination]
    mov ecx, DEST_CAPACITY
    call percent_encode
    test rax, rax
    jnz .bad
    cmp rdx, [rsp + 16]
    jne .bad

    call bench_now_ns
    test rax, rax
    js .bad
    mov [rsp], rax

.loop:
    mov rdi, r12
    mov rsi, r13
    lea rdx, [rel destination]
    mov ecx, DEST_CAPACITY
    call percent_encode
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
    add rsp, 24
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
