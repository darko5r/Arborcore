; Arborcore canonical unsigned 64-bit range algebra
;
; A range is the half-open interval:
;
;   [base, base + length)
;
; Endpoint arithmetic must be representable in uint64.  These routines are
; reference ABI implementations for the Core specification; hot paths may
; later inline observationally equivalent arithmetic when qualified.
;
; Error contract:
;   RAX = 0            success
;   RAX = -EINVAL      argument is outside the requested range domain
;   RAX = -EOVERFLOW   an endpoint is not representable in uint64
;
; Result contract:
;   RDX = result/boolean on success
;   RDX = 0 on every error

%define ERR_EINVAL     -22
%define ERR_EOVERFLOW  -75


global range_end_checked:function
global range_contains_point:function
global range_contains_span:function
global range_overlaps:function
global range_remaining:function


section .text


; ============================================================
; range_end_checked(base, length)
;
; Return the representable half-open endpoint base + length.
; ============================================================

range_end_checked:
    mov rdx, rdi
    add rdx, rsi
    jc .overflow

    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; range_contains_point(base, length, point)
;
; Return RDX=1 iff point belongs to [base,base+length).
; Empty ranges contain no points.
; ============================================================

range_contains_point:
    mov r8, rdi
    mov r9, rsi
    mov r10, rdx

    mov rax, r8
    add rax, r9
    jc .overflow

    xor edx, edx

    cmp r10, r8
    jb .success

    cmp r10, rax
    setb dl

.success:
    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; range_contains_span(base, length, child_base, child_length)
;
; Return RDX=1 iff the child half-open interval is a subset of parent.
; An empty child at the parent's endpoint is contained.
; ============================================================

range_contains_span:
    mov r8, rdi
    mov r9, rsi
    mov r10, rdx
    mov r11, rcx

    mov rax, r8
    add rax, r9
    jc .overflow

    mov rcx, r10
    add rcx, r11
    jc .overflow

    xor edx, edx

    cmp r10, r8
    jb .success

    cmp rcx, rax
    setbe dl

.success:
    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; range_overlaps(base_a, length_a, base_b, length_b)
;
; Return RDX=1 iff two non-empty half-open ranges intersect.
; Representability of both ranges is checked even when one is empty.
; ============================================================

range_overlaps:
    mov r8, rdi
    mov r9, rsi
    mov r10, rdx
    mov r11, rcx

    mov rax, r8
    add rax, r9
    jc .overflow

    mov rcx, r10
    add rcx, r11
    jc .overflow

    xor edx, edx

    test r9, r9
    jz .success

    test r11, r11
    jz .success

    cmp r8, rcx
    jae .success

    cmp r10, rax
    setb dl

.success:
    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; range_remaining(base, length, position)
;
; Domain:
;   base <= position <= base+length
;
; The endpoint itself is accepted and has zero remaining bytes.
; ============================================================

range_remaining:
    mov r8, rdi
    mov r9, rsi
    mov r10, rdx

    mov rax, r8
    add rax, r9
    jc .overflow

    cmp r10, r8
    jb .invalid

    cmp r10, rax
    ja .invalid

    mov rdx, rax
    sub rdx, r10

    xor eax, eax
    ret

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
