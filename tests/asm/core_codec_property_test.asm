; Arborcore Core Retrofit B1 codec-property qualification
;
; Exit status:
;   0 = all properties passed
;   1 = hexadecimal exhaustive round-trip/reference failed
;   2 = percent exhaustive classification/round-trip failed
;   3 = Base64 length algebra failed
;   4 = Base64 one-/two-byte canonical round-trip failed

%define SYS_EXIT 60

global _start

extern bytes_encode_hex
extern bytes_decode_hex
extern percent_encoded_length
extern percent_encode
extern percent_decode
extern base64_encoded_length
extern base64_decoded_max_length
extern base64_encode
extern base64_decode

section .bss
align 16
source2:      resb 2
encoded:      resb 16
decoded:      resb 4
reencoded:    resb 16

section .text

_start:
    cld

    call .test_hex
    test eax, eax
    jnz .fail_hex

    call .test_percent
    test eax, eax
    jnz .fail_percent

    call .test_base64_lengths
    test eax, eax
    jnz .fail_b64_length

    call .test_base64_roundtrip
    test eax, eax
    jnz .fail_b64_roundtrip

    xor edi, edi
    jmp .exit
.fail_hex:
    mov edi, 1
    jmp .exit
.fail_percent:
    mov edi, 2
    jmp .exit
.fail_b64_length:
    mov edi, 3
    jmp .exit
.fail_b64_roundtrip:
    mov edi, 4
.exit:
    mov eax, SYS_EXIT
    syscall

; ------------------------------------------------------------
; All 65,536 ordered two-byte spans. Verify canonical lowercase
; hexadecimal against independent nibble conversion, then decode.
; ------------------------------------------------------------
.test_hex:
    xor r12d, r12d
.hex_loop:
    mov [rel source2], r12b
    mov eax, r12d
    shr eax, 8
    mov [rel source2 + 1], al

    lea rdi, [rel source2]
    mov esi, 2
    lea rdx, [rel encoded]
    mov ecx, 4
    call bytes_encode_hex
    test rax, rax
    jnz .hex_fail
    cmp rdx, 4
    jne .hex_fail

    movzx edi, byte [rel source2]
    call .hex_char_pair
    cmp byte [rel encoded], al
    jne .hex_fail
    cmp byte [rel encoded + 1], ah
    jne .hex_fail

    movzx edi, byte [rel source2 + 1]
    call .hex_char_pair
    cmp byte [rel encoded + 2], al
    jne .hex_fail
    cmp byte [rel encoded + 3], ah
    jne .hex_fail

    lea rdi, [rel encoded]
    mov esi, 4
    lea rdx, [rel decoded]
    mov ecx, 2
    call bytes_decode_hex
    test rax, rax
    jnz .hex_fail
    cmp rdx, 2
    jne .hex_fail

    mov ax, [rel source2]
    cmp ax, [rel decoded]
    jne .hex_fail

    inc r12d
    cmp r12d, 65536
    jb .hex_loop

    xor eax, eax
    ret
.hex_fail:
    mov eax, 1
    ret

; ------------------------------------------------------------
; Exhaust every single byte against the RFC 3986 unreserved set,
; then all 65,536 two-byte spans for compositional round-trip.
; ------------------------------------------------------------
.test_percent:
    xor r12d, r12d
.percent_single:
    mov [rel source2], r12b

    mov edi, r12d
    call .is_unreserved_ref
    mov r13d, eax

    lea rdi, [rel source2]
    mov esi, 1
    call percent_encoded_length
    test rax, rax
    jnz .percent_fail
    mov r14d, 3
    test r13d, r13d
    jz .percent_length_ready
    mov r14d, 1
.percent_length_ready:
    cmp rdx, r14
    jne .percent_fail

    lea rdi, [rel source2]
    mov esi, 1
    lea rdx, [rel encoded]
    mov ecx, 3
    call percent_encode
    test rax, rax
    jnz .percent_fail
    cmp rdx, r14
    jne .percent_fail

    test r13d, r13d
    jz .percent_reserved_check
    mov al, [rel source2]
    cmp al, [rel encoded]
    jne .percent_fail
    jmp .percent_decode_single

.percent_reserved_check:
    cmp byte [rel encoded], '%'
    jne .percent_fail
    movzx edi, byte [rel source2]
    call .hex_char_pair_upper
    cmp byte [rel encoded + 1], al
    jne .percent_fail
    cmp byte [rel encoded + 2], ah
    jne .percent_fail

.percent_decode_single:
    lea rdi, [rel encoded]
    mov rsi, r14
    lea rdx, [rel decoded]
    mov ecx, 1
    call percent_decode
    test rax, rax
    jnz .percent_fail
    cmp rdx, 1
    jne .percent_fail
    mov al, [rel source2]
    cmp al, [rel decoded]
    jne .percent_fail

    inc r12d
    cmp r12d, 256
    jb .percent_single

    xor r12d, r12d
.percent_pair:
    mov [rel source2], r12b
    mov eax, r12d
    shr eax, 8
    mov [rel source2 + 1], al

    lea rdi, [rel source2]
    mov esi, 2
    lea rdx, [rel encoded]
    mov ecx, 6
    call percent_encode
    test rax, rax
    jnz .percent_fail
    mov r13, rdx

    lea rdi, [rel encoded]
    mov rsi, r13
    lea rdx, [rel decoded]
    mov ecx, 2
    call percent_decode
    test rax, rax
    jnz .percent_fail
    cmp rdx, 2
    jne .percent_fail
    mov ax, [rel source2]
    cmp ax, [rel decoded]
    jne .percent_fail

    inc r12d
    cmp r12d, 65536
    jb .percent_pair

    xor eax, eax
    ret
.percent_fail:
    mov eax, 1
    ret

; ------------------------------------------------------------
; Base64 exact/max length equations over n=0..4096.
; ------------------------------------------------------------
.test_base64_lengths:
    xor r12d, r12d
.b64_len_loop:
    mov rdi, r12
    call base64_encoded_length
    test rax, rax
    jnz .b64_len_fail
    mov r13, rdx

    ; 4 * ceil(n/3), reference computed without production helper.
    mov rax, r12
    xor edx, edx
    mov ecx, 3
    div rcx
    test rdx, rdx
    jz .b64_enc_groups_ready
    inc rax
.b64_enc_groups_ready:
    shl rax, 2
    cmp r13, rax
    jne .b64_len_fail

    mov rdi, r12
    call base64_decoded_max_length
    test rax, rax
    jnz .b64_len_fail
    mov r13, rdx

    ; 3 * ceil(n/4).
    mov rax, r12
    xor edx, edx
    mov ecx, 4
    div rcx
    test rdx, rdx
    jz .b64_dec_groups_ready
    inc rax
.b64_dec_groups_ready:
    lea rax, [rax + rax * 2]
    cmp r13, rax
    jne .b64_len_fail

    inc r12d
    cmp r12d, 4097
    jb .b64_len_loop

    xor eax, eax
    ret
.b64_len_fail:
    mov eax, 1
    ret

; ------------------------------------------------------------
; Canonical Base64 round-trip for all one-byte and all ordered
; two-byte inputs. Re-encoding the decoded bytes must reproduce
; exactly the canonical encoded representation.
; ------------------------------------------------------------
.test_base64_roundtrip:
    xor r12d, r12d
.b64_one_loop:
    mov [rel source2], r12b
    mov esi, 1
    call .b64_roundtrip_case
    test eax, eax
    jnz .b64_roundtrip_fail
    inc r12d
    cmp r12d, 256
    jb .b64_one_loop

    xor r12d, r12d
.b64_two_loop:
    mov [rel source2], r12b
    mov eax, r12d
    shr eax, 8
    mov [rel source2 + 1], al
    mov esi, 2
    call .b64_roundtrip_case
    test eax, eax
    jnz .b64_roundtrip_fail
    inc r12d
    cmp r12d, 65536
    jb .b64_two_loop

    xor eax, eax
    ret
.b64_roundtrip_fail:
    mov eax, 1
    ret

; ESI=source length 1 or 2. EAX=0 pass / 1 fail.
.b64_roundtrip_case:
    mov r14, rsi

    lea rdi, [rel source2]
    mov rsi, r14
    lea rdx, [rel encoded]
    mov ecx, 8
    call base64_encode
    test rax, rax
    jnz .b64_case_fail
    cmp rdx, 4
    jne .b64_case_fail
    mov r15, rdx

    lea rdi, [rel encoded]
    mov rsi, r15
    lea rdx, [rel decoded]
    mov rcx, r14
    call base64_decode
    test rax, rax
    jnz .b64_case_fail
    cmp rdx, r14
    jne .b64_case_fail

    xor r8d, r8d
    lea r9, [rel source2]
    lea r10, [rel decoded]
.b64_compare_decoded:
    cmp r8, r14
    jae .b64_reencode
    mov al, [r9 + r8]
    cmp al, [r10 + r8]
    jne .b64_case_fail
    inc r8
    jmp .b64_compare_decoded

.b64_reencode:
    lea rdi, [rel decoded]
    mov rsi, r14
    lea rdx, [rel reencoded]
    mov ecx, 8
    call base64_encode
    test rax, rax
    jnz .b64_case_fail
    cmp rdx, r15
    jne .b64_case_fail

    xor r8d, r8d
    lea r9, [rel encoded]
    lea r10, [rel reencoded]
.b64_compare_encoded:
    cmp r8, r15
    jae .b64_case_pass
    mov al, [r9 + r8]
    cmp al, [r10 + r8]
    jne .b64_case_fail
    inc r8
    jmp .b64_compare_encoded

.b64_case_pass:
    xor eax, eax
    ret
.b64_case_fail:
    mov eax, 1
    ret

; ------------------------------------------------------------
; Reference classifiers/formatters.
; ------------------------------------------------------------
.is_unreserved_ref:
    mov eax, edi
    cmp eax, '0'
    jb .unres_upper
    cmp eax, '9'
    jbe .unres_yes
.unres_upper:
    cmp eax, 'A'
    jb .unres_lower
    cmp eax, 'Z'
    jbe .unres_yes
.unres_lower:
    cmp eax, 'a'
    jb .unres_punct
    cmp eax, 'z'
    jbe .unres_yes
.unres_punct:
    cmp eax, '-'
    je .unres_yes
    cmp eax, '.'
    je .unres_yes
    cmp eax, '_'
    je .unres_yes
    cmp eax, '~'
    je .unres_yes
    xor eax, eax
    ret
.unres_yes:
    mov eax, 1
    ret

; EDI=byte. Return AL=high lowercase hex, AH=low lowercase hex.
.hex_char_pair:
    mov edx, edi
    mov eax, edx
    shr eax, 4
    and eax, 0x0F
    call .nibble_lower
    mov r8b, al
    mov eax, edx
    and eax, 0x0F
    call .nibble_lower
    mov ah, al
    mov al, r8b
    ret

; EDI=byte. Return AL=high uppercase hex, AH=low uppercase hex.
.hex_char_pair_upper:
    mov edx, edi
    mov eax, edx
    shr eax, 4
    and eax, 0x0F
    call .nibble_upper
    mov r8b, al
    mov eax, edx
    and eax, 0x0F
    call .nibble_upper
    mov ah, al
    mov al, r8b
    ret

.nibble_lower:
    cmp eax, 9
    jbe .nibble_lower_num
    add eax, 'a' - 10
    ret
.nibble_lower_num:
    add eax, '0'
    ret

.nibble_upper:
    cmp eax, 9
    jbe .nibble_upper_num
    add eax, 'A' - 10
    ret
.nibble_upper_num:
    add eax, '0'
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
