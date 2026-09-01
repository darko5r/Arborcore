; Arborcore VIEW0 D1 runnable NASM view example.
; Linux x86-64 System V AMD64 ABI.
;
; This example renders only the HTML body fragment. HTTP representation metadata
; remains presenter-owned: after UTF-8 validation, the M1 recipe appends exactly
; one Content-Type: text/html; charset=utf-8 field. Controllers/middleware must
; not pre-add that field in the canonical composition.
;
; This is an application-style consumer of the existing VIEW C API and frozen
; Assembly range helpers. It introduces no new Arborcore Assembly ABI symbol.

default rel

%define ERR_EINVAL -22
%define ARBOR_STATUS_INVALID_ARGUMENT 6

%define ARENA_BASE 0
%define ARENA_CAPACITY 8
%define ARENA_OFFSET 16
%define ARENA_SIZE 24
%define SPAN_SIZE 16

%define MEASURE_OFFSET 0
%define OUTPUT_OFFSET 8
%define OUTPUT_SIZE 56
%define RENDER_LOCAL_SIZE 72

section .rodata
view0_d1_prefix: db '<p>'
view0_d1_prefix_len equ $ - view0_d1_prefix
view0_d1_suffix: db '</p>'
view0_d1_suffix_len equ $ - view0_d1_suffix

section .text
global arborcore_view0_d1_nasm_render_html_text:function

extern arbor_view_measure_add
extern arbor_view_html_text_measure
extern arbor_view_output_begin
extern arbor_view_output_append
extern arbor_view_html_text_append
extern arbor_view_output_commit
extern range_end_checked
extern range_overlaps

view0_d1_call_alignment_ok:
    mov rax, rsp
    and eax, 15
    cmp eax, 8
    sete al
    movzx eax, al
    ret

; arbor_status arborcore_view0_d1_nasm_render_html_text(
;     arbor_asm_arena *arena,
;     arbor_span text,
;     arbor_span *body_out)
;
; SysV x86-64: RDI=arena, RSI=text.data, RDX=text.length, RCX=body_out.
arborcore_view0_d1_nasm_render_html_text:
    push r12
    push r13
    push r14
    push r15
    sub rsp, RENDER_LOCAL_SIZE

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx

    call view0_d1_call_alignment_ok
    test eax, eax
    jz .invalid

    xor eax, eax
    mov [rsp + MEASURE_OFFSET], rax
    mov [rsp + OUTPUT_OFFSET + 0], rax
    mov [rsp + OUTPUT_OFFSET + 8], rax
    mov [rsp + OUTPUT_OFFSET + 16], rax
    mov [rsp + OUTPUT_OFFSET + 24], rax
    mov [rsp + OUTPUT_OFFSET + 32], rax
    mov [rsp + OUTPUT_OFFSET + 40], rax
    mov [rsp + OUTPUT_OFFSET + 48], rax

    lea rdi, [rsp + MEASURE_OFFSET]
    mov esi, view0_d1_prefix_len
    call arbor_view_measure_add
    test rdx, rdx
    jnz .return_status

    lea rdi, [rsp + MEASURE_OFFSET]
    mov rsi, r13
    mov rdx, r14
    call arbor_view_html_text_measure
    test rdx, rdx
    jnz .return_status

    lea rdi, [rsp + MEASURE_OFFSET]
    mov esi, view0_d1_suffix_len
    call arbor_view_measure_add
    test rdx, rdx
    jnz .return_status

    test r12, r12
    jz .invalid
    test r15, r15
    jz .invalid

    mov rdi, r13
    mov rsi, r14
    mov rdx, r12
    mov ecx, ARENA_SIZE
    call range_overlaps
    test rax, rax
    jnz .invalid
    test rdx, rdx
    jnz .invalid

    mov rdi, r15
    mov esi, SPAN_SIZE
    mov rdx, r12
    mov ecx, ARENA_SIZE
    call range_overlaps
    test rax, rax
    jnz .invalid
    test rdx, rdx
    jnz .invalid

    mov rdi, r15
    mov esi, SPAN_SIZE
    mov rdx, r13
    mov rcx, r14
    call range_overlaps
    test rax, rax
    jnz .invalid
    test rdx, rdx
    jnz .invalid

    mov r8, [r12 + ARENA_OFFSET]
    mov r9, [r12 + ARENA_CAPACITY]
    cmp r8, r9
    ja .begin
    mov r10, r9
    sub r10, r8
    mov r11, [rsp + MEASURE_OFFSET]
    cmp r11, r10
    ja .begin

    mov rdi, [r12 + ARENA_BASE]
    mov rsi, r9
    call range_end_checked
    test rax, rax
    jnz .begin

    mov rdi, r15
    mov esi, SPAN_SIZE
    mov rdx, [r12 + ARENA_BASE]
    mov rcx, [r12 + ARENA_CAPACITY]
    call range_overlaps
    test rax, rax
    jnz .invalid
    test rdx, rdx
    jnz .invalid

    mov r10, [r12 + ARENA_BASE]
    add r10, [r12 + ARENA_OFFSET]
    mov rdi, r13
    mov rsi, r14
    mov rdx, r10
    mov rcx, [rsp + MEASURE_OFFSET]
    call range_overlaps
    test rax, rax
    jnz .invalid
    test rdx, rdx
    jnz .invalid

.begin:
    mov rdi, r12
    mov rsi, [rsp + MEASURE_OFFSET]
    lea rdx, [rsp + OUTPUT_OFFSET]
    call arbor_view_output_begin
    test rdx, rdx
    jnz .return_status

    lea rdi, [rsp + OUTPUT_OFFSET]
    lea rsi, [rel view0_d1_prefix]
    mov edx, view0_d1_prefix_len
    call arbor_view_output_append
    test rdx, rdx
    jnz .return_status

    lea rdi, [rsp + OUTPUT_OFFSET]
    mov rsi, r13
    mov rdx, r14
    call arbor_view_html_text_append
    test rdx, rdx
    jnz .return_status

    lea rdi, [rsp + OUTPUT_OFFSET]
    lea rsi, [rel view0_d1_suffix]
    mov edx, view0_d1_suffix_len
    call arbor_view_output_append
    test rdx, rdx
    jnz .return_status

    lea rdi, [rsp + OUTPUT_OFFSET]
    mov rsi, r15
    call arbor_view_output_commit
    jmp .return_status

.invalid:
    mov eax, ARBOR_STATUS_INVALID_ARGUMENT
    mov rdx, ERR_EINVAL

.return_status:
    add rsp, RENDER_LOCAL_SIZE
    pop r15
    pop r14
    pop r13
    pop r12
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
