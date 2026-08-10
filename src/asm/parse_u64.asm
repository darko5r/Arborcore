; Arborcore bounded unsigned-64 parsing primitives
;
; Contracts:
;
;   bytes_parse_u64_decimal(buffer, length)
;   bytes_parse_u64_hex(buffer, length)
;
; Input:
;   RDI = buffer address
;   RSI = length
;
; Return:
;   RAX = 0             success
;   RAX = -EINVAL       invalid syntax
;   RAX = -EOVERFLOW    value exceeds UINT64_MAX
;
;   RDX = parsed uint64 value on success
;   RDX = 0 on failure
;
; Empty input is invalid.
; No sign or prefix is accepted.
; Hex accepts 0-9, A-F, a-f only; it does not accept "0x".


%define ERR_EINVAL    -22
%define ERR_EOVERFLOW -75


global bytes_parse_u64_decimal:function
global bytes_parse_u64_hex:function


section .text


; ============================================================
; bytes_parse_u64_decimal
; ============================================================

bytes_parse_u64_decimal:
    test rsi, rsi
    jz .invalid

    xor r8d, r8d
    xor r9d, r9d

.next:
    movzx ecx, byte [rdi + r9]

    sub ecx, '0'
    cmp ecx, 9
    ja .invalid

    ; R8 = R8 * 10, rejecting unsigned overflow.

    mov rax, r8
    mov r10d, 10
    mul r10

    test rdx, rdx
    jnz .overflow

    add rax, rcx
    jc .overflow

    mov r8, rax

    inc r9
    cmp r9, rsi
    jb .next

    xor eax, eax
    mov rdx, r8
    ret

.invalid:
    mov rax, ERR_EINVAL
    xor edx, edx
    ret

.overflow:
    mov rax, ERR_EOVERFLOW
    xor edx, edx
    ret


; ============================================================
; bytes_parse_u64_hex
; ============================================================

bytes_parse_u64_hex:
    test rsi, rsi
    jz .invalid

    xor r8d, r8d
    xor r9d, r9d

.next:
    movzx ecx, byte [rdi + r9]

    ; 0-9

    mov eax, ecx
    sub eax, '0'
    cmp eax, 9
    jbe .digit_ready

    ; A-F / a-f

    mov eax, ecx
    or eax, 0x20
    sub eax, 'a'
    cmp eax, 'f' - 'a'
    ja .invalid

    add eax, 10

.digit_ready:
    ; Before shifting left by four, the top nibble must be zero.

    mov r10, r8
    shr r10, 60
    jnz .overflow

    shl r8, 4
    add r8, rax

    inc r9
    cmp r9, rsi
    jb .next

    xor eax, eax
    mov rdx, r8
    ret

.invalid:
    mov rax, ERR_EINVAL
    xor edx, edx
    ret

.overflow:
    mov rax, ERR_EOVERFLOW
    xor edx, edx
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
