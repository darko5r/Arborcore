; Arborcore HTTP0 zero-copy HTTP/1.1 header field-line scanner.
;
; Internal MVC/HTTP-layer symbol; not part of frozen Assembly ABI v1.
;
; http0_header_next_asm(headers, headers_length, cursor, field_out)
;   RDI = header-section pointer
;   RSI = header-section length (includes each field-line CRLF, excludes final empty line)
;   RDX = byte cursor within header section
;   RCX = arbor_http_field* output
;
; Return arbor_asm_result_u64:
;   RAX = 0 success, -EINVAL / -EOVERFLOW failure
;   RDX = next cursor on success, 0 on failure
;
; The field output borrows slices directly from the request header section.
; Failure does not modify the output object.

%define ERR_EINVAL    -22
%define ERR_EOVERFLOW -75

%define FIELD_NAME_PTR   0
%define FIELD_NAME_LEN   8
%define FIELD_VALUE_PTR 16
%define FIELD_VALUE_LEN 24

global http0_header_next_asm:function hidden

section .text

http0_header_next_asm:
    push rbx
    sub rsp, 32

    test rcx, rcx
    jz .invalid
    cmp rdx, rsi
    jae .invalid
    test rsi, rsi
    jz .invalid
    test rdi, rdi
    jz .invalid

    mov r8, rdi                  ; header base
    mov r9, rsi                  ; header length
    mov r10, rdx                ; entry cursor / field-name index
    mov rbx, rcx                 ; output pointer

    mov rax, r8
    add rax, r9
    jc .overflow                 ; source span must be representable

    ; Find the CRLF terminating the current field-line. Bare CR/LF is invalid.
    mov rdx, r10
.find_crlf:
    cmp rdx, r9
    jae .invalid
    mov al, [r8 + rdx]
    cmp al, 13
    je .cr_found
    cmp al, 10
    je .invalid
    inc rdx
    jmp .find_crlf

.cr_found:
    lea rax, [rdx + 1]
    cmp rax, r9
    jae .invalid
    cmp byte [r8 + rdx + 1], 10
    jne .invalid
    cmp rdx, r10
    je .invalid                  ; empty field-line is outside this span contract
    lea rax, [rdx + 2]
    mov [rsp + 24], rax          ; next cursor

    ; Find ':' and validate RFC token grammar for the field name.
    mov rcx, r10
.find_colon:
    cmp rcx, rdx
    jae .invalid
    mov al, [r8 + rcx]
    cmp al, ':'
    je .colon_found
    call .token_char
    jc .invalid
    inc rcx
    jmp .find_colon

.colon_found:
    cmp rcx, r10
    je .invalid
    mov rax, rcx
    sub rax, r10
    mov [rsp + 0], rax           ; name length

    ; Value begins after ':'. Trim HTTP OWS (SP / HTAB) at both ends.
    lea rsi, [rcx + 1]           ; value start index
.trim_leading:
    cmp rsi, rdx
    jae .trim_leading_done
    mov al, [r8 + rsi]
    cmp al, 0x20
    je .consume_leading
    cmp al, 0x09
    jne .trim_leading_done
.consume_leading:
    inc rsi
    jmp .trim_leading

.trim_leading_done:
    mov rdi, rdx                 ; exclusive value end index
.trim_trailing:
    cmp rdi, rsi
    jbe .trim_done
    mov al, [r8 + rdi - 1]
    cmp al, 0x20
    je .consume_trailing
    cmp al, 0x09
    jne .trim_done
.consume_trailing:
    dec rdi
    jmp .trim_trailing

.trim_done:
    lea rax, [r8 + rsi]
    mov [rsp + 8], rax           ; value pointer
    mov rax, rdi
    sub rax, rsi
    mov [rsp + 16], rax          ; value length

    ; Field-value bytes: HTAB / SP / VCHAR / obs-text. CR/LF and other
    ; controls, including NUL and DEL, are rejected.
    mov rcx, rsi
.validate_value:
    cmp rcx, rdi
    jae .publish
    mov al, [r8 + rcx]
    cmp al, 0x09
    je .value_next
    cmp al, 0x20
    jb .invalid
    cmp al, 0x7f
    je .invalid
.value_next:
    inc rcx
    jmp .validate_value

.publish:
    lea rax, [r8 + r10]
    mov [rbx + FIELD_NAME_PTR], rax
    mov rax, [rsp + 0]
    mov [rbx + FIELD_NAME_LEN], rax
    mov rax, [rsp + 8]
    mov [rbx + FIELD_VALUE_PTR], rax
    mov rax, [rsp + 16]
    mov [rbx + FIELD_VALUE_LEN], rax

    xor eax, eax
    mov rdx, [rsp + 24]
    jmp .return

; AL=input. CF=0 valid token byte, CF=1 invalid.
.token_char:
    cmp al, '0'
    jb .token_punct
    cmp al, '9'
    jbe .token_valid
    cmp al, 'A'
    jb .token_punct
    cmp al, 'Z'
    jbe .token_valid
    cmp al, 'a'
    jb .token_punct
    cmp al, 'z'
    jbe .token_valid
.token_punct:
    cmp al, '!'
    je .token_valid
    cmp al, '#'
    je .token_valid
    cmp al, '$'
    je .token_valid
    cmp al, '%'
    je .token_valid
    cmp al, '&'
    je .token_valid
    cmp al, 39                    ; apostrophe
    je .token_valid
    cmp al, '*'
    je .token_valid
    cmp al, '+'
    je .token_valid
    cmp al, '-'
    je .token_valid
    cmp al, '.'
    je .token_valid
    cmp al, '^'
    je .token_valid
    cmp al, '_'
    je .token_valid
    cmp al, '`'
    je .token_valid
    cmp al, '|'
    je .token_valid
    cmp al, '~'
    je .token_valid
    stc
    ret
.token_valid:
    clc
    ret

.invalid:
    mov rax, ERR_EINVAL
    xor edx, edx
    jmp .return
.overflow:
    mov rax, ERR_EOVERFLOW
    xor edx, edx
.return:
    add rsp, 32
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
