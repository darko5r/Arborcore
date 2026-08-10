; Arborcore numeric engine tests
;
; Exit status:
;   0 = all tests passed
;   1 = u64_add_checked failed
;   2 = u64_sub_checked failed
;   3 = u64_mul_checked failed
;   4 = u64_align_up_checked failed
;   5 = u64_decimal_length failed
;   6 = u64_format_decimal failed
;   7 = u64_hex_length failed
;   8 = u64_format_hex failed

%define SYS_EXIT        60
%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75


global _start


extern u64_add_checked
extern u64_sub_checked
extern u64_mul_checked
extern u64_align_up_checked
extern u64_decimal_length
extern u64_format_decimal
extern u64_hex_length
extern u64_format_hex


section .rodata

expected_dec_zero:
    db "0"

expected_dec_12345:
    db "12345"

expected_dec_max:
    db "18446744073709551615"

expected_hex_zero:
    db "0"

expected_hex_1a2b:
    db "1a2b"

expected_hex_max:
    db "ffffffffffffffff"


section .bss

align 16
dec_buffer:
    resb 32

align 16
hex_buffer:
    resb 32


section .text


_start:
    cld

    ; ========================================================
    ; u64_add_checked
    ; ========================================================

    xor edi, edi
    xor esi, esi
    call u64_add_checked

    test rax, rax
    jnz .fail_add
    test rdx, rdx
    jnz .fail_add


    mov edi, 1
    mov esi, 2
    call u64_add_checked

    test rax, rax
    jnz .fail_add
    cmp rdx, 3
    jne .fail_add


    mov rdi, -1
    xor esi, esi
    call u64_add_checked

    test rax, rax
    jnz .fail_add
    cmp rdx, -1
    jne .fail_add


    mov rdi, -1
    mov esi, 1
    call u64_add_checked

    cmp rax, ERR_EOVERFLOW
    jne .fail_add
    test rdx, rdx
    jnz .fail_add


    mov rdi, 0xFFFFFFFFFFFFFFFE
    mov esi, 1
    call u64_add_checked

    test rax, rax
    jnz .fail_add
    cmp rdx, -1
    jne .fail_add


    ; ========================================================
    ; u64_sub_checked
    ; ========================================================

    mov edi, 5
    mov esi, 3
    call u64_sub_checked

    test rax, rax
    jnz .fail_sub
    cmp rdx, 2
    jne .fail_sub


    xor edi, edi
    xor esi, esi
    call u64_sub_checked

    test rax, rax
    jnz .fail_sub
    test rdx, rdx
    jnz .fail_sub


    mov rdi, -1
    mov rsi, -1
    call u64_sub_checked

    test rax, rax
    jnz .fail_sub
    test rdx, rdx
    jnz .fail_sub


    xor edi, edi
    mov esi, 1
    call u64_sub_checked

    cmp rax, ERR_EOVERFLOW
    jne .fail_sub
    test rdx, rdx
    jnz .fail_sub


    ; ========================================================
    ; u64_mul_checked
    ; ========================================================

    xor edi, edi
    mov rsi, -1
    call u64_mul_checked

    test rax, rax
    jnz .fail_mul
    test rdx, rdx
    jnz .fail_mul


    mov edi, 1
    mov rsi, -1
    call u64_mul_checked

    test rax, rax
    jnz .fail_mul
    cmp rdx, -1
    jne .fail_mul


    mov edi, 12
    mov esi, 34
    call u64_mul_checked

    test rax, rax
    jnz .fail_mul
    cmp rdx, 408
    jne .fail_mul


    mov rdi, -1
    mov esi, 2
    call u64_mul_checked

    cmp rax, ERR_EOVERFLOW
    jne .fail_mul
    test rdx, rdx
    jnz .fail_mul


    mov rdi, 0x100000000
    mov rsi, 0x100000000
    call u64_mul_checked

    cmp rax, ERR_EOVERFLOW
    jne .fail_mul
    test rdx, rdx
    jnz .fail_mul


    ; ========================================================
    ; u64_align_up_checked
    ; ========================================================

    xor edi, edi
    mov esi, 1
    call u64_align_up_checked

    test rax, rax
    jnz .fail_align
    test rdx, rdx
    jnz .fail_align


    mov edi, 1
    mov esi, 1
    call u64_align_up_checked

    test rax, rax
    jnz .fail_align
    cmp rdx, 1
    jne .fail_align


    mov edi, 1
    mov esi, 8
    call u64_align_up_checked

    test rax, rax
    jnz .fail_align
    cmp rdx, 8
    jne .fail_align


    mov edi, 8
    mov esi, 8
    call u64_align_up_checked

    test rax, rax
    jnz .fail_align
    cmp rdx, 8
    jne .fail_align


    mov edi, 9
    mov esi, 8
    call u64_align_up_checked

    test rax, rax
    jnz .fail_align
    cmp rdx, 16
    jne .fail_align


    ; Highest representable multiple of eight remains valid.

    mov rdi, -8
    mov esi, 8
    call u64_align_up_checked

    test rax, rax
    jnz .fail_align
    cmp rdx, -8
    jne .fail_align


    ; Largest legal power-of-two alignment.

    mov edi, 1
    mov rsi, 0x8000000000000000
    call u64_align_up_checked

    test rax, rax
    jnz .fail_align
    mov rcx, 0x8000000000000000
    cmp rdx, rcx
    jne .fail_align


    ; Already aligned at the largest legal alignment.

    mov rdi, 0x8000000000000000
    mov rsi, 0x8000000000000000
    call u64_align_up_checked

    test rax, rax
    jnz .fail_align
    mov rcx, 0x8000000000000000
    cmp rdx, rcx
    jne .fail_align


    ; One byte above that boundary cannot be rounded upward.

    mov rdi, 0x8000000000000001
    mov rsi, 0x8000000000000000
    call u64_align_up_checked

    cmp rax, ERR_EOVERFLOW
    jne .fail_align
    test rdx, rdx
    jnz .fail_align


    ; Zero alignment is invalid.

    mov edi, 10
    xor esi, esi
    call u64_align_up_checked

    cmp rax, ERR_EINVAL
    jne .fail_align
    test rdx, rdx
    jnz .fail_align


    ; Non-power-of-two alignment is invalid.

    mov edi, 10
    mov esi, 3
    call u64_align_up_checked

    cmp rax, ERR_EINVAL
    jne .fail_align
    test rdx, rdx
    jnz .fail_align


    ; UINT64_MAX cannot be rounded upward to alignment 2.

    mov rdi, -1
    mov esi, 2
    call u64_align_up_checked

    cmp rax, ERR_EOVERFLOW
    jne .fail_align
    test rdx, rdx
    jnz .fail_align


    ; ========================================================
    ; u64_decimal_length
    ; ========================================================

    xor edi, edi
    call u64_decimal_length
    cmp rax, 1
    jne .fail_dec_length

    mov edi, 9
    call u64_decimal_length
    cmp rax, 1
    jne .fail_dec_length

    mov edi, 10
    call u64_decimal_length
    cmp rax, 2
    jne .fail_dec_length

    mov edi, 99
    call u64_decimal_length
    cmp rax, 2
    jne .fail_dec_length

    mov edi, 100
    call u64_decimal_length
    cmp rax, 3
    jne .fail_dec_length

    mov rdi, -1
    call u64_decimal_length
    cmp rax, 20
    jne .fail_dec_length


    ; ========================================================
    ; u64_format_decimal
    ; ========================================================

    ; Zero with exact capacity.

    mov byte [rel dec_buffer], 0xA5
    mov byte [rel dec_buffer + 2], 0x5A

    xor edi, edi
    lea rsi, [rel dec_buffer + 1]
    mov edx, 1
    call u64_format_decimal

    test rax, rax
    jnz .fail_dec_format
    cmp rdx, 1
    jne .fail_dec_format
    cmp byte [rel dec_buffer], 0xA5
    jne .fail_dec_format
    cmp byte [rel dec_buffer + 1], '0'
    jne .fail_dec_format
    cmp byte [rel dec_buffer + 2], 0x5A
    jne .fail_dec_format


    ; Ordinary value with guards immediately before and after.

    mov byte [rel dec_buffer], 0xA5
    mov byte [rel dec_buffer + 6], 0x5A

    mov edi, 12345
    lea rsi, [rel dec_buffer + 1]
    mov edx, 5
    call u64_format_decimal

    test rax, rax
    jnz .fail_dec_format
    cmp rdx, 5
    jne .fail_dec_format
    cmp byte [rel dec_buffer], 0xA5
    jne .fail_dec_format
    cmp byte [rel dec_buffer + 6], 0x5A
    jne .fail_dec_format

    lea rdi, [rel dec_buffer + 1]
    lea rsi, [rel expected_dec_12345]
    mov edx, 5
    call .bytes_match
    cmp eax, 1
    jne .fail_dec_format


    ; Extra capacity must remain untouched after the emitted span.
    ;
    ; This locks in the Arborcore byte-span contract:
    ; no padding and no implicit NUL terminator.

    mov byte [rel dec_buffer + 5], 0xA5
    mov byte [rel dec_buffer + 6], 0x5A

    mov edi, 12345
    lea rsi, [rel dec_buffer]
    mov edx, 32
    call u64_format_decimal

    test rax, rax
    jnz .fail_dec_format
    cmp rdx, 5
    jne .fail_dec_format

    lea rdi, [rel dec_buffer]
    lea rsi, [rel expected_dec_12345]
    mov edx, 5
    call .bytes_match
    cmp eax, 1
    jne .fail_dec_format

    cmp byte [rel dec_buffer + 5], 0xA5
    jne .fail_dec_format
    cmp byte [rel dec_buffer + 6], 0x5A
    jne .fail_dec_format


    ; UINT64_MAX with exact capacity.

    mov rdi, -1
    lea rsi, [rel dec_buffer]
    mov edx, 20
    call u64_format_decimal

    test rax, rax
    jnz .fail_dec_format
    cmp rdx, 20
    jne .fail_dec_format

    lea rdi, [rel dec_buffer]
    lea rsi, [rel expected_dec_max]
    mov edx, 20
    call .bytes_match
    cmp eax, 1
    jne .fail_dec_format


    ; Insufficient capacity: fail before writing anything.

    mov byte [rel dec_buffer], 0xA5
    mov byte [rel dec_buffer + 1], 0xA5
    mov byte [rel dec_buffer + 2], 0xA5
    mov byte [rel dec_buffer + 3], 0xA5
    mov byte [rel dec_buffer + 4], 0xA5

    mov edi, 12345
    lea rsi, [rel dec_buffer]
    mov edx, 4
    call u64_format_decimal

    cmp rax, ERR_ENOSPC
    jne .fail_dec_format
    test rdx, rdx
    jnz .fail_dec_format

    cmp byte [rel dec_buffer], 0xA5
    jne .fail_dec_format
    cmp byte [rel dec_buffer + 1], 0xA5
    jne .fail_dec_format
    cmp byte [rel dec_buffer + 2], 0xA5
    jne .fail_dec_format
    cmp byte [rel dec_buffer + 3], 0xA5
    jne .fail_dec_format
    cmp byte [rel dec_buffer + 4], 0xA5
    jne .fail_dec_format


    ; Zero capacity may use NULL and must not dereference it.

    mov edi, 1
    xor esi, esi
    xor edx, edx
    call u64_format_decimal

    cmp rax, ERR_ENOSPC
    jne .fail_dec_format
    test rdx, rdx
    jnz .fail_dec_format


    ; ========================================================
    ; u64_hex_length
    ; ========================================================

    xor edi, edi
    call u64_hex_length
    cmp rax, 1
    jne .fail_hex_length

    mov edi, 15
    call u64_hex_length
    cmp rax, 1
    jne .fail_hex_length

    mov edi, 16
    call u64_hex_length
    cmp rax, 2
    jne .fail_hex_length

    mov edi, 0xFF
    call u64_hex_length
    cmp rax, 2
    jne .fail_hex_length

    mov edi, 0x100
    call u64_hex_length
    cmp rax, 3
    jne .fail_hex_length

    mov rdi, -1
    call u64_hex_length
    cmp rax, 16
    jne .fail_hex_length


    ; ========================================================
    ; u64_format_hex
    ; ========================================================

    ; Zero with exact capacity.

    mov byte [rel hex_buffer], 0xA5
    mov byte [rel hex_buffer + 2], 0x5A

    xor edi, edi
    lea rsi, [rel hex_buffer + 1]
    mov edx, 1
    call u64_format_hex

    test rax, rax
    jnz .fail_hex_format
    cmp rdx, 1
    jne .fail_hex_format
    cmp byte [rel hex_buffer], 0xA5
    jne .fail_hex_format
    cmp byte [rel hex_buffer + 1], '0'
    jne .fail_hex_format
    cmp byte [rel hex_buffer + 2], 0x5A
    jne .fail_hex_format


    ; Mixed decimal/hexadecimal digits are emitted lowercase.

    mov byte [rel hex_buffer], 0xA5
    mov byte [rel hex_buffer + 5], 0x5A

    mov edi, 0x1A2B
    lea rsi, [rel hex_buffer + 1]
    mov edx, 4
    call u64_format_hex

    test rax, rax
    jnz .fail_hex_format
    cmp rdx, 4
    jne .fail_hex_format
    cmp byte [rel hex_buffer], 0xA5
    jne .fail_hex_format
    cmp byte [rel hex_buffer + 5], 0x5A
    jne .fail_hex_format

    lea rdi, [rel hex_buffer + 1]
    lea rsi, [rel expected_hex_1a2b]
    mov edx, 4
    call .bytes_match
    cmp eax, 1
    jne .fail_hex_format


    ; Extra capacity must not produce padding or a NUL terminator.

    mov byte [rel hex_buffer + 4], 0xA5
    mov byte [rel hex_buffer + 5], 0x5A

    mov edi, 0x1A2B
    lea rsi, [rel hex_buffer]
    mov edx, 32
    call u64_format_hex

    test rax, rax
    jnz .fail_hex_format
    cmp rdx, 4
    jne .fail_hex_format

    lea rdi, [rel hex_buffer]
    lea rsi, [rel expected_hex_1a2b]
    mov edx, 4
    call .bytes_match
    cmp eax, 1
    jne .fail_hex_format

    cmp byte [rel hex_buffer + 4], 0xA5
    jne .fail_hex_format
    cmp byte [rel hex_buffer + 5], 0x5A
    jne .fail_hex_format


    ; UINT64_MAX with exact capacity.

    mov rdi, -1
    lea rsi, [rel hex_buffer]
    mov edx, 16
    call u64_format_hex

    test rax, rax
    jnz .fail_hex_format
    cmp rdx, 16
    jne .fail_hex_format

    lea rdi, [rel hex_buffer]
    lea rsi, [rel expected_hex_max]
    mov edx, 16
    call .bytes_match
    cmp eax, 1
    jne .fail_hex_format


    ; Insufficient capacity: no write.

    mov byte [rel hex_buffer], 0xA5
    mov byte [rel hex_buffer + 1], 0xA5
    mov byte [rel hex_buffer + 2], 0xA5
    mov byte [rel hex_buffer + 3], 0xA5

    mov edi, 0x1A2B
    lea rsi, [rel hex_buffer]
    mov edx, 3
    call u64_format_hex

    cmp rax, ERR_ENOSPC
    jne .fail_hex_format
    test rdx, rdx
    jnz .fail_hex_format

    cmp byte [rel hex_buffer], 0xA5
    jne .fail_hex_format
    cmp byte [rel hex_buffer + 1], 0xA5
    jne .fail_hex_format
    cmp byte [rel hex_buffer + 2], 0xA5
    jne .fail_hex_format
    cmp byte [rel hex_buffer + 3], 0xA5
    jne .fail_hex_format


    ; Zero capacity may use NULL.

    mov edi, 1
    xor esi, esi
    xor edx, edx
    call u64_format_hex

    cmp rax, ERR_ENOSPC
    jne .fail_hex_format
    test rdx, rdx
    jnz .fail_hex_format


.success:
    xor edi, edi
    jmp .exit


.fail_add:
    mov edi, 1
    jmp .exit

.fail_sub:
    mov edi, 2
    jmp .exit

.fail_mul:
    mov edi, 3
    jmp .exit

.fail_align:
    mov edi, 4
    jmp .exit

.fail_dec_length:
    mov edi, 5
    jmp .exit

.fail_dec_format:
    mov edi, 6
    jmp .exit

.fail_hex_length:
    mov edi, 7
    jmp .exit

.fail_hex_format:
    mov edi, 8
    jmp .exit


.exit:
    mov eax, SYS_EXIT
    syscall


; ============================================================
; Local test helper
;
; bytes_match(actual, expected, length)
;
; Input:
;   RDI = actual
;   RSI = expected
;   RDX = length
;
; Return:
;   RAX = 1 equal
;   RAX = 0 different
; ============================================================

.bytes_match:
    test rdx, rdx
    jz .bytes_equal

    mov rcx, rdx
    repe cmpsb
    jne .bytes_not_equal

.bytes_equal:
    mov eax, 1
    ret

.bytes_not_equal:
    xor eax, eax
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
