; Arborcore RFC 3986 percent codec
;
; Unreserved bytes are emitted literally:
;   ALPHA / DIGIT / "-" / "." / "_" / "~"
;
; All other bytes encode as uppercase %HH.
; Decoding accepts uppercase and lowercase hexadecimal.
;
; percent_encoded_length(source, source_length)
;   RDI = source address
;   RSI = source length
;   RAX = 0 / -EOVERFLOW
;   RDX = encoded length on success, 0 on error
;
; percent_encode / percent_decode arguments:
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
; Encode/decode validate required capacity/syntax before writes.
; Output is a byte span: no implicit NUL terminator.
; Source and destination spans must not overlap.

%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75


global percent_encoded_length:function
global percent_encode:function
global percent_decode:function


section .text


; ============================================================
; percent_encoded_length
; ============================================================

percent_encoded_length:
    xor edx, edx                   ; required length
    xor ecx, ecx                   ; source index

.length_loop:
    cmp rcx, rsi
    jae .length_success

    movzx eax, byte [rdi + rcx]

    cmp eax, '0'
    jb .length_check_upper
    cmp eax, '9'
    jbe .length_unreserved

.length_check_upper:
    cmp eax, 'A'
    jb .length_check_lower
    cmp eax, 'Z'
    jbe .length_unreserved

.length_check_lower:
    cmp eax, 'a'
    jb .length_check_punct
    cmp eax, 'z'
    jbe .length_unreserved

.length_check_punct:
    cmp eax, '-'
    je .length_unreserved
    cmp eax, '.'
    je .length_unreserved
    cmp eax, '_'
    je .length_unreserved
    cmp eax, '~'
    je .length_unreserved

    add rdx, 3
    jc .length_overflow
    jmp .length_next

.length_unreserved:
    add rdx, 1
    jc .length_overflow

.length_next:
    inc rcx
    jmp .length_loop

.length_success:
    xor eax, eax
    ret

.length_overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret


; ============================================================
; percent_encode
; ============================================================

percent_encode:
    mov r8, rdx                    ; destination
    mov r9, rsi                    ; source length
    mov r10, rcx                   ; capacity

    ; Preflight required encoded length.
    xor r11d, r11d                 ; required length
    xor esi, esi                   ; source index

.encode_length_loop:
    cmp rsi, r9
    jae .encode_length_ready

    movzx eax, byte [rdi + rsi]

    cmp eax, '0'
    jb .encode_length_upper
    cmp eax, '9'
    jbe .encode_length_unreserved

.encode_length_upper:
    cmp eax, 'A'
    jb .encode_length_lower
    cmp eax, 'Z'
    jbe .encode_length_unreserved

.encode_length_lower:
    cmp eax, 'a'
    jb .encode_length_punct
    cmp eax, 'z'
    jbe .encode_length_unreserved

.encode_length_punct:
    cmp eax, '-'
    je .encode_length_unreserved
    cmp eax, '.'
    je .encode_length_unreserved
    cmp eax, '_'
    je .encode_length_unreserved
    cmp eax, '~'
    je .encode_length_unreserved

    add r11, 3
    jc .encode_overflow
    jmp .encode_length_next

.encode_length_unreserved:
    add r11, 1
    jc .encode_overflow

.encode_length_next:
    inc rsi
    jmp .encode_length_loop

.encode_length_ready:
    cmp r10, r11
    jb .encode_no_space

    test r9, r9
    jz .encode_success

    xor esi, esi                   ; source index
    xor ecx, ecx                   ; destination index

.encode_loop:
    movzx eax, byte [rdi + rsi]

    ; Same unreserved classification.
    cmp eax, '0'
    jb .encode_upper
    cmp eax, '9'
    jbe .encode_literal

.encode_upper:
    cmp eax, 'A'
    jb .encode_lower
    cmp eax, 'Z'
    jbe .encode_literal

.encode_lower:
    cmp eax, 'a'
    jb .encode_punct
    cmp eax, 'z'
    jbe .encode_literal

.encode_punct:
    cmp eax, '-'
    je .encode_literal
    cmp eax, '.'
    je .encode_literal
    cmp eax, '_'
    je .encode_literal
    cmp eax, '~'
    je .encode_literal

    ; Reserved byte -> %HH with uppercase hexadecimal.
    mov edx, eax
    mov byte [r8 + rcx], '%'
    inc rcx

    mov eax, edx
    shr eax, 4
    and eax, 0x0F
    cmp eax, 9
    jbe .encode_high_numeric
    add eax, 'A' - 10
    jmp .encode_high_ready

.encode_high_numeric:
    add eax, '0'

.encode_high_ready:
    mov [r8 + rcx], al
    inc rcx

    mov eax, edx
    and eax, 0x0F
    cmp eax, 9
    jbe .encode_low_numeric
    add eax, 'A' - 10
    jmp .encode_low_ready

.encode_low_numeric:
    add eax, '0'

.encode_low_ready:
    mov [r8 + rcx], al
    inc rcx
    jmp .encode_next

.encode_literal:
    mov [r8 + rcx], al
    inc rcx

.encode_next:
    inc rsi
    cmp rsi, r9
    jb .encode_loop

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


; ============================================================
; percent_decode
; ============================================================

percent_decode:
    mov r8, rdx                    ; destination
    mov r9, rsi                    ; source length
    mov r10, rcx                   ; capacity

    ; First pass validates complete syntax and computes exact output.
    xor r11d, r11d                 ; source index
    xor esi, esi                   ; decoded length

.decode_validate_loop:
    cmp r11, r9
    jae .decode_validated

    movzx eax, byte [rdi + r11]
    cmp eax, '%'
    jne .decode_validate_literal

    ; Need two bytes after '%'.
    mov rax, r9
    sub rax, r11
    cmp rax, 3
    jb .decode_invalid

    movzx eax, byte [rdi + r11 + 1]
    call .nibble
    jc .decode_invalid

    movzx eax, byte [rdi + r11 + 2]
    call .nibble
    jc .decode_invalid

    add r11, 3
    inc rsi
    jmp .decode_validate_loop

.decode_validate_literal:
    inc r11
    inc rsi
    jmp .decode_validate_loop

.decode_validated:
    cmp r10, rsi
    jb .decode_no_space

    test r9, r9
    jz .decode_success

    xor r11d, r11d                 ; source index
    xor r10d, r10d                 ; destination index (capacity no longer needed)

.decode_loop:
    cmp r11, r9
    jae .decode_success

    movzx eax, byte [rdi + r11]
    cmp eax, '%'
    jne .decode_literal

    movzx eax, byte [rdi + r11 + 1]
    call .nibble
    shl eax, 4
    mov edx, eax

    movzx eax, byte [rdi + r11 + 2]
    call .nibble
    or edx, eax

    mov [r8 + r10], dl
    inc r10
    add r11, 3
    jmp .decode_loop

.decode_literal:
    mov [r8 + r10], al
    inc r10
    inc r11
    jmp .decode_loop

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


; ASCII hexadecimal byte -> nibble.
; EAX input, EAX output; CF=1 invalid.
.nibble:
    mov ecx, eax
    sub ecx, '0'
    cmp ecx, 9
    jbe .nibble_digit

    or eax, 0x20
    sub eax, 'a'
    cmp eax, 5
    ja .nibble_invalid
    add eax, 10
    clc
    ret

.nibble_digit:
    mov eax, ecx
    clc
    ret

.nibble_invalid:
    stc
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
