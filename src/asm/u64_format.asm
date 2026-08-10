; Arborcore unsigned 64-bit numeric formatting
;
; Formatting routines write raw byte spans. They do NOT append
; a NUL terminator.
;
; Error contract for formatting:
;   RAX = 0          success
;   RAX = -ENOSPC    destination capacity is too small
;
; Output contract:
;   RDX = bytes written on success
;   RDX = 0 on error
;
; A zero-capacity operation may use a NULL destination because
; no destination memory is accessed when the operation fails for
; insufficient capacity.

%define ERR_ENOSPC  -28


global u64_decimal_length:function
global u64_format_decimal:function
global u64_hex_length:function
global u64_format_hex:function


section .text


; ============================================================
; u64_decimal_length(value)
;
; Input:
;   RDI = value
;
; Return:
;   RAX = number of decimal digits (1..20)
; ============================================================

u64_decimal_length:
    mov rax, rdi
    mov ecx, 1

    cmp rax, 10
    jb .done

    mov r8d, 10

.loop:
    xor edx, edx
    div r8
    inc ecx

    cmp rax, 10
    jae .loop

.done:
    mov eax, ecx
    ret


; ============================================================
; u64_format_decimal(value, destination, capacity)
;
; Input:
;   RDI = value
;   RSI = destination address
;   RDX = destination capacity
;
; Return:
;   RAX = 0 / -ENOSPC
;   RDX = bytes written / 0 on error
;
; Output contains decimal ASCII without a terminating NUL.
; ============================================================

u64_format_decimal:
    ; Preserve value and capacity before DIV uses RAX/RDX.

    mov r8, rdi
    mov r9, rdx

    ; Determine required decimal length.

    mov rax, rdi
    mov ecx, 1
    mov r10d, 10

    cmp rax, 10
    jb .length_ready

.length_loop:
    xor edx, edx
    div r10
    inc ecx

    cmp rax, 10
    jae .length_loop

.length_ready:
    cmp r9, rcx
    jb .no_space

    ; Write backward from one byte past the final digit. This
    ; avoids a temporary reversal buffer.

    lea r11, [rsi + rcx]
    mov rax, r8

.write_digit:
    xor edx, edx
    div r10

    dec r11
    add dl, '0'
    mov [r11], dl

    test rax, rax
    jnz .write_digit

    mov rdx, rcx
    xor eax, eax
    ret

.no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    ret


; ============================================================
; u64_hex_length(value)
;
; Input:
;   RDI = value
;
; Return:
;   RAX = number of hexadecimal digits (1..16)
; ============================================================

u64_hex_length:
    mov rax, rdi
    mov ecx, 1

    cmp rax, 0x10
    jb .done

.loop:
    shr rax, 4
    inc ecx

    cmp rax, 0x10
    jae .loop

.done:
    mov eax, ecx
    ret


; ============================================================
; u64_format_hex(value, destination, capacity)
;
; Input:
;   RDI = value
;   RSI = destination address
;   RDX = destination capacity
;
; Return:
;   RAX = 0 / -ENOSPC
;   RDX = bytes written / 0 on error
;
; Output uses canonical lowercase hexadecimal digits and does not
; include a 0x prefix or terminating NUL.
; ============================================================

u64_format_hex:
    mov r8, rdi
    mov r9, rdx

    ; Determine required hexadecimal length.

    mov rax, rdi
    mov ecx, 1

    cmp rax, 0x10
    jb .length_ready

.length_loop:
    shr rax, 4
    inc ecx

    cmp rax, 0x10
    jae .length_loop

.length_ready:
    cmp r9, rcx
    jb .no_space

    lea r11, [rsi + rcx]
    mov rax, r8

.write_digit:
    mov edx, eax
    and edx, 0x0F

    cmp edx, 9
    jbe .numeric_digit

    add edx, 'a' - 10
    jmp .store_digit

.numeric_digit:
    add edx, '0'

.store_digit:
    dec r11
    mov [r11], dl

    shr rax, 4
    jnz .write_digit

    mov rdx, rcx
    xor eax, eax
    ret

.no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
