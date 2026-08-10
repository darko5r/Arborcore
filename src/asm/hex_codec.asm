; Arborcore hexadecimal byte codec
;
; bytes_encode_hex(source, source_length, destination, capacity)
; bytes_decode_hex(source, source_length, destination, capacity)
;
; Arguments:
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
; Encoding uses lowercase hexadecimal and emits no NUL terminator.
; Decoding accepts uppercase and lowercase hexadecimal.
; Capacity/syntax are validated before destination writes.
; Source and destination spans must not overlap.

%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75


global bytes_encode_hex:function
global bytes_decode_hex:function


section .text


; ============================================================
; bytes_encode_hex
; ============================================================

bytes_encode_hex:
    mov r10, rdx                   ; preserve destination

    ; required = source_length * 2, checked.
    mov rax, rsi
    mov r8d, 2
    mul r8
    test rdx, rdx
    jnz .overflow

    mov r8, rax                    ; required length
    cmp rcx, r8
    jb .no_space

    test rsi, rsi
    jz .success

    xor r9d, r9d                   ; source index
    xor r11d, r11d                 ; destination index

.loop:
    movzx eax, byte [rdi + r9]

    ; High nibble.
    mov ecx, eax
    shr ecx, 4
    cmp ecx, 9
    jbe .high_numeric
    add ecx, 'a' - 10
    jmp .high_ready

.high_numeric:
    add ecx, '0'

.high_ready:
    mov [r10 + r11], cl
    inc r11

    ; Low nibble.
    and eax, 0x0F
    cmp eax, 9
    jbe .low_numeric
    add eax, 'a' - 10
    jmp .low_ready

.low_numeric:
    add eax, '0'

.low_ready:
    mov [r10 + r11], al
    inc r11

    inc r9
    cmp r9, rsi
    jb .loop

.success:
    mov rdx, r8
    xor eax, eax
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret

.no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    ret


; ============================================================
; bytes_decode_hex
; ============================================================

bytes_decode_hex:
    mov r10, rdx                   ; destination
    mov r11, rcx                   ; capacity

    ; Hex text must contain complete byte pairs.
    test rsi, 1
    jnz .invalid

    mov r8, rsi
    shr r8, 1                      ; decoded length

    ; First pass: validate the complete source before writing.
    xor r9d, r9d

.validate_loop:
    cmp r9, rsi
    jae .validated

    movzx eax, byte [rdi + r9]
    call .nibble
    jc .invalid

    movzx eax, byte [rdi + r9 + 1]
    call .nibble
    jc .invalid

    add r9, 2
    jmp .validate_loop

.validated:
    cmp r11, r8
    jb .decode_no_space

    test rsi, rsi
    jz .decode_success

    ; Second pass: decode after syntax/capacity are known-good.
    xor r9d, r9d                   ; source index
    xor r11d, r11d                 ; output index (capacity no longer needed)

.decode_loop:
    movzx eax, byte [rdi + r9]
    call .nibble
    ; Validation guarantees success.
    shl eax, 4
    mov edx, eax

    movzx eax, byte [rdi + r9 + 1]
    call .nibble
    or edx, eax

    mov [r10 + r11], dl

    add r9, 2
    inc r11
    cmp r9, rsi
    jb .decode_loop

.decode_success:
    mov rdx, r8
    xor eax, eax
    ret

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

.decode_no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    ret


; Convert ASCII hexadecimal byte in AL/EAX to nibble in EAX.
; Return CF=0 on success, CF=1 on invalid input.
.nibble:
    mov ecx, eax
    sub ecx, '0'
    cmp ecx, 9
    jbe .digit

    or eax, 0x20
    sub eax, 'a'
    cmp eax, 5
    ja .nibble_invalid
    add eax, 10
    clc
    ret

.digit:
    mov eax, ecx
    clc
    ret

.nibble_invalid:
    stc
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
