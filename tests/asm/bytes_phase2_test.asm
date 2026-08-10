; Arborcore byte-engine Phase 2 tests
;
; Exit status:
;   0 = all tests passed
;   1 = ASCII primitives failed
;   2 = bytes_find_crlf failed
;   3 = bytes_skip_byte failed
;   4 = bytes_skip_ascii_space failed
;   5 = bytes_trim_ascii_space failed
;   6 = decimal parser failed
;   7 = hexadecimal parser failed


%define SYS_EXIT      60
%define ERR_EINVAL   -22
%define ERR_EOVERFLOW -75


global _start


extern ascii_is_digit
extern ascii_is_alpha
extern ascii_is_hex_digit
extern ascii_is_space
extern ascii_to_lower
extern ascii_to_upper

extern bytes_find_crlf
extern bytes_skip_byte
extern bytes_skip_ascii_space
extern bytes_trim_ascii_space

extern bytes_parse_u64_decimal
extern bytes_parse_u64_hex


section .rodata


crlf_source:
    db "GET / HTTP/1.1", 13, 10, "Host: x", 13, 10
crlf_source_length equ $ - crlf_source

no_crlf_source:
    db "GET / HTTP/1.1", 10
no_crlf_source_length equ $ - no_crlf_source

skip_colons:
    db ":::value"
skip_colons_length equ $ - skip_colons

skip_none:
    db "value"
skip_none_length equ $ - skip_none

space_source:
    db 0x20, 0x09, 0x0D, 0x0A, "value"
space_source_length equ $ - space_source

trim_source:
    db 0x20, 0x09, "value", 0x0D, 0x0A, 0x20
trim_source_length equ $ - trim_source

trim_all_space:
    db 0x20, 0x09, 0x0A
trim_all_space_length equ $ - trim_all_space

trim_none:
    db "value"
trim_none_length equ $ - trim_none


dec_zero:
    db "0"

dec_value:
    db "12345"

dec_max:
    db "18446744073709551615"

dec_overflow:
    db "18446744073709551616"

dec_invalid:
    db "12x45"


hex_zero:
    db "0"

hex_value:
    db "1a2B"

hex_max:
    db "FFFFFFFFFFFFFFFF"

hex_max_lower:
    db "ffffffffffffffff"

hex_overflow:
    db "10000000000000000"

hex_invalid:
    db "12G4"

hex_prefix_invalid:
    db "0x10"


section .text


_start:

    ; ========================================================
    ; ASCII primitives
    ; ========================================================

    mov edi, '0'
    call ascii_is_digit
    cmp eax, 1
    jne .fail_ascii

    mov edi, '9'
    call ascii_is_digit
    cmp eax, 1
    jne .fail_ascii

    mov edi, '/'
    call ascii_is_digit
    test eax, eax
    jnz .fail_ascii

    mov edi, ':'
    call ascii_is_digit
    test eax, eax
    jnz .fail_ascii

    mov edi, 'A'
    call ascii_is_alpha
    cmp eax, 1
    jne .fail_ascii

    mov edi, 'z'
    call ascii_is_alpha
    cmp eax, 1
    jne .fail_ascii

    mov edi, '0'
    call ascii_is_alpha
    test eax, eax
    jnz .fail_ascii

    mov edi, 'F'
    call ascii_is_hex_digit
    cmp eax, 1
    jne .fail_ascii

    mov edi, 'f'
    call ascii_is_hex_digit
    cmp eax, 1
    jne .fail_ascii

    mov edi, '9'
    call ascii_is_hex_digit
    cmp eax, 1
    jne .fail_ascii

    mov edi, 'G'
    call ascii_is_hex_digit
    test eax, eax
    jnz .fail_ascii

    mov edi, 0x20
    call ascii_is_space
    cmp eax, 1
    jne .fail_ascii

    mov edi, 0x09
    call ascii_is_space
    cmp eax, 1
    jne .fail_ascii

    mov edi, 0x0D
    call ascii_is_space
    cmp eax, 1
    jne .fail_ascii

    mov edi, 'A'
    call ascii_is_space
    test eax, eax
    jnz .fail_ascii

    mov edi, 'A'
    call ascii_to_lower
    cmp eax, 'a'
    jne .fail_ascii

    mov edi, 'z'
    call ascii_to_lower
    cmp eax, 'z'
    jne .fail_ascii

    mov edi, 0xFF
    call ascii_to_lower
    cmp eax, 0xFF
    jne .fail_ascii

    mov edi, 'a'
    call ascii_to_upper
    cmp eax, 'A'
    jne .fail_ascii

    mov edi, 'Z'
    call ascii_to_upper
    cmp eax, 'Z'
    jne .fail_ascii

    ; ========================================================
    ; CRLF search
    ; ========================================================

    lea rdi, [rel crlf_source]
    mov esi, crlf_source_length
    call bytes_find_crlf

    lea rcx, [rel crlf_source + 14]
    cmp rax, rcx
    jne .fail_crlf

    lea rdi, [rel no_crlf_source]
    mov esi, no_crlf_source_length
    call bytes_find_crlf

    test rax, rax
    jnz .fail_crlf

    xor edi, edi
    xor esi, esi
    call bytes_find_crlf

    test rax, rax
    jnz .fail_crlf

    ; ========================================================
    ; skip byte
    ; ========================================================

    lea rdi, [rel skip_colons]
    mov esi, skip_colons_length
    mov edx, ':'
    call bytes_skip_byte

    cmp rax, 3
    jne .fail_skip_byte

    lea rdi, [rel skip_none]
    mov esi, skip_none_length
    mov edx, ':'
    call bytes_skip_byte

    test rax, rax
    jnz .fail_skip_byte

    xor edi, edi
    xor esi, esi
    mov edx, ':'
    call bytes_skip_byte

    test rax, rax
    jnz .fail_skip_byte

    ; ========================================================
    ; skip ASCII space
    ; ========================================================

    lea rdi, [rel space_source]
    mov esi, space_source_length
    call bytes_skip_ascii_space

    cmp rax, 4
    jne .fail_skip_space

    lea rdi, [rel skip_none]
    mov esi, skip_none_length
    call bytes_skip_ascii_space

    test rax, rax
    jnz .fail_skip_space

    ; ========================================================
    ; trim ASCII space
    ; ========================================================

    lea rdi, [rel trim_source]
    mov esi, trim_source_length
    call bytes_trim_ascii_space

    lea rcx, [rel trim_source + 2]
    cmp rax, rcx
    jne .fail_trim

    cmp rdx, 5
    jne .fail_trim

    lea rdi, [rel trim_all_space]
    mov esi, trim_all_space_length
    call bytes_trim_ascii_space

    lea rcx, [rel trim_all_space + trim_all_space_length]
    cmp rax, rcx
    jne .fail_trim

    test rdx, rdx
    jnz .fail_trim

    lea rdi, [rel trim_none]
    mov esi, trim_none_length
    call bytes_trim_ascii_space

    lea rcx, [rel trim_none]
    cmp rax, rcx
    jne .fail_trim

    cmp rdx, trim_none_length
    jne .fail_trim

    ; ========================================================
    ; decimal parser
    ; ========================================================

    lea rdi, [rel dec_zero]
    mov esi, 1
    call bytes_parse_u64_decimal
    test rax, rax
    jnz .fail_decimal
    test rdx, rdx
    jnz .fail_decimal

    lea rdi, [rel dec_value]
    mov esi, 5
    call bytes_parse_u64_decimal
    test rax, rax
    jnz .fail_decimal
    cmp rdx, 12345
    jne .fail_decimal

    lea rdi, [rel dec_max]
    mov esi, 20
    call bytes_parse_u64_decimal
    test rax, rax
    jnz .fail_decimal
    cmp rdx, -1
    jne .fail_decimal

    lea rdi, [rel dec_overflow]
    mov esi, 20
    call bytes_parse_u64_decimal
    cmp rax, ERR_EOVERFLOW
    jne .fail_decimal
    test rdx, rdx
    jnz .fail_decimal

    lea rdi, [rel dec_invalid]
    mov esi, 5
    call bytes_parse_u64_decimal
    cmp rax, ERR_EINVAL
    jne .fail_decimal

    xor edi, edi
    xor esi, esi
    call bytes_parse_u64_decimal
    cmp rax, ERR_EINVAL
    jne .fail_decimal

    ; ========================================================
    ; hexadecimal parser
    ; ========================================================

    lea rdi, [rel hex_zero]
    mov esi, 1
    call bytes_parse_u64_hex
    test rax, rax
    jnz .fail_hex
    test rdx, rdx
    jnz .fail_hex

    lea rdi, [rel hex_value]
    mov esi, 4
    call bytes_parse_u64_hex
    test rax, rax
    jnz .fail_hex
    cmp rdx, 0x1A2B
    jne .fail_hex

    lea rdi, [rel hex_max]
    mov esi, 16
    call bytes_parse_u64_hex
    test rax, rax
    jnz .fail_hex
    cmp rdx, -1
    jne .fail_hex

    lea rdi, [rel hex_max_lower]
    mov esi, 16
    call bytes_parse_u64_hex
    test rax, rax
    jnz .fail_hex
    cmp rdx, -1
    jne .fail_hex

    lea rdi, [rel hex_overflow]
    mov esi, 17
    call bytes_parse_u64_hex
    cmp rax, ERR_EOVERFLOW
    jne .fail_hex

    lea rdi, [rel hex_invalid]
    mov esi, 4
    call bytes_parse_u64_hex
    cmp rax, ERR_EINVAL
    jne .fail_hex

    lea rdi, [rel hex_prefix_invalid]
    mov esi, 4
    call bytes_parse_u64_hex
    cmp rax, ERR_EINVAL
    jne .fail_hex

    xor edi, edi
    xor esi, esi
    call bytes_parse_u64_hex
    cmp rax, ERR_EINVAL
    jne .fail_hex

    ; ========================================================
    ; Success / failure exits
    ; ========================================================

.success:
    xor edi, edi
    jmp .exit

.fail_ascii:
    mov edi, 1
    jmp .exit

.fail_crlf:
    mov edi, 2
    jmp .exit

.fail_skip_byte:
    mov edi, 3
    jmp .exit

.fail_skip_space:
    mov edi, 4
    jmp .exit

.fail_trim:
    mov edi, 5
    jmp .exit

.fail_decimal:
    mov edi, 6
    jmp .exit

.fail_hex:
    mov edi, 7

.exit:
    mov eax, SYS_EXIT
    syscall


section .note.GNU-stack noalloc noexec nowrite progbits
