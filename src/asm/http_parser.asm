; Arborcore strict bounded HTTP/1.1 request parser foundation
;
; http_parse_request(buffer, length, request_out)
;
; Input:
;   RDI = bounded request bytes
;   RSI = available byte count
;   RDX = pointer to a 96-byte output structure
;
; Return:
;   RAX = 0      complete request available
;   RAX = -11    EAGAIN / incomplete request
;   RAX = -22    EINVAL / malformed or unsupported framing
;   RAX = -75    EOVERFLOW
;
; Output layout (96 bytes):
;   +0   method pointer
;   +8   method length
;   +16  request-target pointer
;   +24  request-target length
;   +32  version pointer
;   +40  version length
;   +48  headers pointer
;   +56  headers byte length (excludes final empty-line CRLF)
;   +64  body pointer
;   +72  bytes available after header terminator
;   +80  Content-Length (0 when absent)
;   +88  complete request length to consume
;
; Scope:
;   - HTTP/1.1 request line only.
;   - Method and field-name bytes use strict HTTP token grammar.
;   - Request-target uses visible ASCII in this foundation layer.
;   - Field values reject controls/DEL while allowing HTAB and obs-text.
;   - Bare LF and interior bare CR line endings are rejected.
;   - Content-Length framing is supported.
;   - Duplicate Content-Length is rejected.
;   - Transfer-Encoding is rejected in this foundation parser so
;     ambiguous/chunked framing cannot be misinterpreted.
;   - Request-target is preserved as raw bytes; URL decoding belongs
;     to a later request-target layer.

%define ERR_EAGAIN     -11
%define ERR_EINVAL     -22
%define ERR_EOVERFLOW  -75

%define REQ_METHOD_PTR       0
%define REQ_METHOD_LEN       8
%define REQ_TARGET_PTR      16
%define REQ_TARGET_LEN      24
%define REQ_VERSION_PTR     32
%define REQ_VERSION_LEN     40
%define REQ_HEADERS_PTR     48
%define REQ_HEADERS_LEN     56
%define REQ_BODY_PTR        64
%define REQ_BODY_AVAILABLE  72
%define REQ_CONTENT_LENGTH  80
%define REQ_MESSAGE_LENGTH  88

extern bytes_find_crlf
extern bytes_equal_ascii_ci
extern bytes_parse_u64_decimal

global http_parse_request:function

section .rodata
content_length_name:
    db "Content-Length"
content_length_name_len equ $ - content_length_name

transfer_encoding_name:
    db "Transfer-Encoding"
transfer_encoding_name_len equ $ - transfer_encoding_name

section .text

http_parse_request:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 48

    ; Locals:
    ; [rsp+0]  end pointer
    ; [rsp+8]  headers start
    ; [rsp+16] current header line end
    ; [rsp+24] Content-Length seen flag
    ; [rsp+32] trimmed value pointer
    ; [rsp+40] trimmed value length

    mov r12, rdi                   ; buffer
    mov r13, rsi                   ; length (later reusable)
    mov r14, rdx                   ; output

    test r14, r14
    jz .invalid_no_output

    ; Deterministic zeroed output on all failures.
    xor eax, eax
    xor ecx, ecx
.clear_out:
    mov [r14 + rcx*8], rax
    inc ecx
    cmp ecx, 12
    jb .clear_out

    mov qword [rsp + 24], 0

    test r13, r13
    jz .incomplete
    test r12, r12
    jz .invalid

    mov rax, r12
    add rax, r13
    jc .overflow
    mov [rsp + 0], rax             ; end pointer

    ; Locate request-line CRLF.
    mov rdi, r12
    mov rsi, r13
    call .strict_crlf
    cmp rax, ERR_EINVAL
    je .invalid
    test rax, rax
    jz .incomplete

    mov r15, rax                   ; request-line end
    mov r8, r15
    sub r8, r12                    ; request-line length

    ; Find first SP: method boundary.
    xor ecx, ecx
.find_first_space:
    cmp rcx, r8
    jae .invalid
    mov al, [r12 + rcx]
    cmp al, ' '
    je .first_space_found
    call .token_char
    jc .invalid
    inc rcx
    jmp .find_first_space

.first_space_found:
    test rcx, rcx
    jz .invalid                    ; empty method
    mov [r14 + REQ_METHOD_PTR], r12
    mov [r14 + REQ_METHOD_LEN], rcx

    ; Find second SP: target boundary.
    lea r9, [rcx + 1]              ; target start index
    cmp r9, r8
    jae .invalid
    mov r10, r9
.find_second_space:
    cmp r10, r8
    jae .invalid
    mov al, [r12 + r10]
    cmp al, ' '
    je .second_space_found
    ; Request-target foundation grammar: visible ASCII only.
    ; Raw controls, SP, DEL and raw non-ASCII bytes are rejected.
    cmp al, 0x21
    jb .invalid
    cmp al, 0x7e
    ja .invalid
    inc r10
    jmp .find_second_space

.second_space_found:
    cmp r10, r9
    je .invalid                    ; empty target
    lea rax, [r12 + r9]
    mov [r14 + REQ_TARGET_PTR], rax
    mov rax, r10
    sub rax, r9
    mov [r14 + REQ_TARGET_LEN], rax

    ; Version must be exactly HTTP/1.1.
    lea r9, [r10 + 1]
    mov rax, r8
    sub rax, r9
    cmp rax, 8
    jne .invalid
    lea r10, [r12 + r9]
    mov rax, 0x312e312f50545448     ; "HTTP/1.1" little-endian
    cmp qword [r10], rax
    jne .invalid
    mov [r14 + REQ_VERSION_PTR], r10
    mov qword [r14 + REQ_VERSION_LEN], 8

    ; Headers begin immediately after request-line CRLF.
    lea r15, [r15 + 2]
    mov rax, [rsp + 0]
    cmp r15, rax
    ja .incomplete
    mov [rsp + 8], r15
    mov [r14 + REQ_HEADERS_PTR], r15

.header_loop:
    mov rbx, r15                   ; current line start
    mov rsi, [rsp + 0]
    cmp rbx, rsi
    ja .invalid
    sub rsi, rbx                   ; bytes remaining
    cmp rsi, 2
    jb .incomplete

    mov rdi, rbx
    call .strict_crlf
    cmp rax, ERR_EINVAL
    je .invalid
    test rax, rax
    jz .incomplete
    mov [rsp + 16], rax

    ; Empty line terminates headers.
    cmp rax, rbx
    je .headers_complete

    ; Find ':' within this header line.
    mov r8, rax
    sub r8, rbx                    ; line length
    xor r13d, r13d                 ; colon index / name length
.find_colon:
    cmp r13, r8
    jae .invalid
    mov al, [rbx + r13]
    cmp al, ':'
    je .colon_found
    ; RFC token grammar for field names.
    call .token_char
    jc .invalid
    inc r13
    jmp .find_colon

.colon_found:
    test r13, r13
    jz .invalid                    ; empty field-name

    ; HTTP OWS is exactly SP / HTAB. Trim it without applying broader
    ; locale/C-whitespace semantics.
    lea rax, [rbx + r13 + 1]
    mov rdx, [rsp + 16]
    sub rdx, rax

    ; Field-value bytes may contain HTAB, SP, visible ASCII and
    ; obs-text (0x80-0xff), but not other controls or DEL.
    mov r9, rax
    mov r10, rdx
    xor ecx, ecx
.validate_field_value:
    cmp rcx, r10
    jae .field_value_valid
    mov al, [r9 + rcx]
    cmp al, 0x09
    je .field_value_next
    cmp al, 0x20
    jb .invalid
    cmp al, 0x7f
    je .invalid
.field_value_next:
    inc rcx
    jmp .validate_field_value
.field_value_valid:
    mov rax, r9
    mov rdx, r10

.trim_ows_leading:
    test rdx, rdx
    jz .trim_ows_done
    mov cl, [rax]
    cmp cl, 0x20
    je .trim_ows_leading_consume
    cmp cl, 0x09
    jne .trim_ows_trailing
.trim_ows_leading_consume:
    inc rax
    dec rdx
    jmp .trim_ows_leading

.trim_ows_trailing:
    test rdx, rdx
    jz .trim_ows_done
    mov cl, [rax + rdx - 1]
    cmp cl, 0x20
    je .trim_ows_trailing_consume
    cmp cl, 0x09
    jne .trim_ows_done
.trim_ows_trailing_consume:
    dec rdx
    jmp .trim_ows_trailing

.trim_ows_done:
    mov [rsp + 32], rax
    mov [rsp + 40], rdx

    ; Content-Length?
    mov rdi, rbx
    mov rsi, r13
    lea rdx, [rel content_length_name]
    mov ecx, content_length_name_len
    call bytes_equal_ascii_ci
    cmp rax, 1
    je .content_length_header

    ; Transfer-Encoding is deliberately unsupported here and rejected
    ; rather than silently misframed.
    mov rdi, rbx
    mov rsi, r13
    lea rdx, [rel transfer_encoding_name]
    mov ecx, transfer_encoding_name_len
    call bytes_equal_ascii_ci
    cmp rax, 1
    je .invalid

.advance_header:
    mov r15, [rsp + 16]
    add r15, 2
    jc .overflow
    jmp .header_loop

.content_length_header:
    cmp qword [rsp + 24], 0
    jne .invalid                    ; duplicate Content-Length

    mov rdi, [rsp + 32]
    mov rsi, [rsp + 40]
    call bytes_parse_u64_decimal
    test rax, rax
    jz .content_length_ok
    cmp rax, ERR_EOVERFLOW
    je .overflow
    jmp .invalid

.content_length_ok:
    mov [r14 + REQ_CONTENT_LENGTH], rdx
    mov qword [rsp + 24], 1
    jmp .advance_header

.headers_complete:
    ; rbx points at the CR of the final empty line.
    mov rax, rbx
    sub rax, [rsp + 8]
    mov [r14 + REQ_HEADERS_LEN], rax

    lea r15, [rbx + 2]             ; body start
    mov rax, [rsp + 0]
    cmp r15, rax
    ja .incomplete
    mov [r14 + REQ_BODY_PTR], r15

    sub rax, r15                   ; all bytes available after headers
    mov [r14 + REQ_BODY_AVAILABLE], rax

    mov rcx, [r14 + REQ_CONTENT_LENGTH]
    cmp rcx, rax
    ja .incomplete

    ; Exact bytes belonging to this request, useful for consuming a
    ; connection buffer that may already contain a pipelined request.
    mov rdx, r15
    sub rdx, r12
    add rdx, rcx
    jc .overflow
    mov [r14 + REQ_MESSAGE_LENGTH], rdx

    xor eax, eax
    jmp .return

.invalid_no_output:
    mov rax, ERR_EINVAL
    jmp .return

.incomplete:
    mov r10, ERR_EAGAIN
    jmp .failure
.invalid:
    mov r10, ERR_EINVAL
    jmp .failure
.overflow:
    mov r10, ERR_EOVERFLOW

.failure:
    ; Fail closed: callers never observe a partially parsed request.
    xor eax, eax
    xor ecx, ecx
.failure_clear:
    mov [r14 + rcx*8], rax
    inc ecx
    cmp ecx, 12
    jb .failure_clear
    mov rax, r10

.return:
    add rsp, 48
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; Strict HTTP line delimiter lookup.
;
; Input:
;   RDI = line start
;   RSI = available bytes
;
; Return:
;   RAX = CR pointer when a valid CRLF is found
;   RAX = 0 on incomplete input
;   RAX = -EINVAL on bare LF or an interior bare CR
;
; A final CR byte is treated as incomplete because its LF may arrive
; in the next read.
.strict_crlf:
    ; RDI/RSI are caller-saved, and bytes_find_crlf also uses/clobbers
    ; R8/R9. Preserve the original bounded span explicitly on the stack.
    ; Entry RSP is 8 mod 16; reserving 24 bytes restores 16-byte
    ; alignment before the nested call.
    sub rsp, 24
    mov [rsp + 0], rdi            ; original line start
    mov [rsp + 8], rsi            ; original available length
    call bytes_find_crlf
    mov r8, [rsp + 0]
    mov r9, [rsp + 8]
    test rax, rax
    jz .strict_no_pair

    mov r10, rax                  ; found CR pointer
    mov rdx, r10
    sub rdx, r8                   ; bytes before the valid pair
    xor ecx, ecx

.strict_prefix_scan:
    cmp rcx, rdx
    jae .strict_found
    mov al, [r8 + rcx]
    cmp al, 0x0d
    je .strict_invalid
    cmp al, 0x0a
    je .strict_invalid
    inc rcx
    jmp .strict_prefix_scan

.strict_found:
    mov rax, r10
    add rsp, 24
    ret

.strict_no_pair:
    xor ecx, ecx

.strict_remainder_scan:
    cmp rcx, r9
    jae .strict_incomplete
    mov al, [r8 + rcx]
    cmp al, 0x0a
    je .strict_invalid
    cmp al, 0x0d
    jne .strict_remainder_next

    lea rdx, [rcx + 1]
    cmp rdx, r9
    je .strict_incomplete         ; trailing CR may become CRLF
    jmp .strict_invalid

.strict_remainder_next:
    inc rcx
    jmp .strict_remainder_scan

.strict_incomplete:
    xor eax, eax
    add rsp, 24
    ret

.strict_invalid:
    mov rax, ERR_EINVAL
    add rsp, 24
    ret

; AL = candidate HTTP token byte.
; CF clear => valid token byte, CF set => invalid.
.token_char:
    movzx eax, al

    cmp eax, '0'
    jb .token_upper
    cmp eax, '9'
    jbe .token_valid

.token_upper:
    cmp eax, 'A'
    jb .token_lower
    cmp eax, 'Z'
    jbe .token_valid

.token_lower:
    cmp eax, 'a'
    jb .token_punct
    cmp eax, 'z'
    jbe .token_valid

.token_punct:
    cmp eax, 0x21
    je .token_valid
    cmp eax, 0x23
    je .token_valid
    cmp eax, 0x24
    je .token_valid
    cmp eax, 0x25
    je .token_valid
    cmp eax, 0x26
    je .token_valid
    cmp eax, 0x27
    je .token_valid
    cmp eax, 0x2a
    je .token_valid
    cmp eax, 0x2b
    je .token_valid
    cmp eax, 0x2d
    je .token_valid
    cmp eax, 0x2e
    je .token_valid
    cmp eax, 0x5e
    je .token_valid
    cmp eax, 0x5f
    je .token_valid
    cmp eax, 0x60
    je .token_valid
    cmp eax, 0x7c
    je .token_valid
    cmp eax, 0x7e
    je .token_valid

    stc
    ret

.token_valid:
    clc
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
