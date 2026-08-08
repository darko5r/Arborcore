; Arborcore byte-span engine
;
; Native Arborcore byte spans are represented as:
;
;   address + explicit length
;
; They are NOT required to be NUL terminated and may contain
; arbitrary byte values, including 0x00.
;
;
; Public primitives:
;
;   bytes_equal
;   bytes_compare
;   bytes_starts_with
;   bytes_ends_with
;   bytes_find
;   bytes_equal_ascii_ci
;   cstr_length
;
;
; Unless otherwise stated:
;
;   - lengths are unsigned
;   - zero-length operations perform no memory access
;   - nonzero spans must identify valid readable memory
;   - functions do not allocate memory


global bytes_equal:function
global bytes_compare:function
global bytes_starts_with:function
global bytes_ends_with:function
global bytes_find:function
global bytes_equal_ascii_ci:function
global cstr_length:function


section .text


; ============================================================
; bytes_equal(left, left_length, right, right_length)
;
; Input:
;   RDI = left address
;   RSI = left length
;   RDX = right address
;   RCX = right length
;
; Return:
;   RAX = 1   equal
;   RAX = 0   different
;
; Comparison is byte-exact.
; ============================================================

bytes_equal:
    cmp rsi, rcx
    jne .not_equal

    test rsi, rsi
    jz .equal


.compare_byte:
    mov al, [rdi]

    cmp al, [rdx]
    jne .not_equal

    inc rdi
    inc rdx

    dec rsi
    jnz .compare_byte


.equal:
    mov eax, 1
    ret


.not_equal:
    xor eax, eax
    ret


; ============================================================
; bytes_compare(left, left_length, right, right_length)
;
; Input:
;   RDI = left address
;   RSI = left length
;   RDX = right address
;   RCX = right length
;
; Return:
;   RAX = -1   left < right
;   RAX =  0   equal
;   RAX =  1   left > right
;
; Comparison is unsigned and lexicographical.
;
; Examples:
;
;   "abc"  < "abd"
;   "abc"  < "abcd"
;   0x80   > 0x7F
; ============================================================

bytes_compare:
    ; Preserve the two original lengths.

    mov r8, rsi
    mov r9, rcx


    ; R10 = minimum(left_length, right_length).

    mov r10, rsi

    cmp r10, rcx
    cmova r10, rcx

    test r10, r10
    jz .compare_lengths


.compare_byte:
    movzx eax, byte [rdi]
    movzx r11d, byte [rdx]

    cmp eax, r11d
    jb .less
    ja .greater

    inc rdi
    inc rdx

    dec r10
    jnz .compare_byte


.compare_lengths:
    cmp r8, r9
    jb .less
    ja .greater


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
; bytes_starts_with(span, span_length, prefix, prefix_length)
;
; Input:
;   RDI = span address
;   RSI = span length
;   RDX = prefix address
;   RCX = prefix length
;
; Return:
;   RAX = 1   yes
;   RAX = 0   no
;
; An empty prefix matches every span.
; ============================================================

bytes_starts_with:
    cmp rcx, rsi
    ja .no

    test rcx, rcx
    jz .yes


.compare_byte:
    mov al, [rdi]

    cmp al, [rdx]
    jne .no

    inc rdi
    inc rdx

    dec rcx
    jnz .compare_byte


.yes:
    mov eax, 1
    ret


.no:
    xor eax, eax
    ret


; ============================================================
; bytes_ends_with(span, span_length, suffix, suffix_length)
;
; Input:
;   RDI = span address
;   RSI = span length
;   RDX = suffix address
;   RCX = suffix length
;
; Return:
;   RAX = 1   yes
;   RAX = 0   no
;
; An empty suffix matches every span.
; ============================================================

bytes_ends_with:
    cmp rcx, rsi
    ja .no

    test rcx, rcx
    jz .yes


    ; Move RDI to:
    ;
    ;   span + span_length - suffix_length

    sub rsi, rcx
    add rdi, rsi


.compare_byte:
    mov al, [rdi]

    cmp al, [rdx]
    jne .no

    inc rdi
    inc rdx

    dec rcx
    jnz .compare_byte


.yes:
    mov eax, 1
    ret


.no:
    xor eax, eax
    ret


; ============================================================
; bytes_find(haystack, haystack_length, needle, needle_length)
;
; Input:
;   RDI = haystack address
;   RSI = haystack length
;   RDX = needle address
;   RCX = needle length
;
; Return:
;   RAX = address of first match
;   RAX = 0 if no match exists
;
; An empty needle matches at the beginning of the haystack.
;
; This first implementation deliberately uses a simple
; bounded search. More sophisticated search algorithms may be
; introduced later if actual parser workloads justify them.
; ============================================================

bytes_find:
    test rcx, rcx
    jz .empty_needle

    cmp rcx, rsi
    ja .not_found


    ; Number of possible starting positions:
    ;
    ;   haystack_length - needle_length + 1

    mov r8, rsi
    sub r8, rcx
    inc r8


    ; R9 = current candidate start.

    mov r9, rdi


.next_candidate:
    xor r10d, r10d


.compare_byte:
    mov al, [r9 + r10]

    cmp al, [rdx + r10]
    jne .candidate_failed

    inc r10

    cmp r10, rcx
    jb .compare_byte


    ; All needle bytes matched.

    mov rax, r9
    ret


.candidate_failed:
    inc r9

    dec r8
    jnz .next_candidate


.not_found:
    xor eax, eax
    ret


.empty_needle:
    mov rax, rdi
    ret


; ============================================================
; bytes_equal_ascii_ci(
;     left,
;     left_length,
;     right,
;     right_length
; )
;
; ASCII case-insensitive equality.
;
; Input:
;   RDI = left address
;   RSI = left length
;   RDX = right address
;   RCX = right length
;
; Return:
;   RAX = 1   equal under ASCII case folding
;   RAX = 0   different
;
; Only:
;
;   A-Z
;
; are folded to:
;
;   a-z
;
; All other byte values are compared literally.
;
; This is NOT locale-aware and is NOT Unicode case folding.
; ============================================================

bytes_equal_ascii_ci:
    cmp rsi, rcx
    jne .not_equal

    test rsi, rsi
    jz .equal


.compare_byte:
    movzx eax, byte [rdi]
    movzx r8d, byte [rdx]


    ; Normalize left ASCII uppercase letter.

    cmp eax, 'A'
    jb .left_ready

    cmp eax, 'Z'
    ja .left_ready

    or eax, 0x20


.left_ready:

    ; Normalize right ASCII uppercase letter.

    cmp r8d, 'A'
    jb .right_ready

    cmp r8d, 'Z'
    ja .right_ready

    or r8d, 0x20


.right_ready:
    cmp eax, r8d
    jne .not_equal

    inc rdi
    inc rdx

    dec rsi
    jnz .compare_byte


.equal:
    mov eax, 1
    ret


.not_equal:
    xor eax, eax
    ret


; ============================================================
; cstr_length(c_string)
;
; Input:
;   RDI = address of a valid NUL-terminated C string
;
; Return:
;   RAX = number of bytes preceding the terminating 0x00
;
; IMPORTANT:
;
; This is an interoperability helper.
;
; Native Arborcore code should prefer address + explicit length.
;
; The caller must provide a valid NUL-terminated string.
; This routine is intentionally not a bounded safety primitive.
; ============================================================

cstr_length:
    xor eax, eax


.scan:
    cmp byte [rdi + rax], 0
    je .done

    inc rax
    jmp .scan


.done:
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
