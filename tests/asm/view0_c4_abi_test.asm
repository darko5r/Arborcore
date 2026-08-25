; VIEW0 C4 real NASM consumer of the existing VIEW C API.
; Test-only application-style renderer; no production Assembly ABI extension.
; Linux x86-64 System V AMD64 ABI.

default rel

%define ERR_EINVAL -22
%define ERR_EFAULT -14
%define ARBOR_STATUS_INVALID_ARGUMENT 6
%define ARBOR_STATUS_NATIVE_ERROR 10

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
view0_c4_prefix: db '<p>'
view0_c4_prefix_len equ $ - view0_c4_prefix
view0_c4_suffix: db '</p>'
view0_c4_suffix_len equ $ - view0_c4_suffix

section .text

global view0_c4_asm_render_html_text:function
global view0_c4_asm_begin_abort:function
global view0_c4_asm_call_render_preserve:function

extern arbor_view_measure_add
extern arbor_view_html_text_measure
extern arbor_view_output_begin
extern arbor_view_output_append
extern arbor_view_html_text_append
extern arbor_view_output_commit
extern arbor_view_output_abort
extern range_end_checked
extern range_overlaps

; Local helper: returns EAX=1 iff a CALL from the caller was made with
; RSP 16-byte aligned, which means this helper sees RSP%16 == 8 on entry.
view0_c4_call_alignment_ok:
    mov rax, rsp
    and eax, 15
    cmp eax, 8
    sete al
    movzx eax, al
    ret

; arbor_status view0_c4_asm_render_html_text(
;     arbor_asm_arena *arena,
;     arbor_span text,
;     arbor_span *body_out)
;
; SysV classification on x86-64:
;   RDI = arena
;   RSI = text.data
;   RDX = text.length
;   RCX = body_out
;
; The renderer is deliberately an application-style consumer. It uses the
; seven C VIEW functions without introducing a second production API.
view0_c4_asm_render_html_text:
    push r12
    push r13
    push r14
    push r15
    sub rsp, RENDER_LOCAL_SIZE

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx

    ; Four pushes leave RSP%16 == 8; 72 local bytes move it to 0 so every
    ; subsequent CALL satisfies the SysV call-site alignment requirement.
    call view0_c4_call_alignment_ok
    test eax, eax
    jz .invalid

    ; Zero measurement + arbor_view_output local state.
    xor eax, eax
    mov [rsp + MEASURE_OFFSET], rax
    mov [rsp + OUTPUT_OFFSET + 0], rax
    mov [rsp + OUTPUT_OFFSET + 8], rax
    mov [rsp + OUTPUT_OFFSET + 16], rax
    mov [rsp + OUTPUT_OFFSET + 24], rax
    mov [rsp + OUTPUT_OFFSET + 32], rax
    mov [rsp + OUTPUT_OFFSET + 40], rax
    mov [rsp + OUTPUT_OFFSET + 48], rax

    ; Pass 1: exact measurement.
    lea rdi, [rsp + MEASURE_OFFSET]
    mov esi, view0_c4_prefix_len
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
    mov esi, view0_c4_suffix_len
    call arbor_view_measure_add
    test rdx, rdx
    jnz .return_status

    ; C3-style cross-role alias policy adapted to a by-value span. The caller
    ; supplies live arena/body objects. C2 already validated text representability.
    test r12, r12
    jz .invalid
    test r15, r15
    jz .invalid

    ; Borrowed text may not alias mutable arena metadata.
    mov rdi, r13
    mov rsi, r14
    mov rdx, r12
    mov ecx, ARENA_SIZE
    call range_overlaps
    test rax, rax
    jnz .invalid
    test rdx, rdx
    jnz .invalid

    ; Result metadata may not alias arena metadata or borrowed text.
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

    ; Preserve C1 error precedence. If arena metadata is invalid or the body
    ; cannot fit, do not invent a future-reservation alias; let C1 begin return
    ; the authoritative EINVAL/EOVERFLOW/ENOSPC status before any body write.
    mov r8, [r12 + ARENA_OFFSET]
    mov r9, [r12 + ARENA_CAPACITY]
    cmp r8, r9
    ja .begin
    mov r10, r9
    sub r10, r8
    mov r11, [rsp + MEASURE_OFFSET]
    cmp r11, r10
    ja .begin

    ; Prove the entire backing range representable before deriving future_start.
    mov rdi, [r12 + ARENA_BASE]
    mov rsi, r9
    call range_end_checked
    test rax, rax
    jnz .begin

    ; Result metadata must also stay outside arena backing, matching C1 commit.
    mov rdi, r15
    mov esi, SPAN_SIZE
    mov rdx, [r12 + ARENA_BASE]
    mov rcx, [r12 + ARENA_CAPACITY]
    call range_overlaps
    test rax, rax
    jnz .invalid
    test rdx, rdx
    jnz .invalid

    ; Reject a borrowed source that would be destroyed by the future body.
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

    ; Pass 2: trusted literals plus C2 HTML Data/text escaping.
    lea rdi, [rsp + OUTPUT_OFFSET]
    lea rsi, [rel view0_c4_prefix]
    mov edx, view0_c4_prefix_len
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
    lea rsi, [rel view0_c4_suffix]
    mov edx, view0_c4_suffix_len
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

; arbor_status view0_c4_asm_begin_abort(arbor_asm_arena *arena,
;                                       uint64_t required_length)
; Qualifies the seventh VIEW C function (abort) from real NASM.
view0_c4_asm_begin_abort:
    sub rsp, OUTPUT_SIZE

    call view0_c4_call_alignment_ok
    test eax, eax
    jz .begin_abort_invalid

    xor eax, eax
    mov [rsp + 0], rax
    mov [rsp + 8], rax
    mov [rsp + 16], rax
    mov [rsp + 24], rax
    mov [rsp + 32], rax
    mov [rsp + 40], rax
    mov [rsp + 48], rax

    ; Original RDI/RSI are caller-saved and the alignment helper clobbered RAX
    ; only, so they remain the C1 begin arguments.
    lea rdx, [rsp]
    call arbor_view_output_begin
    test rdx, rdx
    jnz .begin_abort_return

    lea rdi, [rsp]
    call arbor_view_output_abort
    jmp .begin_abort_return

.begin_abort_invalid:
    mov eax, ARBOR_STATUS_INVALID_ARGUMENT
    mov rdx, ERR_EINVAL

.begin_abort_return:
    add rsp, OUTPUT_SIZE
    ret

; arbor_status view0_c4_asm_call_render_preserve(arena*, arbor_span, body_out*)
; Seeds all SysV callee-saved GPRs, calls the real Assembly renderer, and proves
; they are unchanged. Returns -EFAULT/native_error if any sentinel is clobbered.
view0_c4_asm_call_render_preserve:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    sub rsp, 8

    mov rbx, 0x1122334455667788
    mov rbp, 0x2233445566778899
    mov r12, 0x33445566778899aa
    mov r13, 0x445566778899aabb
    mov r14, 0x5566778899aabbcc
    mov r15, 0x66778899aabbccdd

    ; RDI/RSI/RDX/RCX still carry the original renderer arguments.
    call view0_c4_asm_render_html_text
    mov r10, rax
    mov r11, rdx

    mov rax, 0x1122334455667788
    cmp rbx, rax
    jne .preserve_failed
    mov rax, 0x2233445566778899
    cmp rbp, rax
    jne .preserve_failed
    mov rax, 0x33445566778899aa
    cmp r12, rax
    jne .preserve_failed
    mov rax, 0x445566778899aabb
    cmp r13, rax
    jne .preserve_failed
    mov rax, 0x5566778899aabbcc
    cmp r14, rax
    jne .preserve_failed
    mov rax, 0x66778899aabbccdd
    cmp r15, rax
    jne .preserve_failed

    mov rax, r10
    mov rdx, r11
    jmp .preserve_restore

.preserve_failed:
    mov eax, ARBOR_STATUS_NATIVE_ERROR
    mov rdx, ERR_EFAULT

.preserve_restore:
    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
