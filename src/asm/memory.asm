; Arborcore memory engine
;
; Public primitives:
;
;   memory_copy
;   memory_move
;   memory_set
;   memory_zero
;   memory_compare
;   memory_find_byte
;
; Copy implementations retained for qualification/testing:
;
;   memory_copy_scalar
;   memory_copy_qword
;   memory_copy_rep


%include "generated/memory_thresholds.inc"


global memory_copy:function
global memory_copy_scalar:function
global memory_copy_qword:function
global memory_copy_rep:function

global memory_move:function
global memory_set:function
global memory_zero:function
global memory_compare:function
global memory_find_byte:function


section .text


; ============================================================
; memory_copy(destination, source, length)
;
; Input:
;   RDI = destination
;   RSI = source
;   RDX = number of bytes
;
; Return:
;   RAX = original destination
;
; Contract:
;   - zero length performs no memory access
;   - source/destination must otherwise be valid
;   - overlapping regions are NOT supported
;
; Strategy is selected from the qualified machine policy.
; ============================================================

memory_copy:
    cmp rdx, MEMORY_COPY_QWORD_MIN
    jb memory_copy_scalar

    cmp rdx, MEMORY_COPY_REP_MIN
    jb memory_copy_qword

    jmp memory_copy_rep


; ============================================================
; memory_copy_scalar
;
; Explicit byte-at-a-time reference implementation.
; ============================================================

memory_copy_scalar:
    mov rax, rdi

    test rdx, rdx
    jz .done


.copy_byte:
    mov cl, [rsi]
    mov [rdi], cl

    inc rsi
    inc rdi

    dec rdx
    jnz .copy_byte


.done:
    ret


; ============================================================
; memory_copy_qword
;
; Copies complete 8-byte chunks followed by a 0-7 byte tail.
; ============================================================

memory_copy_qword:
    mov rax, rdi

    mov rcx, rdx
    shr rcx, 3

    jz .tail


.copy_qword:
    mov r8, [rsi]
    mov [rdi], r8

    add rsi, 8
    add rdi, 8

    dec rcx
    jnz .copy_qword


.tail:
    and rdx, 7
    jz .done


.copy_byte:
    mov r8b, [rsi]
    mov [rdi], r8b

    inc rsi
    inc rdi

    dec rdx
    jnz .copy_byte


.done:
    ret


; ============================================================
; memory_copy_rep
;
; REP MOVSB implementation.
;
; Relies on the System V AMD64 ABI requirement that DF is
; clear across normal function boundaries.
; ============================================================

memory_copy_rep:
    mov rax, rdi
    mov rcx, rdx

    rep movsb

    ret


; ============================================================
; memory_set(destination, value, length)
;
; Similar contract to C memset().
;
; Input:
;   RDI = destination
;   RSI = value; low 8 bits are used
;   RDX = number of bytes
;
; Return:
;   RAX = original destination
;
; Notes:
;   - zero length performs no memory access
;   - uses REP STOSB
; ============================================================

memory_set:
    mov r8, rdi

    mov eax, esi
    mov rcx, rdx

    rep stosb

    mov rax, r8
    ret


; ============================================================
; memory_zero(destination, length)
;
; Input:
;   RDI = destination
;   RSI = length
;
; Return:
;   RAX = original destination
;
; memory_zero is deliberately implemented through memory_set
; so zero filling has one underlying semantic implementation.
;
; This is an ordinary logical zero-fill primitive, not a secure-clear
; guarantee.  A dedicated security primitive is qualified separately.
; ============================================================

memory_zero:
    mov rdx, rsi
    xor esi, esi

    jmp memory_set


; ============================================================
; memory_compare(left, right, length)
;
; Input:
;   RDI = left range
;   RSI = right range
;   RDX = number of bytes
;
; Return:
;   RAX = -1   left < right
;   RAX =  0   equal
;   RAX =  1   left > right
;
; Comparison is unsigned and lexicographical.
; This routine exits at the first differing byte and therefore does not
; provide constant-time equality semantics.
; ============================================================

memory_compare:
    test rdx, rdx
    jz .equal


.compare_byte:
    movzx ecx, byte [rdi]
    movzx r8d, byte [rsi]

    cmp ecx, r8d
    jb .less
    ja .greater

    inc rdi
    inc rsi

    dec rdx
    jnz .compare_byte


.equal:
    xor eax, eax
    ret


.less:
    mov rax, -1
    ret


.greater:
    mov eax, 1
    ret


; ============================================================
; memory_find_byte(buffer, value, length)
;
; Input:
;   RDI = buffer
;   RSI = value; low 8 bits are searched for
;   RDX = number of bytes
;
; Return:
;   RAX = address of first matching byte
;   RAX = 0 if not found
;
; Uses REPNE SCASB.
; ============================================================

memory_find_byte:
    mov eax, esi
    mov rcx, rdx

    test rcx, rcx
    jz .not_found

    repne scasb

    jne .not_found

    lea rax, [rdi - 1]
    ret


.not_found:
    xor eax, eax
    ret


; ============================================================
; memory_move(destination, source, length)
;
; memmove-like overlap-safe copy.
;
; Input:
;   RDI = destination
;   RSI = source
;   RDX = number of bytes
;
; Return:
;   RAX = original destination
;
; Cases:
;
;   destination < source:
;       forward copy is safe
;
;   destination >= source + length:
;       no destructive overlap; forward copy is safe
;
;   source < destination < source + length:
;       copy backward to avoid destroying unread source bytes
;
; DF is always restored clear before returning.
; ============================================================

memory_move:
    mov rax, rdi

    test rdx, rdx
    jz .done

    cmp rdi, rsi
    je .done

    jb .forward


    ; Destination lies above source.
    ;
    ; Calculate distance without forming source+length, thereby
    ; avoiding an unnecessary potentially overflowing address
    ; calculation.

    mov r8, rdi
    sub r8, rsi

    cmp r8, rdx
    jae .forward


    ; --------------------------------------------------------
    ; Destructive forward overlap:
    ;
    ; source:
    ;       [a b c d e f g h]
    ;
    ; destination:
    ;           [...............]
    ;
    ; Begin at the high end and move downward.
    ; --------------------------------------------------------

    lea rsi, [rsi + rdx - 1]
    lea rdi, [rdi + rdx - 1]

    mov rcx, rdx

    std
    rep movsb
    cld

    ret


.forward:
    mov rcx, rdx

    rep movsb


.done:
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
