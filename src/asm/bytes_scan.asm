; Arborcore bounded byte-scanning primitives
;
; Native spans are represented as address + explicit length.
; Zero-length operations perform no memory access.


global bytes_find_crlf:function
global bytes_skip_byte:function
global bytes_skip_ascii_space:function
global bytes_trim_ascii_space:function


section .text


; ============================================================
; bytes_find_crlf(buffer, length)
;
; Input:
;   RDI = buffer address
;   RSI = length
;
; Return:
;   RAX = address of first CR in the first CRLF pair
;   RAX = 0 if no CRLF exists
; ============================================================

bytes_find_crlf:
    cmp rsi, 2
    jb .not_found

    xor r8d, r8d

    mov r9, rsi
    dec r9

.scan:
    cmp byte [rdi + r8], 0x0D
    jne .next

    cmp byte [rdi + r8 + 1], 0x0A
    je .found

.next:
    inc r8
    cmp r8, r9
    jb .scan

.not_found:
    xor eax, eax
    ret

.found:
    lea rax, [rdi + r8]
    ret


; ============================================================
; bytes_skip_byte(buffer, length, value)
;
; Input:
;   RDI = buffer address
;   RSI = length
;   EDX = byte value; low 8 bits used
;
; Return:
;   RAX = count of leading bytes equal to value
; ============================================================

bytes_skip_byte:
    xor eax, eax

    test rsi, rsi
    jz .done

.scan:
    cmp byte [rdi + rax], dl
    jne .done

    inc rax
    cmp rax, rsi
    jb .scan

.done:
    ret


; ============================================================
; bytes_skip_ascii_space(buffer, length)
;
; Input:
;   RDI = buffer address
;   RSI = length
;
; Return:
;   RAX = number of leading ASCII whitespace bytes
; ============================================================

bytes_skip_ascii_space:
    xor eax, eax

    test rsi, rsi
    jz .done

.scan:
    movzx ecx, byte [rdi + rax]

    cmp ecx, 0x20
    je .space

    sub ecx, 0x09
    cmp ecx, 0x0D - 0x09
    ja .done

.space:
    inc rax
    cmp rax, rsi
    jb .scan

.done:
    ret


; ============================================================
; bytes_trim_ascii_space(buffer, length)
;
; Input:
;   RDI = buffer address
;   RSI = length
;
; Return:
;   RAX = address of first non-space byte
;   RDX = trimmed length
;
; If every byte is whitespace:
;   RAX = buffer + original length
;   RDX = 0
; ============================================================

bytes_trim_ascii_space:
    mov rax, rdi
    mov rdx, rsi

    test rdx, rdx
    jz .done

.leading:
    movzx ecx, byte [rax]

    cmp ecx, 0x20
    je .consume_leading

    sub ecx, 0x09
    cmp ecx, 0x0D - 0x09
    ja .trailing

.consume_leading:
    inc rax
    dec rdx
    jnz .leading

    ret

.trailing:
    test rdx, rdx
    jz .done

    lea r8, [rax + rdx - 1]
    movzx ecx, byte [r8]

    cmp ecx, 0x20
    je .consume_trailing

    sub ecx, 0x09
    cmp ecx, 0x0D - 0x09
    ja .done

.consume_trailing:
    dec rdx
    jmp .trailing

.done:
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
