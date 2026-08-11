; Arborcore checked integer geometry
;
; Default error contract:
;   RAX = 0            success
;   RAX = -EINVAL      invalid argument/domain
;   RAX = -EOVERFLOW   mathematical result is not representable
;
; Default result contract:
;   RDX = result on success
;   RDX = 0 on every error
;
; Unless a routine explicitly says otherwise, inputs are interpreted as
; unsigned 64-bit integers.  The helpers in this module define reference ABI
; semantics for size/alignment/narrowing operations; hot callers may later
; inline an observationally equivalent formula when qualification justifies it.

%define ERR_EINVAL     -22
%define ERR_EOVERFLOW  -75


global u64_add_checked:function
global u64_sub_checked:function
global u64_mul_checked:function
global u64_floor_div_checked:function
global u64_ceil_div_checked:function
global u64_align_up_checked:function
global u64_align_down_checked:function
global u64_to_u32_checked:function
global u64_to_i32_nonnegative_checked:function
global s64_to_i32_checked:function


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
; For unsigned arithmetic, right > left is underflow and therefore a result
; outside U64.  Arborcore reports that mathematical non-representability as
; EOVERFLOW.
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
; MUL forms a 128-bit product in RDX:RAX.  A non-zero high half proves that
; the mathematical product does not belong to U64.
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
; u64_floor_div_checked(value, divisor)
;
; Return:
;   RAX = 0 / -EINVAL
;   RDX = floor(value / divisor) / 0 on error
;
; divisor must be non-zero.
; ============================================================

u64_floor_div_checked:
    test rsi, rsi
    jz .invalid

    mov rax, rdi
    xor edx, edx
    div rsi

    mov rdx, rax
    xor eax, eax
    ret

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret


; ============================================================
; u64_ceil_div_checked(value, divisor)
;
; Return:
;   RAX = 0 / -EINVAL
;   RDX = ceil(value / divisor) / 0 on error
;
; Uses quotient/remainder rather than (value + divisor - 1) / divisor,
; avoiding an unnecessary addition-overflow hazard.
; ============================================================

u64_ceil_div_checked:
    test rsi, rsi
    jz .invalid

    mov rax, rdi
    xor edx, edx
    div rsi

    mov rcx, rax
    test rdx, rdx
    jz .success

    ; With a non-zero remainder, quotient < value <= UINT64_MAX, so the
    ; increment is mathematically representable.  Keep it explicit because
    ; it is part of the reference semantics.
    inc rcx

.success:
    mov rdx, rcx
    xor eax, eax
    ret

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret


; ============================================================
; u64_align_up_checked(value, alignment)
;
; alignment must be a non-zero power of two.
; ============================================================

u64_align_up_checked:
    test rsi, rsi
    jz .invalid

    mov rcx, rsi
    dec rcx

    test rsi, rcx
    jnz .invalid

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


; ============================================================
; u64_align_down_checked(value, alignment)
;
; Return the greatest aligned value <= input.
; alignment must be a non-zero power of two.
; ============================================================

u64_align_down_checked:
    test rsi, rsi
    jz .invalid

    mov rcx, rsi
    dec rcx

    test rsi, rcx
    jnz .invalid

    not rcx
    mov rdx, rdi
    and rdx, rcx

    xor eax, eax
    ret

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret


; ============================================================
; u64_to_u32_checked(value)
;
; Return zero-extended uint32 iff value <= UINT32_MAX.
; ============================================================

u64_to_u32_checked:
    mov rax, rdi
    shr rax, 32
    jnz .overflow

    mov edx, edi
    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; u64_to_i32_nonnegative_checked(value)
;
; Return value iff 0 <= value <= INT32_MAX.
; This is the canonical narrowing for non-negative Linux `int` domains such
; as counts/backlogs/maxevents.  It does not model negative sentinels.
; ============================================================

u64_to_i32_nonnegative_checked:
    cmp rdi, 0x7fffffff
    ja .overflow

    mov edx, edi
    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; s64_to_i32_checked(value)
;
; Input:
;   RDI = signed 64-bit value
;
; Return:
;   RDX = sign-extended int32 value on success
;   RDX = 0 on overflow
;
; This explicitly models signed Linux `int` domains, including negative
; sentinels such as -1 when a later ABI chooses to permit them.
; ============================================================

s64_to_i32_checked:
    cmp rdi, -2147483648
    jl .overflow

    cmp rdi, 2147483647
    jg .overflow

    movsxd rdx, edi
    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
