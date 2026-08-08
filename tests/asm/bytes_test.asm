; Arborcore byte-span engine tests
;
; Exit status:
;
;   0 = all tests passed
;   1 = bytes_equal failed
;   2 = bytes_compare failed
;   3 = bytes_starts_with failed
;   4 = bytes_ends_with failed
;   5 = bytes_find failed
;   6 = bytes_equal_ascii_ci failed
;   7 = cstr_length failed


%define SYS_EXIT 60


global _start


extern bytes_equal
extern bytes_compare
extern bytes_starts_with
extern bytes_ends_with
extern bytes_find
extern bytes_equal_ascii_ci
extern cstr_length


section .rodata


; ============================================================
; Equality data
; ============================================================

equal_a:
    db "Arborcore"

equal_a_length equ $ - equal_a


equal_b:
    db "Arborcore"

equal_b_length equ $ - equal_b


different_first:
    db "Xrborcore"

different_first_length equ $ - different_first


different_middle:
    db "ArborXore"

different_middle_length equ $ - different_middle


different_last:
    db "ArborcorX"

different_last_length equ $ - different_last


shorter:
    db "Arborcor"

shorter_length equ $ - shorter


embedded_a:
    db 0x41, 0x00, 0x42, 0x80, 0xFF

embedded_a_length equ $ - embedded_a


embedded_b:
    db 0x41, 0x00, 0x42, 0x80, 0xFF

embedded_b_length equ $ - embedded_b


; ============================================================
; Compare data
; ============================================================

compare_less_a:
    db 0x10, 0x20, 0x30

compare_less_b:
    db 0x10, 0x21, 0x00


compare_unsigned_a:
    db 0x80

compare_unsigned_b:
    db 0x7F


compare_prefix:
    db "abc"

compare_longer:
    db "abcd"


; ============================================================
; Prefix data
; ============================================================

request_line:
    db "GET /users/42 HTTP/1.1"

request_line_length equ $ - request_line


get_prefix:
    db "GET "

get_prefix_length equ $ - get_prefix


wrong_prefix:
    db "POST"

wrong_prefix_length equ $ - wrong_prefix


longer_prefix:
    db "GET /users/42 HTTP/1.1X"

longer_prefix_length equ $ - longer_prefix


; ============================================================
; Suffix data
; ============================================================

path_value:
    db "/index.html"

path_value_length equ $ - path_value


html_suffix:
    db ".html"

html_suffix_length equ $ - html_suffix


wrong_suffix:
    db ".json"

wrong_suffix_length equ $ - wrong_suffix


; ============================================================
; Find data
; ============================================================

find_haystack:
    db "abc::def::ghi"

find_haystack_length equ $ - find_haystack


find_begin:
    db "abc"

find_begin_length equ $ - find_begin


find_middle:
    db "::def"

find_middle_length equ $ - find_middle


find_end:
    db "ghi"

find_end_length equ $ - find_end


find_missing:
    db "xyz"

find_missing_length equ $ - find_missing


find_too_long:
    db "abc::def::ghiX"

find_too_long_length equ $ - find_too_long


find_zero:
    db 0x00

find_zero_length equ $ - find_zero


; ============================================================
; ASCII case-insensitive data
; ============================================================

ci_header_a:
    db "Content-Type"

ci_header_a_length equ $ - ci_header_a


ci_header_b:
    db "content-type"

ci_header_b_length equ $ - ci_header_b


ci_mixed_a:
    db "ABC-123"

ci_mixed_a_length equ $ - ci_mixed_a


ci_mixed_b:
    db "abc-123"

ci_mixed_b_length equ $ - ci_mixed_b


ci_wrong:
    db "Most"

ci_wrong_length equ $ - ci_wrong


ci_host:
    db "Host"

ci_host_length equ $ - ci_host


ci_non_ascii_a:
    db 0xC0, 'A'

ci_non_ascii_a_length equ $ - ci_non_ascii_a


ci_non_ascii_b:
    db 0xE0, 'a'

ci_non_ascii_b_length equ $ - ci_non_ascii_b


ci_short:
    db "content"

ci_short_length equ $ - ci_short


; ============================================================
; C strings
; ============================================================

cstr_empty:
    db 0


cstr_one:
    db "A", 0


cstr_arborcore:
    db "Arborcore", 0


section .text


_start:

    ; ========================================================
    ; bytes_equal
    ; ========================================================

    ; Empty spans are equal and must not dereference NULL.

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx

    call bytes_equal

    cmp rax, 1
    jne .fail_equal


    ; Identical ordinary spans.

    lea rdi, [rel equal_a]
    mov esi, equal_a_length

    lea rdx, [rel equal_b]
    mov ecx, equal_b_length

    call bytes_equal

    cmp rax, 1
    jne .fail_equal


    ; Different first byte.

    lea rdi, [rel equal_a]
    mov esi, equal_a_length

    lea rdx, [rel different_first]
    mov ecx, different_first_length

    call bytes_equal

    test rax, rax
    jnz .fail_equal


    ; Different middle byte.

    lea rdi, [rel equal_a]
    mov esi, equal_a_length

    lea rdx, [rel different_middle]
    mov ecx, different_middle_length

    call bytes_equal

    test rax, rax
    jnz .fail_equal


    ; Different final byte.

    lea rdi, [rel equal_a]
    mov esi, equal_a_length

    lea rdx, [rel different_last]
    mov ecx, different_last_length

    call bytes_equal

    test rax, rax
    jnz .fail_equal


    ; Different lengths.

    lea rdi, [rel equal_a]
    mov esi, equal_a_length

    lea rdx, [rel shorter]
    mov ecx, shorter_length

    call bytes_equal

    test rax, rax
    jnz .fail_equal


    ; Embedded NUL and high-byte values are ordinary bytes.

    lea rdi, [rel embedded_a]
    mov esi, embedded_a_length

    lea rdx, [rel embedded_b]
    mov ecx, embedded_b_length

    call bytes_equal

    cmp rax, 1
    jne .fail_equal


    ; ========================================================
    ; bytes_compare
    ; ========================================================

    ; Empty spans compare equal without dereferencing NULL.

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx

    call bytes_compare

    test rax, rax
    jnz .fail_compare


    ; Equal.

    lea rdi, [rel equal_a]
    mov esi, equal_a_length

    lea rdx, [rel equal_b]
    mov ecx, equal_b_length

    call bytes_compare

    test rax, rax
    jnz .fail_compare


    ; Lexicographically less.

    lea rdi, [rel compare_less_a]
    mov esi, 3

    lea rdx, [rel compare_less_b]
    mov ecx, 3

    call bytes_compare

    cmp rax, -1
    jne .fail_compare


    ; Lexicographically greater.

    lea rdi, [rel compare_less_b]
    mov esi, 3

    lea rdx, [rel compare_less_a]
    mov ecx, 3

    call bytes_compare

    cmp rax, 1
    jne .fail_compare


    ; Unsigned byte comparison: 0x80 > 0x7F.

    lea rdi, [rel compare_unsigned_a]
    mov esi, 1

    lea rdx, [rel compare_unsigned_b]
    mov ecx, 1

    call bytes_compare

    cmp rax, 1
    jne .fail_compare


    ; Equal prefix, shorter span sorts first.

    lea rdi, [rel compare_prefix]
    mov esi, 3

    lea rdx, [rel compare_longer]
    mov ecx, 4

    call bytes_compare

    cmp rax, -1
    jne .fail_compare


    ; Reverse length comparison.

    lea rdi, [rel compare_longer]
    mov esi, 4

    lea rdx, [rel compare_prefix]
    mov ecx, 3

    call bytes_compare

    cmp rax, 1
    jne .fail_compare


    ; ========================================================
    ; bytes_starts_with
    ; ========================================================

    ; Empty prefix.

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx

    call bytes_starts_with

    cmp rax, 1
    jne .fail_starts


    ; Normal prefix.

    lea rdi, [rel request_line]
    mov esi, request_line_length

    lea rdx, [rel get_prefix]
    mov ecx, get_prefix_length

    call bytes_starts_with

    cmp rax, 1
    jne .fail_starts


    ; Wrong prefix.

    lea rdi, [rel request_line]
    mov esi, request_line_length

    lea rdx, [rel wrong_prefix]
    mov ecx, wrong_prefix_length

    call bytes_starts_with

    test rax, rax
    jnz .fail_starts


    ; Whole span is a prefix of itself.

    lea rdi, [rel request_line]
    mov esi, request_line_length

    lea rdx, [rel request_line]
    mov ecx, request_line_length

    call bytes_starts_with

    cmp rax, 1
    jne .fail_starts


    ; Prefix longer than span.

    lea rdi, [rel request_line]
    mov esi, request_line_length

    lea rdx, [rel longer_prefix]
    mov ecx, longer_prefix_length

    call bytes_starts_with

    test rax, rax
    jnz .fail_starts


    ; ========================================================
    ; bytes_ends_with
    ; ========================================================

    ; Empty suffix.

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx

    call bytes_ends_with

    cmp rax, 1
    jne .fail_ends


    ; Normal suffix.

    lea rdi, [rel path_value]
    mov esi, path_value_length

    lea rdx, [rel html_suffix]
    mov ecx, html_suffix_length

    call bytes_ends_with

    cmp rax, 1
    jne .fail_ends


    ; Wrong suffix.

    lea rdi, [rel path_value]
    mov esi, path_value_length

    lea rdx, [rel wrong_suffix]
    mov ecx, wrong_suffix_length

    call bytes_ends_with

    test rax, rax
    jnz .fail_ends


    ; Whole span is a suffix of itself.

    lea rdi, [rel path_value]
    mov esi, path_value_length

    lea rdx, [rel path_value]
    mov ecx, path_value_length

    call bytes_ends_with

    cmp rax, 1
    jne .fail_ends


    ; ========================================================
    ; bytes_find
    ; ========================================================

    ; Empty needle returns beginning of haystack.

    lea rdi, [rel find_haystack]
    mov esi, find_haystack_length

    lea rdx, [rel find_haystack]
    xor ecx, ecx

    call bytes_find

    lea r8, [rel find_haystack]

    cmp rax, r8
    jne .fail_find


    ; Match at beginning.

    lea rdi, [rel find_haystack]
    mov esi, find_haystack_length

    lea rdx, [rel find_begin]
    mov ecx, find_begin_length

    call bytes_find

    lea r8, [rel find_haystack]

    cmp rax, r8
    jne .fail_find


    ; Match in middle.

    lea rdi, [rel find_haystack]
    mov esi, find_haystack_length

    lea rdx, [rel find_middle]
    mov ecx, find_middle_length

    call bytes_find

    lea r8, [rel find_haystack + 3]

    cmp rax, r8
    jne .fail_find


    ; Match at end.

    lea rdi, [rel find_haystack]
    mov esi, find_haystack_length

    lea rdx, [rel find_end]
    mov ecx, find_end_length

    call bytes_find

    lea r8, [rel find_haystack + 10]

    cmp rax, r8
    jne .fail_find


    ; Missing needle.

    lea rdi, [rel find_haystack]
    mov esi, find_haystack_length

    lea rdx, [rel find_missing]
    mov ecx, find_missing_length

    call bytes_find

    test rax, rax
    jnz .fail_find


    ; Needle longer than haystack.

    lea rdi, [rel find_haystack]
    mov esi, find_haystack_length

    lea rdx, [rel find_too_long]
    mov ecx, find_too_long_length

    call bytes_find

    test rax, rax
    jnz .fail_find


    ; Arbitrary byte spans may contain and search for 0x00.

    lea rdi, [rel embedded_a]
    mov esi, embedded_a_length

    lea rdx, [rel find_zero]
    mov ecx, find_zero_length

    call bytes_find

    lea r8, [rel embedded_a + 1]

    cmp rax, r8
    jne .fail_find


    ; ========================================================
    ; bytes_equal_ascii_ci
    ; ========================================================

    ; Empty spans.

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx

    call bytes_equal_ascii_ci

    cmp rax, 1
    jne .fail_ascii_ci


    ; HTTP-style header name.

    lea rdi, [rel ci_header_a]
    mov esi, ci_header_a_length

    lea rdx, [rel ci_header_b]
    mov ecx, ci_header_b_length

    call bytes_equal_ascii_ci

    cmp rax, 1
    jne .fail_ascii_ci


    ; Letters fold; punctuation and digits remain literal.

    lea rdi, [rel ci_mixed_a]
    mov esi, ci_mixed_a_length

    lea rdx, [rel ci_mixed_b]
    mov ecx, ci_mixed_b_length

    call bytes_equal_ascii_ci

    cmp rax, 1
    jne .fail_ascii_ci


    ; Actual ASCII difference.

    lea rdi, [rel ci_host]
    mov esi, ci_host_length

    lea rdx, [rel ci_wrong]
    mov ecx, ci_wrong_length

    call bytes_equal_ascii_ci

    test rax, rax
    jnz .fail_ascii_ci


    ; Non-ASCII bytes are compared literally.

    lea rdi, [rel ci_non_ascii_a]
    mov esi, ci_non_ascii_a_length

    lea rdx, [rel ci_non_ascii_b]
    mov ecx, ci_non_ascii_b_length

    call bytes_equal_ascii_ci

    test rax, rax
    jnz .fail_ascii_ci


    ; Different lengths.

    lea rdi, [rel ci_header_a]
    mov esi, ci_header_a_length

    lea rdx, [rel ci_short]
    mov ecx, ci_short_length

    call bytes_equal_ascii_ci

    test rax, rax
    jnz .fail_ascii_ci


    ; ========================================================
    ; cstr_length
    ; ========================================================

    lea rdi, [rel cstr_empty]

    call cstr_length

    test rax, rax
    jnz .fail_cstr


    lea rdi, [rel cstr_one]

    call cstr_length

    cmp rax, 1
    jne .fail_cstr


    lea rdi, [rel cstr_arborcore]

    call cstr_length

    cmp rax, 9
    jne .fail_cstr


    ; ========================================================
    ; Everything passed.
    ; ========================================================

.success:
    xor edi, edi
    jmp .exit


.fail_equal:
    mov edi, 1
    jmp .exit


.fail_compare:
    mov edi, 2
    jmp .exit


.fail_starts:
    mov edi, 3
    jmp .exit


.fail_ends:
    mov edi, 4
    jmp .exit


.fail_find:
    mov edi, 5
    jmp .exit


.fail_ascii_ci:
    mov edi, 6
    jmp .exit


.fail_cstr:
    mov edi, 7


.exit:
    mov eax, SYS_EXIT
    syscall


section .note.GNU-stack noalloc noexec nowrite progbits
