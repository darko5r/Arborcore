; Arborcore checked unsigned 64-bit arithmetic
;
; Error contract:
;   RAX = 0            success
;   RAX = -EINVAL      invalid argument
;   RAX = -EOVERFLOW   arithmetic result is not representable as uint64
;
; Result contract:
;   RDX = result on success
;   RDX = 0 on every error
;
; All input values are interpreted as unsigned 64-bit integers.

%define ERR_EINVAL     -22
%define ERR_EOVERFLOW  -75


global u64_add_checked:function
global u64_sub_checked:function
global u64_mul_checked:function
global u64_align_up_checked:function


section .text


; ============================================================
; u64_add_checked(left, right)
;
; Input:
;   RDI = left
;   RSI = right
;
; Return:
;   RAX = 0 / -EOVERFLOW
;   RDX = sum / 0 on error
; ============================================================

u64_add_checked:
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
; u64_sub_checked(left, right)
;
; Input:
;   RDI = left
;   RSI = right
;
; Return:
;   RAX = 0 / -EOVERFLOW
;   RDX = left - right / 0 on error
;
; For unsigned arithmetic, right > left is an underflow and is
; reported as EOVERFLOW because the mathematical result cannot
; be represented as uint64.
; ============================================================

u64_sub_checked:
    cmp rdi, rsi
    jb .overflow

    mov rdx, rdi
    sub rdx, rsi

    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; u64_mul_checked(left, right)
;
; Input:
;   RDI = left
;   RSI = right
;
; Return:
;   RAX = 0 / -EOVERFLOW
;   RDX = product / 0 on error
;
; MUL forms a 128-bit product in RDX:RAX. Any non-zero high
; half means the product does not fit in uint64.
; ============================================================

u64_mul_checked:
    mov rax, rdi
    mul rsi

    test rdx, rdx
    jnz .overflow

    mov rdx, rax
    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; u64_align_up_checked(value, alignment)
;
; Input:
;   RDI = value
;   RSI = alignment
;
; Return:
;   RAX = 0            success
;   RAX = -EINVAL      alignment is zero or not a power of two
;   RAX = -EOVERFLOW   rounded value cannot fit in uint64
;   RDX = aligned value / 0 on error
;
; Valid alignments are non-zero powers of two.
; ============================================================

u64_align_up_checked:
    test rsi, rsi
    jz .invalid

    ; For a non-zero power of two:
    ;
    ;   alignment & (alignment - 1) == 0

    mov rcx, rsi
    dec rcx

    test rsi, rcx
    jnz .invalid

    ; Round upward with:
    ;
    ;   (value + alignment - 1) & ~(alignment - 1)
    ;
    ; The addition must be checked before applying the mask.

    mov rdx, rdi
    add rdx, rcx
    jc .overflow

    not rcx
    and rdx, rcx

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
