; Arborcore encoding-engine tests
;
; Exit status:
;   0 = all tests passed
;   1 = bytes_encode_hex failed
;   2 = bytes_decode_hex failed
;   3 = percent_encoded_length failed
;   4 = percent_encode failed
;   5 = percent_decode failed
;   6 = base64_encoded_length failed
;   7 = base64_decoded_max_length failed
;   8 = base64_encode failed
;   9 = base64_decode failed

%define SYS_EXIT        60
%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75


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


section .rodata

hex_binary:
    db 0x00, 0x0F, 0x10, 0xAB, 0xFF
hex_binary_length equ $ - hex_binary

hex_expected:
    db "000f10abff"
hex_expected_length equ $ - hex_expected

hex_mixed_case:
    db "000F10aBfF"
hex_mixed_case_length equ $ - hex_mixed_case

hex_odd:
    db "abc"
hex_odd_length equ $ - hex_odd

hex_invalid:
    db "00g0"
hex_invalid_length equ $ - hex_invalid


percent_unreserved:
    db "AZaz09-._~"
percent_unreserved_length equ $ - percent_unreserved

percent_source:
    db "a b/%~"
percent_source_length equ $ - percent_source

percent_expected:
    db "a%20b%2F%25~"
percent_expected_length equ $ - percent_expected

percent_binary:
    db 0x00, 0xFF
percent_binary_length equ $ - percent_binary

percent_binary_expected:
    db "%00%FF"
percent_binary_expected_length equ $ - percent_binary_expected

percent_binary_encoded_lower:
    db "%00%ff"
percent_binary_encoded_lower_length equ $ - percent_binary_encoded_lower

percent_invalid_short1:
    db "%"
percent_invalid_short1_length equ $ - percent_invalid_short1

percent_invalid_short2:
    db "%2"
percent_invalid_short2_length equ $ - percent_invalid_short2

percent_invalid_hex:
    db "%GG"
percent_invalid_hex_length equ $ - percent_invalid_hex

percent_literal_plain:
    db "plain"
percent_literal_plain_length equ $ - percent_literal_plain

; RFC 3986 percent decoding is not form decoding.
percent_plus_literal:
    db "+"
percent_plus_literal_length equ $ - percent_plus_literal

percent_plus_encoded:
    db "%2B"
percent_plus_encoded_length equ $ - percent_plus_encoded


b64_f:
    db "f"
b64_f_length equ $ - b64_f
b64_f_enc:
    db "Zg=="
b64_f_enc_length equ $ - b64_f_enc

b64_fo:
    db "fo"
b64_fo_length equ $ - b64_fo
b64_fo_enc:
    db "Zm8="
b64_fo_enc_length equ $ - b64_fo_enc

b64_foo:
    db "foo"
b64_foo_length equ $ - b64_foo
b64_foo_enc:
    db "Zm9v"
b64_foo_enc_length equ $ - b64_foo_enc

b64_foob:
    db "foob"
b64_foob_length equ $ - b64_foob
b64_foob_enc:
    db "Zm9vYg=="
b64_foob_enc_length equ $ - b64_foob_enc

b64_fooba:
    db "fooba"
b64_fooba_length equ $ - b64_fooba
b64_fooba_enc:
    db "Zm9vYmE="
b64_fooba_enc_length equ $ - b64_fooba_enc

b64_foobar:
    db "foobar"
b64_foobar_length equ $ - b64_foobar
b64_foobar_enc:
    db "Zm9vYmFy"
b64_foobar_enc_length equ $ - b64_foobar_enc

b64_symbols_binary:
    db 0xFB, 0xFF, 0xFF
b64_symbols_binary_length equ $ - b64_symbols_binary
b64_symbols_encoded:
    db "+///"
b64_symbols_encoded_length equ $ - b64_symbols_encoded

b64_bad_length:
    db "Zg="
b64_bad_length_length equ $ - b64_bad_length

b64_bad_char:
    db "Zm$="
b64_bad_char_length equ $ - b64_bad_char

b64_bad_padding_first:
    db "=m9v"
b64_bad_padding_first_length equ $ - b64_bad_padding_first

b64_bad_padding_nonfinal:
    db "Zg==AAAA"
b64_bad_padding_nonfinal_length equ $ - b64_bad_padding_nonfinal

b64_bad_padding_shape:
    db "Zg=A"
b64_bad_padding_shape_length equ $ - b64_bad_padding_shape

; Noncanonical xx==: second sextet has nonzero low four tail bits.
b64_bad_tail_double:
    db "Zh=="
b64_bad_tail_double_length equ $ - b64_bad_tail_double

; Noncanonical xxx=: third sextet has nonzero low two tail bits.
b64_bad_tail_single:
    db "Zm9="
b64_bad_tail_single_length equ $ - b64_bad_tail_single


section .bss

hex_buffer:
    resb 64

percent_buffer:
    resb 128

base64_buffer:
    resb 128


section .text


_start:

    ; ========================================================
    ; bytes_encode_hex
    ; ========================================================

    ; Empty span is NULL-safe.
    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call bytes_encode_hex
    test rax, rax
    jnz .fail_hex_encode
    test rdx, rdx
    jnz .fail_hex_encode

    ; Binary bytes -> canonical lowercase hexadecimal.
    lea rdi, [rel hex_binary]
    mov esi, hex_binary_length
    lea rdx, [rel hex_buffer]
    mov ecx, hex_expected_length
    call bytes_encode_hex
    test rax, rax
    jnz .fail_hex_encode
    cmp rdx, hex_expected_length
    jne .fail_hex_encode
    lea rdi, [rel hex_buffer]
    lea rsi, [rel hex_expected]
    mov edx, hex_expected_length
    call .bytes_match
    cmp eax, 1
    jne .fail_hex_encode

    ; Excess capacity: no implicit NUL/padding after the span.
    mov byte [rel hex_buffer + hex_expected_length], 0xA5
    mov byte [rel hex_buffer + hex_expected_length + 1], 0x5A
    lea rdi, [rel hex_binary]
    mov esi, hex_binary_length
    lea rdx, [rel hex_buffer]
    mov ecx, 64
    call bytes_encode_hex
    test rax, rax
    jnz .fail_hex_encode
    cmp rdx, hex_expected_length
    jne .fail_hex_encode
    cmp byte [rel hex_buffer + hex_expected_length], 0xA5
    jne .fail_hex_encode
    cmp byte [rel hex_buffer + hex_expected_length + 1], 0x5A
    jne .fail_hex_encode

    ; ENOSPC must occur before writes.
    mov byte [rel hex_buffer], 0xA5
    lea rdi, [rel hex_binary]
    mov esi, hex_binary_length
    lea rdx, [rel hex_buffer]
    mov ecx, hex_expected_length - 1
    call bytes_encode_hex
    cmp rax, ERR_ENOSPC
    jne .fail_hex_encode
    test rdx, rdx
    jnz .fail_hex_encode
    cmp byte [rel hex_buffer], 0xA5
    jne .fail_hex_encode

    ; Length overflow is detected before source dereference.
    xor edi, edi
    mov rsi, 0x8000000000000000
    xor edx, edx
    mov rcx, -1
    call bytes_encode_hex
    cmp rax, ERR_EOVERFLOW
    jne .fail_hex_encode
    test rdx, rdx
    jnz .fail_hex_encode


    ; ========================================================
    ; bytes_decode_hex
    ; ========================================================

    ; Empty span.
    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call bytes_decode_hex
    test rax, rax
    jnz .fail_hex_decode
    test rdx, rdx
    jnz .fail_hex_decode

    ; Mixed-case decoding.
    lea rdi, [rel hex_mixed_case]
    mov esi, hex_mixed_case_length
    lea rdx, [rel hex_buffer]
    mov ecx, hex_binary_length
    call bytes_decode_hex
    test rax, rax
    jnz .fail_hex_decode
    cmp rdx, hex_binary_length
    jne .fail_hex_decode
    lea rdi, [rel hex_buffer]
    lea rsi, [rel hex_binary]
    mov edx, hex_binary_length
    call .bytes_match
    cmp eax, 1
    jne .fail_hex_decode

    ; Excess capacity must remain untouched after decoded output.
    mov byte [rel hex_buffer + hex_binary_length], 0xA5
    mov byte [rel hex_buffer + hex_binary_length + 1], 0x5A
    lea rdi, [rel hex_mixed_case]
    mov esi, hex_mixed_case_length
    lea rdx, [rel hex_buffer]
    mov ecx, 64
    call bytes_decode_hex
    test rax, rax
    jnz .fail_hex_decode
    cmp rdx, hex_binary_length
    jne .fail_hex_decode
    cmp byte [rel hex_buffer + hex_binary_length], 0xA5
    jne .fail_hex_decode
    cmp byte [rel hex_buffer + hex_binary_length + 1], 0x5A
    jne .fail_hex_decode

    ; Odd length is invalid and NULL-safe before dereference.
    xor edi, edi
    mov esi, 1
    xor edx, edx
    xor ecx, ecx
    call bytes_decode_hex
    cmp rax, ERR_EINVAL
    jne .fail_hex_decode
    test rdx, rdx
    jnz .fail_hex_decode

    ; Invalid hexadecimal must not write.
    mov byte [rel hex_buffer], 0xA5
    lea rdi, [rel hex_invalid]
    mov esi, hex_invalid_length
    lea rdx, [rel hex_buffer]
    mov ecx, 64
    call bytes_decode_hex
    cmp rax, ERR_EINVAL
    jne .fail_hex_decode
    test rdx, rdx
    jnz .fail_hex_decode
    cmp byte [rel hex_buffer], 0xA5
    jne .fail_hex_decode

    ; ENOSPC must not write.
    mov byte [rel hex_buffer], 0xA5
    lea rdi, [rel hex_expected]
    mov esi, hex_expected_length
    lea rdx, [rel hex_buffer]
    mov ecx, hex_binary_length - 1
    call bytes_decode_hex
    cmp rax, ERR_ENOSPC
    jne .fail_hex_decode
    test rdx, rdx
    jnz .fail_hex_decode
    cmp byte [rel hex_buffer], 0xA5
    jne .fail_hex_decode


    ; ========================================================
    ; percent_encoded_length
    ; ========================================================

    xor edi, edi
    xor esi, esi
    call percent_encoded_length
    test rax, rax
    jnz .fail_percent_length
    test rdx, rdx
    jnz .fail_percent_length

    lea rdi, [rel percent_unreserved]
    mov esi, percent_unreserved_length
    call percent_encoded_length
    test rax, rax
    jnz .fail_percent_length
    cmp rdx, percent_unreserved_length
    jne .fail_percent_length

    lea rdi, [rel percent_source]
    mov esi, percent_source_length
    call percent_encoded_length
    test rax, rax
    jnz .fail_percent_length
    cmp rdx, percent_expected_length
    jne .fail_percent_length

    lea rdi, [rel percent_binary]
    mov esi, percent_binary_length
    call percent_encoded_length
    test rax, rax
    jnz .fail_percent_length
    cmp rdx, percent_binary_expected_length
    jne .fail_percent_length


    ; ========================================================
    ; percent_encode
    ; ========================================================

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call percent_encode
    test rax, rax
    jnz .fail_percent_encode
    test rdx, rdx
    jnz .fail_percent_encode

    ; All RFC 3986 unreserved bytes in the sample remain literal.
    lea rdi, [rel percent_unreserved]
    mov esi, percent_unreserved_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_unreserved_length
    call percent_encode
    test rax, rax
    jnz .fail_percent_encode
    cmp rdx, percent_unreserved_length
    jne .fail_percent_encode
    lea rdi, [rel percent_buffer]
    lea rsi, [rel percent_unreserved]
    mov edx, percent_unreserved_length
    call .bytes_match
    cmp eax, 1
    jne .fail_percent_encode

    lea rdi, [rel percent_source]
    mov esi, percent_source_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_expected_length
    call percent_encode
    test rax, rax
    jnz .fail_percent_encode
    cmp rdx, percent_expected_length
    jne .fail_percent_encode
    lea rdi, [rel percent_buffer]
    lea rsi, [rel percent_expected]
    mov edx, percent_expected_length
    call .bytes_match
    cmp eax, 1
    jne .fail_percent_encode

    ; Binary values and uppercase percent hexadecimal.
    lea rdi, [rel percent_binary]
    mov esi, percent_binary_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_binary_expected_length
    call percent_encode
    test rax, rax
    jnz .fail_percent_encode
    cmp rdx, percent_binary_expected_length
    jne .fail_percent_encode
    lea rdi, [rel percent_buffer]
    lea rsi, [rel percent_binary_expected]
    mov edx, percent_binary_expected_length
    call .bytes_match
    cmp eax, 1
    jne .fail_percent_encode

    ; Excess capacity leaves bytes after output untouched.
    mov byte [rel percent_buffer + percent_expected_length], 0xA5
    mov byte [rel percent_buffer + percent_expected_length + 1], 0x5A
    lea rdi, [rel percent_source]
    mov esi, percent_source_length
    lea rdx, [rel percent_buffer]
    mov ecx, 128
    call percent_encode
    test rax, rax
    jnz .fail_percent_encode
    cmp rdx, percent_expected_length
    jne .fail_percent_encode
    cmp byte [rel percent_buffer + percent_expected_length], 0xA5
    jne .fail_percent_encode
    cmp byte [rel percent_buffer + percent_expected_length + 1], 0x5A
    jne .fail_percent_encode

    ; ENOSPC before writes.
    mov byte [rel percent_buffer], 0xA5
    lea rdi, [rel percent_source]
    mov esi, percent_source_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_expected_length - 1
    call percent_encode
    cmp rax, ERR_ENOSPC
    jne .fail_percent_encode
    test rdx, rdx
    jnz .fail_percent_encode
    cmp byte [rel percent_buffer], 0xA5
    jne .fail_percent_encode


    ; ========================================================
    ; percent_decode
    ; ========================================================

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call percent_decode
    test rax, rax
    jnz .fail_percent_decode
    test rdx, rdx
    jnz .fail_percent_decode

    lea rdi, [rel percent_expected]
    mov esi, percent_expected_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_source_length
    call percent_decode
    test rax, rax
    jnz .fail_percent_decode
    cmp rdx, percent_source_length
    jne .fail_percent_decode
    lea rdi, [rel percent_buffer]
    lea rsi, [rel percent_source]
    mov edx, percent_source_length
    call .bytes_match
    cmp eax, 1
    jne .fail_percent_decode

    ; Lowercase hexadecimal is accepted during decode.
    lea rdi, [rel percent_binary_encoded_lower]
    mov esi, percent_binary_encoded_lower_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_binary_length
    call percent_decode
    test rax, rax
    jnz .fail_percent_decode
    cmp rdx, percent_binary_length
    jne .fail_percent_decode
    lea rdi, [rel percent_buffer]
    lea rsi, [rel percent_binary]
    mov edx, percent_binary_length
    call .bytes_match
    cmp eax, 1
    jne .fail_percent_decode

    ; Plain text remains literal.
    lea rdi, [rel percent_literal_plain]
    mov esi, percent_literal_plain_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_literal_plain_length
    call percent_decode
    test rax, rax
    jnz .fail_percent_decode
    cmp rdx, percent_literal_plain_length
    jne .fail_percent_decode

    ; Literal '+' remains '+' (RFC 3986, not form-url-encoding).
    lea rdi, [rel percent_plus_literal]
    mov esi, percent_plus_literal_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_plus_literal_length
    call percent_decode
    test rax, rax
    jnz .fail_percent_decode
    cmp rdx, percent_plus_literal_length
    jne .fail_percent_decode
    cmp byte [rel percent_buffer], '+'
    jne .fail_percent_decode

    ; %2B also decodes to literal '+'.
    lea rdi, [rel percent_plus_encoded]
    mov esi, percent_plus_encoded_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_plus_literal_length
    call percent_decode
    test rax, rax
    jnz .fail_percent_decode
    cmp rdx, percent_plus_literal_length
    jne .fail_percent_decode
    cmp byte [rel percent_buffer], '+'
    jne .fail_percent_decode

    ; Excess capacity must remain untouched after decoded output.
    mov byte [rel percent_buffer + percent_source_length], 0xA5
    mov byte [rel percent_buffer + percent_source_length + 1], 0x5A
    lea rdi, [rel percent_expected]
    mov esi, percent_expected_length
    lea rdx, [rel percent_buffer]
    mov ecx, 128
    call percent_decode
    test rax, rax
    jnz .fail_percent_decode
    cmp rdx, percent_source_length
    jne .fail_percent_decode
    cmp byte [rel percent_buffer + percent_source_length], 0xA5
    jne .fail_percent_decode
    cmp byte [rel percent_buffer + percent_source_length + 1], 0x5A
    jne .fail_percent_decode

    ; Truncated/invalid percent triplets must not write.
    mov byte [rel percent_buffer], 0xA5
    lea rdi, [rel percent_invalid_short1]
    mov esi, percent_invalid_short1_length
    lea rdx, [rel percent_buffer]
    mov ecx, 128
    call percent_decode
    cmp rax, ERR_EINVAL
    jne .fail_percent_decode
    test rdx, rdx
    jnz .fail_percent_decode
    cmp byte [rel percent_buffer], 0xA5
    jne .fail_percent_decode

    mov byte [rel percent_buffer], 0xA5
    lea rdi, [rel percent_invalid_short2]
    mov esi, percent_invalid_short2_length
    lea rdx, [rel percent_buffer]
    mov ecx, 128
    call percent_decode
    cmp rax, ERR_EINVAL
    jne .fail_percent_decode
    test rdx, rdx
    jnz .fail_percent_decode
    cmp byte [rel percent_buffer], 0xA5
    jne .fail_percent_decode

    mov byte [rel percent_buffer], 0xA5
    lea rdi, [rel percent_invalid_hex]
    mov esi, percent_invalid_hex_length
    lea rdx, [rel percent_buffer]
    mov ecx, 128
    call percent_decode
    cmp rax, ERR_EINVAL
    jne .fail_percent_decode
    test rdx, rdx
    jnz .fail_percent_decode
    cmp byte [rel percent_buffer], 0xA5
    jne .fail_percent_decode

    ; ENOSPC after syntax validation, before writes.
    mov byte [rel percent_buffer], 0xA5
    lea rdi, [rel percent_expected]
    mov esi, percent_expected_length
    lea rdx, [rel percent_buffer]
    mov ecx, percent_source_length - 1
    call percent_decode
    cmp rax, ERR_ENOSPC
    jne .fail_percent_decode
    test rdx, rdx
    jnz .fail_percent_decode
    cmp byte [rel percent_buffer], 0xA5
    jne .fail_percent_decode


    ; ========================================================
    ; base64_encoded_length
    ; ========================================================

    mov edi, 0
    call base64_encoded_length
    test rax, rax
    jnz .fail_b64_encoded_length
    test rdx, rdx
    jnz .fail_b64_encoded_length

    mov edi, 1
    call base64_encoded_length
    test rax, rax
    jnz .fail_b64_encoded_length
    cmp rdx, 4
    jne .fail_b64_encoded_length

    mov edi, 2
    call base64_encoded_length
    test rax, rax
    jnz .fail_b64_encoded_length
    cmp rdx, 4
    jne .fail_b64_encoded_length

    mov edi, 3
    call base64_encoded_length
    test rax, rax
    jnz .fail_b64_encoded_length
    cmp rdx, 4
    jne .fail_b64_encoded_length

    mov edi, 4
    call base64_encoded_length
    test rax, rax
    jnz .fail_b64_encoded_length
    cmp rdx, 8
    jne .fail_b64_encoded_length

    mov rdi, -1
    call base64_encoded_length
    cmp rax, ERR_EOVERFLOW
    jne .fail_b64_encoded_length
    test rdx, rdx
    jnz .fail_b64_encoded_length


    ; ========================================================
    ; base64_decoded_max_length
    ; ========================================================

    mov edi, 0
    call base64_decoded_max_length
    test rax, rax
    jnz .fail_b64_decoded_max
    test rdx, rdx
    jnz .fail_b64_decoded_max

    mov edi, 1
    call base64_decoded_max_length
    test rax, rax
    jnz .fail_b64_decoded_max
    cmp rdx, 3
    jne .fail_b64_decoded_max

    mov edi, 4
    call base64_decoded_max_length
    test rax, rax
    jnz .fail_b64_decoded_max
    cmp rdx, 3
    jne .fail_b64_decoded_max

    mov edi, 5
    call base64_decoded_max_length
    test rax, rax
    jnz .fail_b64_decoded_max
    cmp rdx, 6
    jne .fail_b64_decoded_max

    mov edi, 8
    call base64_decoded_max_length
    test rax, rax
    jnz .fail_b64_decoded_max
    cmp rdx, 6
    jne .fail_b64_decoded_max

    mov rdi, -1
    call base64_decoded_max_length
    test rax, rax
    jnz .fail_b64_decoded_max
    mov rcx, 0xC000000000000000
    cmp rdx, rcx
    jne .fail_b64_decoded_max


    ; ========================================================
    ; base64_encode -- RFC 4648 vectors
    ; ========================================================

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call base64_encode
    test rax, rax
    jnz .fail_b64_encode
    test rdx, rdx
    jnz .fail_b64_encode

    lea rdi, [rel b64_f]
    mov esi, b64_f_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_f_enc_length
    call base64_encode
    test rax, rax
    jnz .fail_b64_encode
    cmp rdx, b64_f_enc_length
    jne .fail_b64_encode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_f_enc]
    mov edx, b64_f_enc_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_encode

    lea rdi, [rel b64_fo]
    mov esi, b64_fo_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_fo_enc_length
    call base64_encode
    test rax, rax
    jnz .fail_b64_encode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_fo_enc]
    mov edx, b64_fo_enc_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_encode

    lea rdi, [rel b64_foo]
    mov esi, b64_foo_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_foo_enc_length
    call base64_encode
    test rax, rax
    jnz .fail_b64_encode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_foo_enc]
    mov edx, b64_foo_enc_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_encode

    lea rdi, [rel b64_foobar]
    mov esi, b64_foobar_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_foobar_enc_length
    call base64_encode
    test rax, rax
    jnz .fail_b64_encode
    cmp rdx, b64_foobar_enc_length
    jne .fail_b64_encode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_foobar_enc]
    mov edx, b64_foobar_enc_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_encode

    ; Full group followed by one-byte padded tail.
    lea rdi, [rel b64_foob]
    mov esi, b64_foob_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_foob_enc_length
    call base64_encode
    test rax, rax
    jnz .fail_b64_encode
    cmp rdx, b64_foob_enc_length
    jne .fail_b64_encode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_foob_enc]
    mov edx, b64_foob_enc_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_encode

    ; Full group followed by two-byte padded tail.
    lea rdi, [rel b64_fooba]
    mov esi, b64_fooba_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_fooba_enc_length
    call base64_encode
    test rax, rax
    jnz .fail_b64_encode
    cmp rdx, b64_fooba_enc_length
    jne .fail_b64_encode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_fooba_enc]
    mov edx, b64_fooba_enc_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_encode

    ; '+' and '/' alphabet symbols.
    lea rdi, [rel b64_symbols_binary]
    mov esi, b64_symbols_binary_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_symbols_encoded_length
    call base64_encode
    test rax, rax
    jnz .fail_b64_encode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_symbols_encoded]
    mov edx, b64_symbols_encoded_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_encode

    ; Excess capacity: no NUL/padding beyond encoded span.
    mov byte [rel base64_buffer + b64_foobar_enc_length], 0xA5
    mov byte [rel base64_buffer + b64_foobar_enc_length + 1], 0x5A
    lea rdi, [rel b64_foobar]
    mov esi, b64_foobar_length
    lea rdx, [rel base64_buffer]
    mov ecx, 128
    call base64_encode
    test rax, rax
    jnz .fail_b64_encode
    cmp rdx, b64_foobar_enc_length
    jne .fail_b64_encode
    cmp byte [rel base64_buffer + b64_foobar_enc_length], 0xA5
    jne .fail_b64_encode
    cmp byte [rel base64_buffer + b64_foobar_enc_length + 1], 0x5A
    jne .fail_b64_encode

    ; ENOSPC must not write.
    mov byte [rel base64_buffer], 0xA5
    lea rdi, [rel b64_foobar]
    mov esi, b64_foobar_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_foobar_enc_length - 1
    call base64_encode
    cmp rax, ERR_ENOSPC
    jne .fail_b64_encode
    test rdx, rdx
    jnz .fail_b64_encode
    cmp byte [rel base64_buffer], 0xA5
    jne .fail_b64_encode

    ; Overflow before source dereference.
    xor edi, edi
    mov rsi, -1
    xor edx, edx
    mov rcx, -1
    call base64_encode
    cmp rax, ERR_EOVERFLOW
    jne .fail_b64_encode
    test rdx, rdx
    jnz .fail_b64_encode


    ; ========================================================
    ; base64_decode
    ; ========================================================

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call base64_decode
    test rax, rax
    jnz .fail_b64_decode
    test rdx, rdx
    jnz .fail_b64_decode

    ; RFC 4648 vectors: one-, two-, three-byte tails and full groups.
    lea rdi, [rel b64_f_enc]
    mov esi, b64_f_enc_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_f_length
    call base64_decode
    test rax, rax
    jnz .fail_b64_decode
    cmp rdx, b64_f_length
    jne .fail_b64_decode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_f]
    mov edx, b64_f_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_decode

    lea rdi, [rel b64_fo_enc]
    mov esi, b64_fo_enc_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_fo_length
    call base64_decode
    test rax, rax
    jnz .fail_b64_decode
    cmp rdx, b64_fo_length
    jne .fail_b64_decode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_fo]
    mov edx, b64_fo_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_decode

    lea rdi, [rel b64_foo_enc]
    mov esi, b64_foo_enc_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_foo_length
    call base64_decode
    test rax, rax
    jnz .fail_b64_decode
    cmp rdx, b64_foo_length
    jne .fail_b64_decode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_foo]
    mov edx, b64_foo_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_decode

    lea rdi, [rel b64_foobar_enc]
    mov esi, b64_foobar_enc_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_foobar_length
    call base64_decode
    test rax, rax
    jnz .fail_b64_decode
    cmp rdx, b64_foobar_length
    jne .fail_b64_decode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_foobar]
    mov edx, b64_foobar_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_decode

    ; Decode full group followed by one-byte padded tail.
    lea rdi, [rel b64_foob_enc]
    mov esi, b64_foob_enc_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_foob_length
    call base64_decode
    test rax, rax
    jnz .fail_b64_decode
    cmp rdx, b64_foob_length
    jne .fail_b64_decode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_foob]
    mov edx, b64_foob_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_decode

    ; Decode full group followed by two-byte padded tail.
    lea rdi, [rel b64_fooba_enc]
    mov esi, b64_fooba_enc_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_fooba_length
    call base64_decode
    test rax, rax
    jnz .fail_b64_decode
    cmp rdx, b64_fooba_length
    jne .fail_b64_decode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_fooba]
    mov edx, b64_fooba_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_decode

    lea rdi, [rel b64_symbols_encoded]
    mov esi, b64_symbols_encoded_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_symbols_binary_length
    call base64_decode
    test rax, rax
    jnz .fail_b64_decode
    lea rdi, [rel base64_buffer]
    lea rsi, [rel b64_symbols_binary]
    mov edx, b64_symbols_binary_length
    call .bytes_match
    cmp eax, 1
    jne .fail_b64_decode

    ; Excess capacity must remain untouched after decoded output.
    mov byte [rel base64_buffer + b64_foobar_length], 0xA5
    mov byte [rel base64_buffer + b64_foobar_length + 1], 0x5A
    lea rdi, [rel b64_foobar_enc]
    mov esi, b64_foobar_enc_length
    lea rdx, [rel base64_buffer]
    mov ecx, 128
    call base64_decode
    test rax, rax
    jnz .fail_b64_decode
    cmp rdx, b64_foobar_length
    jne .fail_b64_decode
    cmp byte [rel base64_buffer + b64_foobar_length], 0xA5
    jne .fail_b64_decode
    cmp byte [rel base64_buffer + b64_foobar_length + 1], 0x5A
    jne .fail_b64_decode

    ; Invalid length.
    mov byte [rel base64_buffer], 0xA5
    lea rdi, [rel b64_bad_length]
    mov esi, b64_bad_length_length
    lea rdx, [rel base64_buffer]
    mov ecx, 128
    call base64_decode
    cmp rax, ERR_EINVAL
    jne .fail_b64_decode
    test rdx, rdx
    jnz .fail_b64_decode
    cmp byte [rel base64_buffer], 0xA5
    jne .fail_b64_decode

    ; Invalid alphabet.
    mov byte [rel base64_buffer], 0xA5
    lea rdi, [rel b64_bad_char]
    mov esi, b64_bad_char_length
    lea rdx, [rel base64_buffer]
    mov ecx, 128
    call base64_decode
    cmp rax, ERR_EINVAL
    jne .fail_b64_decode
    test rdx, rdx
    jnz .fail_b64_decode
    cmp byte [rel base64_buffer], 0xA5
    jne .fail_b64_decode

    ; Padding cannot begin a quartet.
    lea rdi, [rel b64_bad_padding_first]
    mov esi, b64_bad_padding_first_length
    lea rdx, [rel base64_buffer]
    mov ecx, 128
    call base64_decode
    cmp rax, ERR_EINVAL
    jne .fail_b64_decode
    test rdx, rdx
    jnz .fail_b64_decode

    ; Padding is forbidden in non-final quartets.
    lea rdi, [rel b64_bad_padding_nonfinal]
    mov esi, b64_bad_padding_nonfinal_length
    lea rdx, [rel base64_buffer]
    mov ecx, 128
    call base64_decode
    cmp rax, ERR_EINVAL
    jne .fail_b64_decode
    test rdx, rdx
    jnz .fail_b64_decode

    ; Invalid final padding shape.
    lea rdi, [rel b64_bad_padding_shape]
    mov esi, b64_bad_padding_shape_length
    lea rdx, [rel base64_buffer]
    mov ecx, 128
    call base64_decode
    cmp rax, ERR_EINVAL
    jne .fail_b64_decode
    test rdx, rdx
    jnz .fail_b64_decode

    ; Canonical tail-bit enforcement.
    lea rdi, [rel b64_bad_tail_double]
    mov esi, b64_bad_tail_double_length
    lea rdx, [rel base64_buffer]
    mov ecx, 128
    call base64_decode
    cmp rax, ERR_EINVAL
    jne .fail_b64_decode
    test rdx, rdx
    jnz .fail_b64_decode

    lea rdi, [rel b64_bad_tail_single]
    mov esi, b64_bad_tail_single_length
    lea rdx, [rel base64_buffer]
    mov ecx, 128
    call base64_decode
    cmp rax, ERR_EINVAL
    jne .fail_b64_decode
    test rdx, rdx
    jnz .fail_b64_decode

    ; ENOSPC after full syntax validation and before writes.
    mov byte [rel base64_buffer], 0xA5
    lea rdi, [rel b64_foobar_enc]
    mov esi, b64_foobar_enc_length
    lea rdx, [rel base64_buffer]
    mov ecx, b64_foobar_length - 1
    call base64_decode
    cmp rax, ERR_ENOSPC
    jne .fail_b64_decode
    test rdx, rdx
    jnz .fail_b64_decode
    cmp byte [rel base64_buffer], 0xA5
    jne .fail_b64_decode


.success:
    xor edi, edi
    jmp .exit

.fail_hex_encode:
    mov edi, 1
    jmp .exit

.fail_hex_decode:
    mov edi, 2
    jmp .exit

.fail_percent_length:
    mov edi, 3
    jmp .exit

.fail_percent_encode:
    mov edi, 4
    jmp .exit

.fail_percent_decode:
    mov edi, 5
    jmp .exit

.fail_b64_encoded_length:
    mov edi, 6
    jmp .exit

.fail_b64_decoded_max:
    mov edi, 7
    jmp .exit

.fail_b64_encode:
    mov edi, 8
    jmp .exit

.fail_b64_decode:
    mov edi, 9
    jmp .exit


; Compare exactly RDX bytes at RDI and RSI.
; Returns EAX=1 equal, EAX=0 different.
.bytes_match:
    test rdx, rdx
    jz .bytes_match_equal
    xor ecx, ecx

.bytes_match_loop:
    mov al, [rdi + rcx]
    cmp al, [rsi + rcx]
    jne .bytes_match_different
    inc rcx
    cmp rcx, rdx
    jb .bytes_match_loop

.bytes_match_equal:
    mov eax, 1
    ret

.bytes_match_different:
    xor eax, eax
    ret


.exit:
    mov eax, SYS_EXIT
    syscall


section .note.GNU-stack noalloc noexec nowrite progbits
