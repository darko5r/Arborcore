; Arborcore strict RFC 4648 Base64 codec
;
; Alphabet:
;   A-Z a-z 0-9 + /
;
; Encoding always emits canonical '=' padding.
; Decoding is strict:
;   - encoded length must be a multiple of 4
;   - padding may appear only in the final quartet
;   - one or two '=' bytes are accepted as structurally appropriate
;   - noncanonical nonzero tail bits are rejected
;
; base64_encoded_length(source_length)
; base64_decoded_max_length(encoded_length)
;   RDI = input length
;   RAX = 0 / -EOVERFLOW
;   RDX = required/maximum length on success, 0 on error
;
; base64_encode / base64_decode arguments:
;   RDI = source address
;   RSI = source length
;   RDX = destination address
;   RCX = destination capacity
;
; Return:
;   RAX = 0             success
;   RAX = -22           EINVAL (decode syntax)
;   RAX = -28           ENOSPC
;   RAX = -75           EOVERFLOW
;   RDX = bytes written on success
;   RDX = 0 on error
;
; Capacity/syntax are validated before destination writes.
; Output is not NUL terminated.
; Source and destination spans must not overlap.

%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75


global base64_encoded_length:function
global base64_decoded_max_length:function
global base64_encode:function
global base64_decode:function


section .text


; ============================================================
; base64_encoded_length
; ============================================================

base64_encoded_length:
    ; q = n / 3, rem = n % 3
    mov rax, rdi
    xor edx, edx
    mov ecx, 3
    div rcx

    mov r8, rdx                    ; remainder

    ; q * 4, checked.
    mov ecx, 4
    mul rcx
    test rdx, rdx
    jnz .encoded_length_overflow

    test r8, r8
    jz .encoded_length_success

    add rax, 4
    jc .encoded_length_overflow

.encoded_length_success:
    mov rdx, rax
    xor eax, eax
    ret

.encoded_length_overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; base64_decoded_max_length
;
; Safe capacity upper bound for an encoded span of arbitrary
; length: 3 * ceil(encoded_length / 4).
; Strict decode may reject a non-multiple-of-four input later.
; ============================================================

base64_decoded_max_length:
    mov rax, rdi
    xor edx, edx
    mov ecx, 4
    div rcx

    test rdx, rdx
    jz .decoded_max_groups_ready
    inc rax

.decoded_max_groups_ready:
    ; ceil(n/4) <= 2^62, therefore *3 is representable in u64.
    lea rdx, [rax + rax * 2]
    xor eax, eax
    ret


; ============================================================
; base64_encode
; ============================================================

base64_encode:
    mov r8, rdx                    ; destination
    mov r9, rsi                    ; source length
    mov r10, rcx                   ; capacity

    ; Calculate exact encoded length without dereferencing source.
    mov rax, r9
    xor edx, edx
    mov ecx, 3
    div rcx
    mov r11, rdx                   ; remainder

    mov ecx, 4
    mul rcx
    test rdx, rdx
    jnz .encode_overflow

    test r11, r11
    jz .encode_required_ready
    add rax, 4
    jc .encode_overflow

.encode_required_ready:
    mov r11, rax                   ; exact required length
    cmp r10, r11
    jb .encode_no_space

    test r9, r9
    jz .encode_success

    xor r10d, r10d                 ; source index
    xor ecx, ecx                   ; destination index

.encode_full_loop:
    mov rax, r9
    sub rax, r10
    cmp rax, 3
    jb .encode_tail

    ; Pack three source bytes into EDX bits 23..0.
    movzx edx, byte [rdi + r10]
    shl edx, 16
    movzx eax, byte [rdi + r10 + 1]
    shl eax, 8
    or edx, eax
    movzx eax, byte [rdi + r10 + 2]
    or edx, eax

    mov eax, edx
    shr eax, 18
    and eax, 0x3F
    call .encode_char
    mov [r8 + rcx], al
    inc rcx

    mov eax, edx
    shr eax, 12
    and eax, 0x3F
    call .encode_char
    mov [r8 + rcx], al
    inc rcx

    mov eax, edx
    shr eax, 6
    and eax, 0x3F
    call .encode_char
    mov [r8 + rcx], al
    inc rcx

    mov eax, edx
    and eax, 0x3F
    call .encode_char
    mov [r8 + rcx], al
    inc rcx

    add r10, 3
    jmp .encode_full_loop

.encode_tail:
    test rax, rax
    jz .encode_success

    cmp rax, 1
    je .encode_tail_one

    ; Two remaining bytes -> three symbols plus '='.
    movzx edx, byte [rdi + r10]
    shl edx, 8
    movzx eax, byte [rdi + r10 + 1]
    or edx, eax

    mov eax, edx
    shr eax, 10
    and eax, 0x3F
    call .encode_char
    mov [r8 + rcx], al
    inc rcx

    mov eax, edx
    shr eax, 4
    and eax, 0x3F
    call .encode_char
    mov [r8 + rcx], al
    inc rcx

    mov eax, edx
    shl eax, 2
    and eax, 0x3F
    call .encode_char
    mov [r8 + rcx], al
    inc rcx

    mov byte [r8 + rcx], '='
    inc rcx
    jmp .encode_success

.encode_tail_one:
    ; One remaining byte -> two symbols plus '=='.
    movzx edx, byte [rdi + r10]

    mov eax, edx
    shr eax, 2
    and eax, 0x3F
    call .encode_char
    mov [r8 + rcx], al
    inc rcx

    mov eax, edx
    shl eax, 4
    and eax, 0x3F
    call .encode_char
    mov [r8 + rcx], al
    inc rcx

    mov byte [r8 + rcx], '='
    inc rcx
    mov byte [r8 + rcx], '='
    inc rcx

.encode_success:
    mov rdx, r11
    xor eax, eax
    ret

.encode_overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret

.encode_no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    ret


; EAX sextet 0..63 -> canonical Base64 byte in AL.
.encode_char:
    cmp eax, 25
    jbe .encode_char_upper
    cmp eax, 51
    jbe .encode_char_lower
    cmp eax, 61
    jbe .encode_char_digit
    cmp eax, 62
    je .encode_char_plus
    mov eax, '/'
    ret

.encode_char_upper:
    add eax, 'A'
    ret

.encode_char_lower:
    add eax, 'a' - 26
    ret

.encode_char_digit:
    add eax, '0' - 52
    ret

.encode_char_plus:
    mov eax, '+'
    ret


; ============================================================
; base64_decode
; ============================================================

base64_decode:
    mov r8, rdx                    ; destination
    mov r9, rsi                    ; encoded length
    mov r10, rcx                   ; capacity

    test r9, r9
    jz .decode_empty

    ; Strict RFC 4648 Base64 is a sequence of complete quartets.
    test r9, 3
    jnz .decode_invalid

    ; Validate every non-final quartet: no padding allowed there.
    xor r11d, r11d                 ; source index
    mov rax, r9
    sub rax, 4                     ; final quartet offset

.validate_nonfinal:
    cmp r11, rax
    jae .validate_final

    movzx eax, byte [rdi + r11]
    call .decode_char
    jc .decode_invalid

    movzx eax, byte [rdi + r11 + 1]
    call .decode_char
    jc .decode_invalid

    movzx eax, byte [rdi + r11 + 2]
    call .decode_char
    jc .decode_invalid

    movzx eax, byte [rdi + r11 + 3]
    call .decode_char
    jc .decode_invalid

    add r11, 4
    mov rax, r9
    sub rax, 4
    jmp .validate_nonfinal

.validate_final:
    ; Final quartet begins at R11.
    movzx eax, byte [rdi + r11]
    call .decode_char
    jc .decode_invalid
    mov ecx, eax                   ; v0

    movzx eax, byte [rdi + r11 + 1]
    call .decode_char
    jc .decode_invalid
    mov edx, eax                   ; v1

    movzx eax, byte [rdi + r11 + 2]
    cmp eax, '='
    je .final_double_padding

    call .decode_char
    jc .decode_invalid
    mov esi, eax                   ; v2

    movzx eax, byte [rdi + r11 + 3]
    cmp eax, '='
    je .final_single_padding

    call .decode_char
    jc .decode_invalid

    ; No padding: exact decoded length = groups * 3.
    mov rax, r9
    shr rax, 2
    lea rsi, [rax + rax * 2]
    jmp .decode_capacity_ready

.final_double_padding:
    ; xx== is valid only if v1 low four bits are zero.
    cmp byte [rdi + r11 + 3], '='
    jne .decode_invalid
    test edx, 0x0F
    jnz .decode_invalid

    mov rax, r9
    shr rax, 2
    lea rsi, [rax + rax * 2]
    sub rsi, 2
    jmp .decode_capacity_ready

.final_single_padding:
    ; xxx= is canonical only if v2 low two bits are zero.
    test esi, 0x03
    jnz .decode_invalid

    mov rax, r9
    shr rax, 2
    lea rsi, [rax + rax * 2]
    dec rsi

.decode_capacity_ready:
    cmp r10, rsi
    jb .decode_no_space

    ; Decode all complete quartets. R10 becomes source index;
    ; R11 becomes output index. RSI retains exact output length.
    xor r10d, r10d
    xor r11d, r11d

.decode_quartet_loop:
    cmp r10, r9
    jae .decode_success

    ; v0
    movzx eax, byte [rdi + r10]
    call .decode_char
    mov ecx, eax
    shl ecx, 18

    ; v1
    movzx eax, byte [rdi + r10 + 1]
    call .decode_char
    shl eax, 12
    or ecx, eax

    ; v2 or padding.
    movzx eax, byte [rdi + r10 + 2]
    cmp eax, '='
    je .decode_write_one
    call .decode_char
    shl eax, 6
    or ecx, eax

    ; v3 or padding.
    movzx eax, byte [rdi + r10 + 3]
    cmp eax, '='
    je .decode_write_two
    call .decode_char
    or ecx, eax

    mov eax, ecx
    shr eax, 16
    mov [r8 + r11], al
    mov eax, ecx
    shr eax, 8
    mov [r8 + r11 + 1], al
    mov [r8 + r11 + 2], cl

    add r11, 3
    add r10, 4
    jmp .decode_quartet_loop

.decode_write_two:
    mov eax, ecx
    shr eax, 16
    mov [r8 + r11], al
    mov eax, ecx
    shr eax, 8
    mov [r8 + r11 + 1], al
    add r11, 2
    add r10, 4
    jmp .decode_quartet_loop

.decode_write_one:
    mov eax, ecx
    shr eax, 16
    mov [r8 + r11], al
    inc r11
    add r10, 4
    jmp .decode_quartet_loop

.decode_empty:
    xor edx, edx
    xor eax, eax
    ret

.decode_success:
    mov rdx, rsi
    xor eax, eax
    ret

.decode_invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

.decode_no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    ret


; EAX ASCII input -> EAX sextet 0..63; CF=1 invalid.
.decode_char:
    cmp eax, 'A'
    jb .decode_char_lower
    cmp eax, 'Z'
    jbe .decode_char_upper_ok

.decode_char_lower:
    cmp eax, 'a'
    jb .decode_char_digit
    cmp eax, 'z'
    jbe .decode_char_lower_ok

.decode_char_digit:
    cmp eax, '0'
    jb .decode_char_symbols
    cmp eax, '9'
    jbe .decode_char_digit_ok

.decode_char_symbols:
    cmp eax, '+'
    je .decode_char_plus
    cmp eax, '/'
    je .decode_char_slash
    stc
    ret

.decode_char_upper_ok:
    sub eax, 'A'
    clc
    ret

.decode_char_lower_ok:
    sub eax, 'a'
    add eax, 26
    clc
    ret

.decode_char_digit_ok:
    sub eax, '0'
    add eax, 52
    clc
    ret

.decode_char_plus:
    mov eax, 62
    clc
    ret

.decode_char_slash:
    mov eax, 63
    clc
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
