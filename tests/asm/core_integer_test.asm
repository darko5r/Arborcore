; Arborcore Core Retrofit A: integer-geometry qualification
;
; Exit status:
;   0 = pass
;   1 = floor/ceiling division
;   2 = alignment geometry
;   3 = checked narrowing
;   4 = exhaustive small-domain division
;   5 = exhaustive small-domain alignment

%define SYS_EXIT        60
%define ERR_EINVAL     -22
%define ERR_EOVERFLOW  -75


global _start


extern u64_floor_div_checked
extern u64_ceil_div_checked
extern u64_align_up_checked
extern u64_align_down_checked
extern u64_to_u32_checked
extern u64_to_i32_nonnegative_checked
extern s64_to_i32_checked


section .text


_start:
    cld

    ; ========================================================
    ; Boundary/reference vectors: floor/ceiling division
    ; ========================================================

    xor edi, edi
    mov esi, 7
    call u64_floor_div_checked
    test rax, rax
    jnz core_integer_fail_div
    test rdx, rdx
    jnz core_integer_fail_div

    mov rdi, -1
    mov esi, 1
    call u64_floor_div_checked
    test rax, rax
    jnz core_integer_fail_div
    cmp rdx, -1
    jne core_integer_fail_div

    mov rdi, -1
    mov rsi, -1
    call u64_floor_div_checked
    test rax, rax
    jnz core_integer_fail_div
    cmp rdx, 1
    jne core_integer_fail_div

    mov edi, 10
    xor esi, esi
    call u64_floor_div_checked
    cmp rax, ERR_EINVAL
    jne core_integer_fail_div
    test rdx, rdx
    jnz core_integer_fail_div

    xor edi, edi
    mov esi, 7
    call u64_ceil_div_checked
    test rax, rax
    jnz core_integer_fail_div
    test rdx, rdx
    jnz core_integer_fail_div

    mov edi, 1
    mov esi, 8
    call u64_ceil_div_checked
    test rax, rax
    jnz core_integer_fail_div
    cmp rdx, 1
    jne core_integer_fail_div

    mov edi, 8
    mov esi, 8
    call u64_ceil_div_checked
    test rax, rax
    jnz core_integer_fail_div
    cmp rdx, 1
    jne core_integer_fail_div

    mov edi, 9
    mov esi, 8
    call u64_ceil_div_checked
    test rax, rax
    jnz core_integer_fail_div
    cmp rdx, 2
    jne core_integer_fail_div

    ; ceil(UINT64_MAX / 2) = 2^63.
    mov rdi, -1
    mov esi, 2
    call u64_ceil_div_checked
    test rax, rax
    jnz core_integer_fail_div
    mov rcx, 0x8000000000000000
    cmp rdx, rcx
    jne core_integer_fail_div

    mov edi, 10
    xor esi, esi
    call u64_ceil_div_checked
    cmp rax, ERR_EINVAL
    jne core_integer_fail_div
    test rdx, rdx
    jnz core_integer_fail_div

    ; ========================================================
    ; Boundary/reference vectors: align-down
    ; ========================================================

    xor edi, edi
    mov esi, 1
    call u64_align_down_checked
    test rax, rax
    jnz core_integer_fail_align
    test rdx, rdx
    jnz core_integer_fail_align

    mov edi, 15
    mov esi, 8
    call u64_align_down_checked
    test rax, rax
    jnz core_integer_fail_align
    cmp rdx, 8
    jne core_integer_fail_align

    mov edi, 16
    mov esi, 8
    call u64_align_down_checked
    test rax, rax
    jnz core_integer_fail_align
    cmp rdx, 16
    jne core_integer_fail_align

    mov rdi, -1
    mov rsi, 0x8000000000000000
    call u64_align_down_checked
    test rax, rax
    jnz core_integer_fail_align
    mov rcx, 0x8000000000000000
    cmp rdx, rcx
    jne core_integer_fail_align

    mov edi, 10
    xor esi, esi
    call u64_align_down_checked
    cmp rax, ERR_EINVAL
    jne core_integer_fail_align
    test rdx, rdx
    jnz core_integer_fail_align

    mov edi, 10
    mov esi, 3
    call u64_align_down_checked
    cmp rax, ERR_EINVAL
    jne core_integer_fail_align
    test rdx, rdx
    jnz core_integer_fail_align

    ; ========================================================
    ; Checked narrowing boundaries
    ; ========================================================

    xor edi, edi
    call u64_to_u32_checked
    test rax, rax
    jnz core_integer_fail_narrow
    test rdx, rdx
    jnz core_integer_fail_narrow

    mov rdi, 0xffffffff
    call u64_to_u32_checked
    test rax, rax
    jnz core_integer_fail_narrow
    mov ecx, 0xffffffff
    cmp rdx, rcx
    jne core_integer_fail_narrow

    mov rdi, 0x100000000
    call u64_to_u32_checked
    cmp rax, ERR_EOVERFLOW
    jne core_integer_fail_narrow
    test rdx, rdx
    jnz core_integer_fail_narrow

    mov rdi, 0x7fffffff
    call u64_to_i32_nonnegative_checked
    test rax, rax
    jnz core_integer_fail_narrow
    cmp rdx, 0x7fffffff
    jne core_integer_fail_narrow

    mov rdi, 0x80000000
    call u64_to_i32_nonnegative_checked
    cmp rax, ERR_EOVERFLOW
    jne core_integer_fail_narrow
    test rdx, rdx
    jnz core_integer_fail_narrow

    mov rdi, -2147483648
    call s64_to_i32_checked
    test rax, rax
    jnz core_integer_fail_narrow
    mov rcx, -2147483648
    cmp rdx, rcx
    jne core_integer_fail_narrow

    mov rdi, 2147483647
    call s64_to_i32_checked
    test rax, rax
    jnz core_integer_fail_narrow
    cmp rdx, 2147483647
    jne core_integer_fail_narrow

    mov rdi, -2147483649
    call s64_to_i32_checked
    cmp rax, ERR_EOVERFLOW
    jne core_integer_fail_narrow
    test rdx, rdx
    jnz core_integer_fail_narrow

    mov rdi, 2147483648
    call s64_to_i32_checked
    cmp rax, ERR_EOVERFLOW
    jne core_integer_fail_narrow
    test rdx, rdx
    jnz core_integer_fail_narrow

    ; ========================================================
    ; Exhaustive small-domain division against an independent
    ; repeated-subtraction reference implementation.
    ;
    ; value   = 0..255
    ; divisor = 1..32
    ; ========================================================

    xor r12d, r12d

core_integer_div_value_loop:
    mov r13d, 1

core_integer_div_divisor_loop:
    mov rdi, r12
    mov rsi, r13
    call u64_floor_div_checked
    test rax, rax
    jnz core_integer_fail_div_exhaustive
    mov r14, rdx

    mov rdi, r12
    mov rsi, r13
    call core_integer_ref_div_small
    cmp r14, rax
    jne core_integer_fail_div_exhaustive

    mov rdi, r12
    mov rsi, r13
    call u64_ceil_div_checked
    test rax, rax
    jnz core_integer_fail_div_exhaustive
    mov r14, rdx

    mov rdi, r12
    mov rsi, r13
    call core_integer_ref_div_small
    mov r15, rax
    test rdx, rdx
    jz core_integer_ceil_expected_ready
    inc r15

core_integer_ceil_expected_ready:
    cmp r14, r15
    jne core_integer_fail_div_exhaustive

    inc r13
    cmp r13, 32
    jbe core_integer_div_divisor_loop

    inc r12
    cmp r12, 255
    jbe core_integer_div_value_loop

    ; ========================================================
    ; Exhaustive small-domain alignment properties.
    ;
    ; value     = 0..255
    ; alignment = 1,2,4,...,256
    ;
    ; The defining inequalities/divisibility properties uniquely
    ; identify align-up and align-down for these domains.
    ; ========================================================

    xor r12d, r12d

core_integer_align_value_loop:
    mov r13d, 1

core_integer_align_alignment_loop:
    mov rdi, r12
    mov rsi, r13
    call u64_align_up_checked
    test rax, rax
    jnz core_integer_fail_align_exhaustive
    mov r14, rdx

    ; up >= value
    cmp r14, r12
    jb core_integer_fail_align_exhaustive

    ; up - value < alignment
    mov rax, r14
    sub rax, r12
    cmp rax, r13
    jae core_integer_fail_align_exhaustive

    ; up mod alignment == 0
    mov rax, r14
    xor edx, edx
    div r13
    test rdx, rdx
    jnz core_integer_fail_align_exhaustive

    mov rdi, r12
    mov rsi, r13
    call u64_align_down_checked
    test rax, rax
    jnz core_integer_fail_align_exhaustive
    mov r14, rdx

    ; down <= value
    cmp r14, r12
    ja core_integer_fail_align_exhaustive

    ; value - down < alignment
    mov rax, r12
    sub rax, r14
    cmp rax, r13
    jae core_integer_fail_align_exhaustive

    ; down mod alignment == 0
    mov rax, r14
    xor edx, edx
    div r13
    test rdx, rdx
    jnz core_integer_fail_align_exhaustive

    shl r13, 1
    cmp r13, 256
    jbe core_integer_align_alignment_loop

    inc r12
    cmp r12, 255
    jbe core_integer_align_value_loop

core_integer_success:
    xor edi, edi
    jmp core_integer_exit


core_integer_fail_div:
    mov edi, 1
    jmp core_integer_exit

core_integer_fail_align:
    mov edi, 2
    jmp core_integer_exit

core_integer_fail_narrow:
    mov edi, 3
    jmp core_integer_exit

core_integer_fail_div_exhaustive:
    mov edi, 4
    jmp core_integer_exit

core_integer_fail_align_exhaustive:
    mov edi, 5

core_integer_exit:
    mov eax, SYS_EXIT
    syscall


; ============================================================
; Independent small-domain unsigned reference division.
;
; Input:
;   RDI = value
;   RSI = divisor (>0)
;
; Return:
;   RAX = quotient
;   RDX = remainder
; ============================================================

core_integer_ref_div_small:
    xor eax, eax
    mov rdx, rdi

core_integer_ref_div_loop:
    cmp rdx, rsi
    jb core_integer_ref_div_done

    sub rdx, rsi
    inc rax
    jmp core_integer_ref_div_loop

core_integer_ref_div_done:
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
