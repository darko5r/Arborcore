; Arborcore Core Retrofit A: half-open range algebra qualification
;
; Exit status:
;   0 = pass
;   1 = endpoint/overflow contract
;   2 = point containment
;   3 = span containment
;   4 = overlap
;   5 = remaining

%define SYS_EXIT        60
%define ERR_EINVAL     -22
%define ERR_EOVERFLOW  -75


global _start


extern range_end_checked
extern range_contains_point
extern range_contains_span
extern range_overlaps
extern range_remaining


section .text


_start:
    cld

    ; ========================================================
    ; Endpoint boundaries
    ; ========================================================

    xor edi, edi
    xor esi, esi
    call range_end_checked
    test rax, rax
    jnz core_range_fail_end
    test rdx, rdx
    jnz core_range_fail_end

    mov rdi, -1
    xor esi, esi
    call range_end_checked
    test rax, rax
    jnz core_range_fail_end
    cmp rdx, -1
    jne core_range_fail_end

    mov rdi, -1
    mov esi, 1
    call range_end_checked
    cmp rax, ERR_EOVERFLOW
    jne core_range_fail_end
    test rdx, rdx
    jnz core_range_fail_end

    mov rdi, 0xfffffffffffffffe
    mov esi, 1
    call range_end_checked
    test rax, rax
    jnz core_range_fail_end
    cmp rdx, -1
    jne core_range_fail_end

    ; Exhaustive small endpoint domain.
    xor r12d, r12d

core_range_end_base_loop:
    xor r13d, r13d

core_range_end_length_loop:
    mov rdi, r12
    mov rsi, r13
    call range_end_checked
    test rax, rax
    jnz core_range_fail_end

    mov rbx, r12
    add rbx, r13
    cmp rdx, rbx
    jne core_range_fail_end

    inc r13
    cmp r13, 31
    jbe core_range_end_length_loop

    inc r12
    cmp r12, 31
    jbe core_range_end_base_loop

    ; ========================================================
    ; Point containment: exhaustive small domain.
    ; ========================================================

    xor r12d, r12d

core_range_point_base_loop:
    xor r13d, r13d

core_range_point_length_loop:
    xor r14d, r14d

core_range_point_position_loop:
    xor ebx, ebx
    mov rbp, r12
    add rbp, r13

    cmp r14, r12
    jb core_range_point_expected_ready
    cmp r14, rbp
    jae core_range_point_expected_ready
    mov ebx, 1

core_range_point_expected_ready:
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    call range_contains_point
    test rax, rax
    jnz core_range_fail_point
    cmp rdx, rbx
    jne core_range_fail_point

    inc r14
    cmp r14, 31
    jbe core_range_point_position_loop

    inc r13
    cmp r13, 15
    jbe core_range_point_length_loop

    inc r12
    cmp r12, 15
    jbe core_range_point_base_loop

    ; Overflowing parent range must fail closed.
    mov rdi, -1
    mov esi, 1
    xor edx, edx
    call range_contains_point
    cmp rax, ERR_EOVERFLOW
    jne core_range_fail_point
    test rdx, rdx
    jnz core_range_fail_point

    ; ========================================================
    ; Span containment: exhaustive small domain.
    ; ========================================================

    xor r12d, r12d

core_range_span_base_loop:
    xor r13d, r13d

core_range_span_length_loop:
    xor r14d, r14d

core_range_span_child_base_loop:
    xor r15d, r15d

core_range_span_child_length_loop:
    xor ebx, ebx

    mov rax, r12
    add rax, r13                    ; parent end
    mov rbp, r14
    add rbp, r15                    ; child end

    cmp r14, r12
    jb core_range_span_expected_ready
    cmp rbp, rax
    ja core_range_span_expected_ready
    mov ebx, 1

core_range_span_expected_ready:
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    mov rcx, r15
    call range_contains_span
    test rax, rax
    jnz core_range_fail_span
    cmp rdx, rbx
    jne core_range_fail_span

    inc r15
    cmp r15, 7
    jbe core_range_span_child_length_loop

    inc r14
    cmp r14, 15
    jbe core_range_span_child_base_loop

    inc r13
    cmp r13, 7
    jbe core_range_span_length_loop

    inc r12
    cmp r12, 7
    jbe core_range_span_base_loop

    ; Child endpoint overflow must be reported even when parent is valid.
    xor edi, edi
    mov rsi, -1
    mov rdx, -1
    mov ecx, 1
    call range_contains_span
    cmp rax, ERR_EOVERFLOW
    jne core_range_fail_span
    test rdx, rdx
    jnz core_range_fail_span

    ; ========================================================
    ; Overlap: exhaustive small domain.
    ; ========================================================

    xor r12d, r12d

core_range_overlap_a_base_loop:
    xor r13d, r13d

core_range_overlap_a_length_loop:
    xor r14d, r14d

core_range_overlap_b_base_loop:
    xor r15d, r15d

core_range_overlap_b_length_loop:
    xor ebx, ebx

    test r13, r13
    jz core_range_overlap_expected_ready
    test r15, r15
    jz core_range_overlap_expected_ready

    mov rax, r12
    add rax, r13                    ; end A
    mov rbp, r14
    add rbp, r15                    ; end B

    cmp r12, rbp
    jae core_range_overlap_expected_ready
    cmp r14, rax
    jae core_range_overlap_expected_ready
    mov ebx, 1

core_range_overlap_expected_ready:
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    mov rcx, r15
    call range_overlaps
    test rax, rax
    jnz core_range_fail_overlap
    cmp rdx, rbx
    jne core_range_fail_overlap

    inc r15
    cmp r15, 7
    jbe core_range_overlap_b_length_loop

    inc r14
    cmp r14, 7
    jbe core_range_overlap_b_base_loop

    inc r13
    cmp r13, 7
    jbe core_range_overlap_a_length_loop

    inc r12
    cmp r12, 7
    jbe core_range_overlap_a_base_loop

    ; Empty ranges are non-overlapping even when located inside another range.
    mov edi, 5
    xor esi, esi
    xor edx, edx
    mov ecx, 10
    call range_overlaps
    test rax, rax
    jnz core_range_fail_overlap
    test rdx, rdx
    jnz core_range_fail_overlap

    ; ========================================================
    ; Remaining: exhaustive small domain including endpoint.
    ; ========================================================

    xor r12d, r12d

core_range_remaining_base_loop:
    xor r13d, r13d

core_range_remaining_length_loop:
    xor r14d, r14d

core_range_remaining_position_loop:
    xor ebx, ebx                    ; expected success boolean
    xor ebp, ebp                    ; expected remaining

    mov rax, r12
    add rax, r13                    ; end

    cmp r14, r12
    jb core_range_remaining_expected_ready
    cmp r14, rax
    ja core_range_remaining_expected_ready

    mov ebx, 1
    mov rbp, rax
    sub rbp, r14

core_range_remaining_expected_ready:
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    call range_remaining

    test rbx, rbx
    jz core_range_remaining_expect_invalid

    test rax, rax
    jnz core_range_fail_remaining
    cmp rdx, rbp
    jne core_range_fail_remaining
    jmp core_range_remaining_next

core_range_remaining_expect_invalid:
    cmp rax, ERR_EINVAL
    jne core_range_fail_remaining
    test rdx, rdx
    jnz core_range_fail_remaining

core_range_remaining_next:
    inc r14
    cmp r14, 15
    jbe core_range_remaining_position_loop

    inc r13
    cmp r13, 7
    jbe core_range_remaining_length_loop

    inc r12
    cmp r12, 7
    jbe core_range_remaining_base_loop

    ; Zero-length range accepts its endpoint for remaining=0.
    mov rdi, -1
    xor esi, esi
    mov rdx, -1
    call range_remaining
    test rax, rax
    jnz core_range_fail_remaining
    test rdx, rdx
    jnz core_range_fail_remaining

    ; Endpoint overflow fails closed.
    mov rdi, -1
    mov esi, 1
    mov rdx, -1
    call range_remaining
    cmp rax, ERR_EOVERFLOW
    jne core_range_fail_remaining
    test rdx, rdx
    jnz core_range_fail_remaining

core_range_success:
    xor edi, edi
    jmp core_range_exit

core_range_fail_end:
    mov edi, 1
    jmp core_range_exit

core_range_fail_point:
    mov edi, 2
    jmp core_range_exit

core_range_fail_span:
    mov edi, 3
    jmp core_range_exit

core_range_fail_overlap:
    mov edi, 4
    jmp core_range_exit

core_range_fail_remaining:
    mov edi, 5

core_range_exit:
    mov eax, SYS_EXIT
    syscall


section .note.GNU-stack noalloc noexec nowrite progbits
