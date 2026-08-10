; Arborcore ASCII primitives
;
; These functions operate on one unsigned byte value passed in EDI.
; Only the low 8 bits are significant.
;
; ASCII whitespace for ascii_is_space is:
;   HT  0x09
;   LF  0x0A
;   VT  0x0B
;   FF  0x0C
;   CR  0x0D
;   SP  0x20
;
; No locale or Unicode semantics are applied.


global ascii_is_digit:function
global ascii_is_alpha:function
global ascii_is_hex_digit:function
global ascii_is_space:function
global ascii_to_lower:function
global ascii_to_upper:function


section .text


; ============================================================
; ascii_is_digit(value)
;
; Input:
;   EDI = byte value
;
; Return:
;   EAX = 1 for '0'..'9'
;   EAX = 0 otherwise
; ============================================================

ascii_is_digit:
    movzx eax, dil

    sub eax, '0'
    cmp eax, 9

    setbe al
    movzx eax, al
    ret


; ============================================================
; ascii_is_alpha(value)
;
; Return:
;   EAX = 1 for A-Z or a-z
;   EAX = 0 otherwise
; ============================================================

ascii_is_alpha:
    movzx eax, dil

    or eax, 0x20
    sub eax, 'a'
    cmp eax, 'z' - 'a'

    setbe al
    movzx eax, al
    ret


; ============================================================
; ascii_is_hex_digit(value)
;
; Return:
;   EAX = 1 for 0-9, A-F, or a-f
;   EAX = 0 otherwise
; ============================================================

ascii_is_hex_digit:
    movzx eax, dil

    mov ecx, eax
    sub ecx, '0'
    cmp ecx, 9
    jbe .yes

    or eax, 0x20
    sub eax, 'a'
    cmp eax, 'f' - 'a'
    jbe .yes

    xor eax, eax
    ret

.yes:
    mov eax, 1
    ret


; ============================================================
; ascii_is_space(value)
;
; Return:
;   EAX = 1 for ASCII whitespace
;   EAX = 0 otherwise
; ============================================================

ascii_is_space:
    movzx eax, dil

    cmp eax, 0x20
    je .yes

    sub eax, 0x09
    cmp eax, 0x0D - 0x09
    jbe .yes

    xor eax, eax
    ret

.yes:
    mov eax, 1
    ret


; ============================================================
; ascii_to_lower(value)
;
; Return:
;   EAX = ASCII lowercase form for A-Z
;   EAX = original unsigned byte otherwise
; ============================================================

ascii_to_lower:
    movzx eax, dil

    cmp eax, 'A'
    jb .done

    cmp eax, 'Z'
    ja .done

    or eax, 0x20

.done:
    ret


; ============================================================
; ascii_to_upper(value)
;
; Return:
;   EAX = ASCII uppercase form for a-z
;   EAX = original unsigned byte otherwise
; ============================================================

ascii_to_upper:
    movzx eax, dil

    cmp eax, 'a'
    jb .done

    cmp eax, 'z'
    ja .done

    and eax, ~0x20

.done:
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
