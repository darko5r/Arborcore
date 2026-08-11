; Arborcore Core Retrofit B1 sequence-property qualification
;
; Production code is not modified by this suite.  It checks finite-domain
; and algebraic properties against independent reference logic in this test.
;
; Exit status:
;   0 = all properties passed
;   1 = ASCII exhaustive domain failed
;   2 = single-byte relation algebra failed
;   3 = prefix/suffix/find properties failed
;   4 = bounded scan/trim properties failed

%define SYS_EXIT 60

global _start

extern ascii_is_digit
extern ascii_is_alpha
extern ascii_is_hex_digit
extern ascii_is_space
extern ascii_to_lower
extern ascii_to_upper

extern bytes_equal
extern bytes_compare
extern bytes_starts_with
extern bytes_ends_with
extern bytes_find
extern bytes_equal_ascii_ci

extern bytes_find_crlf
extern bytes_skip_byte
extern bytes_skip_ascii_space
extern bytes_trim_ascii_space

section .bss
align 16
left_byte:  resb 1
right_byte: resb 1
haystack:   resb 4
needle:     resb 2
scan_buf:   resb 4

section .text

_start:
    cld

    call .test_ascii
    test eax, eax
    jnz .fail_ascii

    call .test_byte_relations
    test eax, eax
    jnz .fail_relations

    call .test_find_prefix_suffix
    test eax, eax
    jnz .fail_find

    call .test_scans
    test eax, eax
    jnz .fail_scans

    xor edi, edi
    jmp .exit

.fail_ascii:
    mov edi, 1
    jmp .exit
.fail_relations:
    mov edi, 2
    jmp .exit
.fail_find:
    mov edi, 3
    jmp .exit
.fail_scans:
    mov edi, 4

.exit:
    mov eax, SYS_EXIT
    syscall

; ------------------------------------------------------------
; Exhaust all 256 byte values for ASCII predicates/conversions.
; ------------------------------------------------------------
.test_ascii:
    xor r12d, r12d

.ascii_loop:
    ; digit reference
    xor r13d, r13d
    cmp r12d, '0'
    jb .digit_ref_ready
    cmp r12d, '9'
    ja .digit_ref_ready
    mov r13d, 1
.digit_ref_ready:
    mov edi, r12d
    call ascii_is_digit
    cmp eax, r13d
    jne .ascii_fail

    ; alpha reference
    xor r13d, r13d
    cmp r12d, 'A'
    jb .alpha_lower_ref
    cmp r12d, 'Z'
    jbe .alpha_ref_yes
.alpha_lower_ref:
    cmp r12d, 'a'
    jb .alpha_ref_ready
    cmp r12d, 'z'
    ja .alpha_ref_ready
.alpha_ref_yes:
    mov r13d, 1
.alpha_ref_ready:
    mov edi, r12d
    call ascii_is_alpha
    cmp eax, r13d
    jne .ascii_fail

    ; hexadecimal reference
    xor r13d, r13d
    cmp r12d, '0'
    jb .hex_upper_ref
    cmp r12d, '9'
    jbe .hex_ref_yes
.hex_upper_ref:
    cmp r12d, 'A'
    jb .hex_lower_ref
    cmp r12d, 'F'
    jbe .hex_ref_yes
.hex_lower_ref:
    cmp r12d, 'a'
    jb .hex_ref_ready
    cmp r12d, 'f'
    ja .hex_ref_ready
.hex_ref_yes:
    mov r13d, 1
.hex_ref_ready:
    mov edi, r12d
    call ascii_is_hex_digit
    cmp eax, r13d
    jne .ascii_fail

    ; ASCII whitespace reference
    xor r13d, r13d
    cmp r12d, 0x20
    je .space_ref_yes
    cmp r12d, 0x09
    jb .space_ref_ready
    cmp r12d, 0x0D
    ja .space_ref_ready
.space_ref_yes:
    mov r13d, 1
.space_ref_ready:
    mov edi, r12d
    call ascii_is_space
    cmp eax, r13d
    jne .ascii_fail

    ; lowercase reference and idempotence
    mov r13d, r12d
    cmp r12d, 'A'
    jb .lower_ref_ready
    cmp r12d, 'Z'
    ja .lower_ref_ready
    add r13d, 'a' - 'A'
.lower_ref_ready:
    mov edi, r12d
    call ascii_to_lower
    cmp eax, r13d
    jne .ascii_fail
    mov edi, eax
    call ascii_to_lower
    cmp eax, r13d
    jne .ascii_fail

    ; uppercase reference and idempotence
    mov r13d, r12d
    cmp r12d, 'a'
    jb .upper_ref_ready
    cmp r12d, 'z'
    ja .upper_ref_ready
    sub r13d, 'a' - 'A'
.upper_ref_ready:
    mov edi, r12d
    call ascii_to_upper
    cmp eax, r13d
    jne .ascii_fail
    mov edi, eax
    call ascii_to_upper
    cmp eax, r13d
    jne .ascii_fail

    inc r12d
    cmp r12d, 256
    jb .ascii_loop

    xor eax, eax
    ret

.ascii_fail:
    mov eax, 1
    ret

; ------------------------------------------------------------
; Exhaust all 65,536 ordered pairs of one-byte spans.
; ------------------------------------------------------------
.test_byte_relations:
    xor r12d, r12d

.left_loop:
    mov [rel left_byte], r12b
    xor r13d, r13d

.right_loop:
    mov [rel right_byte], r13b

    ; Exact equality reference.
    lea rdi, [rel left_byte]
    mov esi, 1
    lea rdx, [rel right_byte]
    mov ecx, 1
    call bytes_equal

    xor r14d, r14d
    cmp r12d, r13d
    sete r14b
    cmp eax, r14d
    jne .relations_fail

    ; Unsigned lexicographical ordering for one byte.
    lea rdi, [rel left_byte]
    mov esi, 1
    lea rdx, [rel right_byte]
    mov ecx, 1
    call bytes_compare

    xor r14d, r14d
    cmp r12d, r13d
    jb .compare_ref_less
    ja .compare_ref_greater
    jmp .compare_ref_ready
.compare_ref_less:
    mov r14, -1
    jmp .compare_ref_ready
.compare_ref_greater:
    mov r14d, 1
.compare_ref_ready:
    cmp rax, r14
    jne .relations_fail

    ; ASCII case-insensitive equality reference.
    mov r14d, r12d
    cmp r14d, 'A'
    jb .left_folded
    cmp r14d, 'Z'
    ja .left_folded
    add r14d, 'a' - 'A'
.left_folded:
    mov r15d, r13d
    cmp r15d, 'A'
    jb .right_folded
    cmp r15d, 'Z'
    ja .right_folded
    add r15d, 'a' - 'A'
.right_folded:
    lea rdi, [rel left_byte]
    mov esi, 1
    lea rdx, [rel right_byte]
    mov ecx, 1
    call bytes_equal_ascii_ci
    xor r11d, r11d
    cmp r14d, r15d
    sete r11b
    cmp eax, r11d
    jne .relations_fail

    inc r13d
    cmp r13d, 256
    jb .right_loop

    inc r12d
    cmp r12d, 256
    jb .left_loop

    ; Empty-span identities must be NULL-safe.
    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call bytes_equal
    cmp eax, 1
    jne .relations_fail

    xor edi, edi
    xor esi, esi
    xor edx, edx
    xor ecx, ecx
    call bytes_compare
    test rax, rax
    jne .relations_fail

    xor eax, eax
    ret

.relations_fail:
    mov eax, 1
    ret

; ------------------------------------------------------------
; Exhaust 4-byte binary haystacks and 0..2 byte binary needles.
; Reference implementations below are independent bounded scans.
; ------------------------------------------------------------
.test_find_prefix_suffix:
    xor r12d, r12d                  ; haystack mask 0..15

.hay_case:
    mov edi, r12d
    lea rsi, [rel haystack]
    mov edx, 4
    call .make_binary_case

    xor r13d, r13d                  ; needle mask 0..3

.needle_case:
    mov edi, r13d
    lea rsi, [rel needle]
    mov edx, 2
    call .make_binary_case

    xor r14d, r14d                  ; needle length 0..2

.needle_length:
    lea rdi, [rel haystack]
    mov esi, 4
    lea rdx, [rel needle]
    mov ecx, r14d
    call bytes_find
    mov r15, rax

    lea rdi, [rel haystack]
    mov esi, 4
    lea rdx, [rel needle]
    mov ecx, r14d
    call .ref_find
    cmp r15, rax
    jne .find_fail

    lea rdi, [rel haystack]
    mov esi, 4
    lea rdx, [rel needle]
    mov ecx, r14d
    call bytes_starts_with
    mov r15d, eax

    lea rdi, [rel haystack]
    mov esi, 4
    lea rdx, [rel needle]
    mov ecx, r14d
    call .ref_starts_with
    cmp r15d, eax
    jne .find_fail

    lea rdi, [rel haystack]
    mov esi, 4
    lea rdx, [rel needle]
    mov ecx, r14d
    call bytes_ends_with
    mov r15d, eax

    lea rdi, [rel haystack]
    mov esi, 4
    lea rdx, [rel needle]
    mov ecx, r14d
    call .ref_ends_with
    cmp r15d, eax
    jne .find_fail

    inc r14d
    cmp r14d, 3
    jb .needle_length

    inc r13d
    cmp r13d, 4
    jb .needle_case

    inc r12d
    cmp r12d, 16
    jb .hay_case

    xor eax, eax
    ret

.find_fail:
    mov eax, 1
    ret

; ------------------------------------------------------------
; CRLF minimality: all 3^4 strings over {X, CR, LF}.
; Skip/trim: all 2^4 strings over simple finite alphabets.
; ------------------------------------------------------------
.test_scans:
    xor r12d, r12d

.crlf_case:
    mov edi, r12d
    lea rsi, [rel scan_buf]
    call .make_crlf_case

    lea rdi, [rel scan_buf]
    mov esi, 4
    call bytes_find_crlf
    mov r13, rax

    lea rdi, [rel scan_buf]
    mov esi, 4
    call .ref_find_crlf
    cmp r13, rax
    jne .scans_fail

    inc r12d
    cmp r12d, 81
    jb .crlf_case

    ; All binary 4-byte spans for skip_byte('a').
    xor r12d, r12d
.skip_case:
    mov edi, r12d
    lea rsi, [rel scan_buf]
    mov edx, 4
    call .make_binary_case

    lea rdi, [rel scan_buf]
    mov esi, 4
    mov edx, 'a'
    call bytes_skip_byte
    mov r13, rax

    lea rdi, [rel scan_buf]
    mov esi, 4
    mov edx, 'a'
    call .ref_skip_byte
    cmp r13, rax
    jne .scans_fail

    inc r12d
    cmp r12d, 16
    jb .skip_case

    ; All 4-byte masks where 0 -> ordinary space, 1 -> 'X'.
    xor r12d, r12d
.space_case:
    mov r8d, r12d
    lea r9, [rel scan_buf]
    xor r10d, r10d
.space_fill:
    mov eax, ' '
    test r8d, 1
    jz .space_store
    mov eax, 'X'
.space_store:
    mov [r9 + r10], al
    shr r8d, 1
    inc r10d
    cmp r10d, 4
    jb .space_fill

    lea rdi, [rel scan_buf]
    mov esi, 4
    call bytes_skip_ascii_space
    mov r13, rax

    lea rdi, [rel scan_buf]
    mov esi, 4
    call .ref_skip_ascii_space
    cmp r13, rax
    jne .scans_fail

    lea rdi, [rel scan_buf]
    mov esi, 4
    call bytes_trim_ascii_space
    mov r13, rax
    mov r14, rdx

    lea rdi, [rel scan_buf]
    mov esi, 4
    call .ref_trim_ascii_space
    cmp r13, rax
    jne .scans_fail
    cmp r14, rdx
    jne .scans_fail

    ; trim(trim(S)) = trim(S)
    mov rdi, r13
    mov rsi, r14
    call bytes_trim_ascii_space
    cmp rax, r13
    jne .scans_fail
    cmp rdx, r14
    jne .scans_fail

    inc r12d
    cmp r12d, 16
    jb .space_case

    xor eax, eax
    ret

.scans_fail:
    mov eax, 1
    ret

; ------------------------------------------------------------
; Test-only reference helpers.
; ------------------------------------------------------------

; EDI=mask, RSI=destination, EDX=count. bit 0 -> 'a', bit 1 -> 'b'.
.make_binary_case:
    mov r8d, edi
    xor ecx, ecx
.make_binary_loop:
    mov eax, 'a'
    test r8d, 1
    jz .make_binary_store
    mov eax, 'b'
.make_binary_store:
    mov [rsi + rcx], al
    shr r8d, 1
    inc ecx
    cmp ecx, edx
    jb .make_binary_loop
    ret

; EDI=base-3 code 0..80, RSI=destination. digits: X, CR, LF.
.make_crlf_case:
    mov eax, edi
    xor r8d, r8d
    mov r10d, 3
.make_crlf_loop:
    xor edx, edx
    div r10d
    mov ecx, 'X'
    cmp edx, 1
    jne .crlf_not_cr
    mov ecx, 13
    jmp .crlf_store
.crlf_not_cr:
    cmp edx, 2
    jne .crlf_store
    mov ecx, 10
.crlf_store:
    mov [rsi + r8], cl
    inc r8d
    cmp r8d, 4
    jb .make_crlf_loop
    ret

; Same ABI as bytes_find.
.ref_find:
    test rcx, rcx
    jz .ref_find_empty
    cmp rcx, rsi
    ja .ref_find_none
    mov r8, rsi
    sub r8, rcx
    inc r8
    xor r9d, r9d
.ref_find_candidate:
    xor r10d, r10d
.ref_find_compare:
    lea r11, [rdi + r9]
    mov al, [r11 + r10]
    cmp al, [rdx + r10]
    jne .ref_find_next
    inc r10
    cmp r10, rcx
    jb .ref_find_compare
    lea rax, [rdi + r9]
    ret
.ref_find_next:
    inc r9
    dec r8
    jnz .ref_find_candidate
.ref_find_none:
    xor eax, eax
    ret
.ref_find_empty:
    mov rax, rdi
    ret

.ref_starts_with:
    cmp rcx, rsi
    ja .ref_starts_no
    xor r8d, r8d
.ref_starts_loop:
    cmp r8, rcx
    jae .ref_starts_yes
    mov al, [rdi + r8]
    cmp al, [rdx + r8]
    jne .ref_starts_no
    inc r8
    jmp .ref_starts_loop
.ref_starts_yes:
    mov eax, 1
    ret
.ref_starts_no:
    xor eax, eax
    ret

.ref_ends_with:
    cmp rcx, rsi
    ja .ref_ends_no
    mov r8, rsi
    sub r8, rcx
    xor r9d, r9d
.ref_ends_loop:
    cmp r9, rcx
    jae .ref_ends_yes
    lea r11, [rdi + r8]
    mov al, [r11 + r9]
    cmp al, [rdx + r9]
    jne .ref_ends_no
    inc r9
    jmp .ref_ends_loop
.ref_ends_yes:
    mov eax, 1
    ret
.ref_ends_no:
    xor eax, eax
    ret

.ref_find_crlf:
    cmp rsi, 2
    jb .ref_crlf_none
    xor r8d, r8d
.ref_crlf_loop:
    mov al, [rdi + r8]
    cmp al, 13
    jne .ref_crlf_next
    cmp byte [rdi + r8 + 1], 10
    je .ref_crlf_yes
.ref_crlf_next:
    inc r8
    mov r9, rsi
    dec r9
    cmp r8, r9
    jb .ref_crlf_loop
.ref_crlf_none:
    xor eax, eax
    ret
.ref_crlf_yes:
    lea rax, [rdi + r8]
    ret

.ref_skip_byte:
    xor eax, eax
.ref_skip_loop:
    cmp rax, rsi
    jae .ref_skip_done
    cmp byte [rdi + rax], dl
    jne .ref_skip_done
    inc rax
    jmp .ref_skip_loop
.ref_skip_done:
    ret

.ref_skip_ascii_space:
    xor eax, eax
.ref_space_loop:
    cmp rax, rsi
    jae .ref_space_done
    movzx ecx, byte [rdi + rax]
    cmp ecx, 0x20
    je .ref_space_consume
    cmp ecx, 0x09
    jb .ref_space_done
    cmp ecx, 0x0D
    ja .ref_space_done
.ref_space_consume:
    inc rax
    jmp .ref_space_loop
.ref_space_done:
    ret

.ref_trim_ascii_space:
    mov rax, rdi
    mov rdx, rsi
.ref_trim_leading:
    test rdx, rdx
    jz .ref_trim_done
    movzx ecx, byte [rax]
    cmp ecx, 0x20
    je .ref_trim_lead_consume
    cmp ecx, 0x09
    jb .ref_trim_trailing
    cmp ecx, 0x0D
    ja .ref_trim_trailing
.ref_trim_lead_consume:
    inc rax
    dec rdx
    jmp .ref_trim_leading
.ref_trim_trailing:
    test rdx, rdx
    jz .ref_trim_done
    movzx ecx, byte [rax + rdx - 1]
    cmp ecx, 0x20
    je .ref_trim_trail_consume
    cmp ecx, 0x09
    jb .ref_trim_done
    cmp ecx, 0x0D
    ja .ref_trim_done
.ref_trim_trail_consume:
    dec rdx
    jmp .ref_trim_trailing
.ref_trim_done:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
