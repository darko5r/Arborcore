; Arborcore Core Retrofit B1 numeric parse/format qualification
;
; Exit status:
;   0 = all properties passed
;   1 = decimal round-trip/length property failed
;   2 = hexadecimal round-trip/length property failed
;   3 = boundary lattice property failed

%define SYS_EXIT 60

global _start

extern bytes_parse_u64_decimal
extern bytes_parse_u64_hex
extern u64_decimal_length
extern u64_format_decimal
extern u64_hex_length
extern u64_format_hex

section .rodata
align 8
boundary_values:
    dq 0
    dq 1
    dq 9
    dq 10
    dq 15
    dq 16
    dq 99
    dq 100
    dq 255
    dq 256
    dq 999
    dq 1000
    dq 0xFFFF
    dq 0x10000
    dq 0xFFFFFFFF
    dq 0x100000000
    dq 0x7FFFFFFFFFFFFFFF
    dq 0x8000000000000000
    dq 0xFFFFFFFFFFFFFFFE
    dq 0xFFFFFFFFFFFFFFFF
boundary_count equ ($ - boundary_values) / 8

section .bss
align 16
dec_buffer: resb 32
hex_buffer: resb 32

section .text

_start:
    cld

    ; Exhaust deterministic dense domain 0..65535.
    xor r12d, r12d
.dense_loop:
    mov rdi, r12
    call .check_decimal
    test eax, eax
    jnz .fail_decimal

    mov rdi, r12
    call .check_hex
    test eax, eax
    jnz .fail_hex

    inc r12d
    cmp r12d, 65536
    jb .dense_loop

    ; Explicit high-value/boundary lattice.
    xor r12d, r12d
    lea rbx, [rel boundary_values]
.boundary_loop:
    mov rdi, [rbx + r12 * 8]
    mov r13, rdi

    call .check_decimal
    test eax, eax
    jnz .fail_boundary

    mov rdi, r13
    call .check_hex
    test eax, eax
    jnz .fail_boundary

    inc r12d
    cmp r12d, boundary_count
    jb .boundary_loop

    xor edi, edi
    jmp .exit

.fail_decimal:
    mov edi, 1
    jmp .exit
.fail_hex:
    mov edi, 2
    jmp .exit
.fail_boundary:
    mov edi, 3
.exit:
    mov eax, SYS_EXIT
    syscall

; RDI=value, EAX=0 pass / 1 fail
.check_decimal:
    mov r14, rdi

    call u64_decimal_length
    mov r15, rax

    mov rdi, r14
    call .ref_decimal_length
    cmp r15, rax
    jne .check_fail

    mov rdi, r14
    lea rsi, [rel dec_buffer]
    mov edx, 32
    call u64_format_decimal
    test rax, rax
    jnz .check_fail
    cmp rdx, r15
    jne .check_fail

    ; Canonical formatting: no leading zero for nonzero multi-digit value.
    cmp r14, 0
    je .decimal_parse
    cmp r15, 1
    jbe .decimal_parse
    cmp byte [rel dec_buffer], '0'
    je .check_fail

.decimal_parse:
    lea rdi, [rel dec_buffer]
    mov rsi, r15
    call bytes_parse_u64_decimal
    test rax, rax
    jnz .check_fail
    cmp rdx, r14
    jne .check_fail

    xor eax, eax
    ret

.check_hex:
    mov r14, rdi

    call u64_hex_length
    mov r15, rax

    mov rdi, r14
    call .ref_hex_length
    cmp r15, rax
    jne .check_fail

    mov rdi, r14
    lea rsi, [rel hex_buffer]
    mov edx, 32
    call u64_format_hex
    test rax, rax
    jnz .check_fail
    cmp rdx, r15
    jne .check_fail

    ; Canonical lowercase hexadecimal grammar and no leading zero.
    xor r8d, r8d
    lea r9, [rel hex_buffer]
.hex_grammar_loop:
    cmp r8, r15
    jae .hex_grammar_done
    movzx ecx, byte [r9 + r8]
    cmp ecx, '0'
    jb .check_fail
    cmp ecx, '9'
    jbe .hex_grammar_next
    cmp ecx, 'a'
    jb .check_fail
    cmp ecx, 'f'
    ja .check_fail
.hex_grammar_next:
    inc r8
    jmp .hex_grammar_loop
.hex_grammar_done:
    cmp r14, 0
    je .hex_parse
    cmp r15, 1
    jbe .hex_parse
    cmp byte [rel hex_buffer], '0'
    je .check_fail

.hex_parse:
    lea rdi, [rel hex_buffer]
    mov rsi, r15
    call bytes_parse_u64_hex
    test rax, rax
    jnz .check_fail
    cmp rdx, r14
    jne .check_fail

    xor eax, eax
    ret

.check_fail:
    mov eax, 1
    ret

; Independent reference decimal length by repeated division.
.ref_decimal_length:
    mov rax, rdi
    mov ecx, 1
    mov r8d, 10
.ref_dec_loop:
    cmp rax, 10
    jb .ref_dec_done
    xor edx, edx
    div r8
    inc ecx
    jmp .ref_dec_loop
.ref_dec_done:
    mov eax, ecx
    ret

; Independent reference hexadecimal length by nibble shifts.
.ref_hex_length:
    mov rax, rdi
    mov ecx, 1
.ref_hex_loop:
    cmp rax, 16
    jb .ref_hex_done
    shr rax, 4
    inc ecx
    jmp .ref_hex_loop
.ref_hex_done:
    mov eax, ecx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
