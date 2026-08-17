; Arborcore HTTP0 HTTP/1.1 final-response serializer.
;
; Internal HTTP-layer symbol; not part of frozen Assembly ABI v1.
;
; http0_response_serialize_asm(buffer, args)
;   RDI = arbor_asm_buffer*
;   RSI = prevalidated http0_asm_response_args*
;
; Return arbor_asm_result_u64:
;   RAX = 0 success, -EINVAL / -ENOSPC / -EOVERFLOW failure
;   RDX = bytes appended on success, 0 on failure
;
; The routine performs a complete length/capacity preflight before appending.
; The public C layer rejects response/body/field storage that aliases the output
; backing buffer, so sequential appends cannot destroy later source bytes.

%define ERR_EINVAL    -22
%define ERR_ENOSPC    -28
%define ERR_EOVERFLOW -75

%define BUFFER_DATA      0
%define BUFFER_LENGTH    8
%define BUFFER_CAPACITY 16

%define ARG_STATUS               0
%define ARG_REASON_PTR           8
%define ARG_REASON_LEN          16
%define ARG_FIELDS_PTR          24
%define ARG_FIELD_COUNT         32
%define ARG_BODY_PTR            40
%define ARG_BODY_LEN            48
%define ARG_SEND_BODY           56
%define ARG_EMIT_CONTENT_LENGTH 64
%define ARG_CLOSE               72

%define FIELD_NAME_PTR   0
%define FIELD_NAME_LEN   8
%define FIELD_VALUE_PTR 16
%define FIELD_VALUE_LEN 24
%define FIELD_SIZE       32

extern buffer_append
extern u64_decimal_length
extern u64_format_decimal

global http0_response_serialize_asm:function hidden

section .rodata
http11_prefix: db "HTTP/1.1 "
http11_prefix_len equ $ - http11_prefix
status_space: db " "
status_space_len equ $ - status_space
colon_space: db ": "
colon_space_len equ $ - colon_space
crlf: db 13,10
crlf_len equ $ - crlf
content_length_prefix: db "Content-Length: "
content_length_prefix_len equ $ - content_length_prefix
connection_close: db "Connection: close",13,10
connection_close_len equ $ - connection_close

section .text

http0_response_serialize_asm:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    ; Local layout:
    ; +0..+19  status decimal scratch
    ; +24..+43 Content-Length decimal scratch
    ; +48      Content-Length decimal length
    ; +56      reserved

    mov r12, rdi                 ; buffer
    mov r13, rsi                 ; args

    test r12, r12
    jz .invalid
    test r13, r13
    jz .invalid

    ; Validate scalar argument domain defensively even though C prevalidates.
    mov rax, [r13 + ARG_STATUS]
    cmp rax, 200
    jb .invalid
    cmp rax, 599
    ja .invalid

    cmp qword [r13 + ARG_SEND_BODY], 1
    ja .invalid
    cmp qword [r13 + ARG_EMIT_CONTENT_LENGTH], 1
    ja .invalid
    cmp qword [r13 + ARG_CLOSE], 1
    ja .invalid

    mov rax, [r13 + ARG_REASON_LEN]
    test rax, rax
    jz .reason_pointer_ok
    cmp qword [r13 + ARG_REASON_PTR], 0
    je .invalid
.reason_pointer_ok:

    mov rax, [r13 + ARG_FIELD_COUNT]
    test rax, rax
    jz .field_pointer_ok
    cmp qword [r13 + ARG_FIELDS_PTR], 0
    je .invalid
    mov rcx, rax
    shr rcx, 59                  ; count * 32 must fit u64
    jnz .overflow
    shl rax, 5
    mov rcx, [r13 + ARG_FIELDS_PTR]
    add rcx, rax
    jc .overflow
.field_pointer_ok:

    mov rax, [r13 + ARG_BODY_LEN]
    test rax, rax
    jz .body_pointer_ok
    cmp qword [r13 + ARG_BODY_PTR], 0
    je .invalid
    mov rcx, [r13 + ARG_BODY_PTR]
    add rcx, rax
    jc .overflow
.body_pointer_ok:

    ; Validate destination buffer and remember original logical length in RBX.
    mov rbx, [r12 + BUFFER_LENGTH]
    mov rax, [r12 + BUFFER_CAPACITY]
    cmp rbx, rax
    ja .invalid
    test rax, rax
    jz .buffer_ok
    cmp qword [r12 + BUFFER_DATA], 0
    je .invalid
    mov rcx, [r12 + BUFFER_DATA]
    add rcx, rax
    jc .overflow
.buffer_ok:

    ; required = status line: "HTTP/1.1 " + 3 digits + SP + reason + CRLF
    mov r15d, http11_prefix_len + 3 + status_space_len + crlf_len
    add r15, [r13 + ARG_REASON_LEN]
    jc .overflow

    ; Add ordered application field lines.
    xor r14d, r14d
.preflight_fields:
    cmp r14, [r13 + ARG_FIELD_COUNT]
    jae .preflight_content_length
    mov rax, r14
    shl rax, 5
    add rax, [r13 + ARG_FIELDS_PTR]

    mov rcx, [rax + FIELD_NAME_LEN]
    test rcx, rcx
    jz .invalid
    cmp qword [rax + FIELD_NAME_PTR], 0
    je .invalid
    mov rdx, [rax + FIELD_NAME_PTR]
    add rdx, rcx
    jc .overflow
    add r15, rcx
    jc .overflow
    add r15, colon_space_len
    jc .overflow

    mov rcx, [rax + FIELD_VALUE_LEN]
    test rcx, rcx
    jz .field_value_pointer_ok
    cmp qword [rax + FIELD_VALUE_PTR], 0
    je .invalid
    mov rdx, [rax + FIELD_VALUE_PTR]
    add rdx, rcx
    jc .overflow
.field_value_pointer_ok:
    add r15, rcx
    jc .overflow
    add r15, crlf_len
    jc .overflow

    inc r14
    jmp .preflight_fields

.preflight_content_length:
    cmp qword [r13 + ARG_EMIT_CONTENT_LENGTH], 0
    je .preflight_connection
    mov rdi, [r13 + ARG_BODY_LEN]
    call u64_decimal_length
    mov [rsp + 48], rax
    add r15, content_length_prefix_len
    jc .overflow
    add r15, rax
    jc .overflow
    add r15, crlf_len
    jc .overflow

.preflight_connection:
    cmp qword [r13 + ARG_CLOSE], 0
    je .preflight_final_crlf
    add r15, connection_close_len
    jc .overflow

.preflight_final_crlf:
    add r15, crlf_len
    jc .overflow

    cmp qword [r13 + ARG_SEND_BODY], 0
    je .capacity_check
    add r15, [r13 + ARG_BODY_LEN]
    jc .overflow

.capacity_check:
    mov rax, rbx
    add rax, r15
    jc .overflow
    cmp rax, [r12 + BUFFER_CAPACITY]
    ja .no_space

    ; Prepare both decimal strings before touching output.
    mov rdi, [r13 + ARG_STATUS]
    lea rsi, [rsp + 0]
    mov edx, 20
    call u64_format_decimal
    test rax, rax
    jnz .unexpected
    cmp rdx, 3
    jne .unexpected

    cmp qword [r13 + ARG_EMIT_CONTENT_LENGTH], 0
    je .append_status_prefix
    mov rdi, [r13 + ARG_BODY_LEN]
    lea rsi, [rsp + 24]
    mov edx, 20
    call u64_format_decimal
    test rax, rax
    jnz .unexpected
    cmp rdx, [rsp + 48]
    jne .unexpected

.append_status_prefix:
    mov rdi, r12
    lea rsi, [rel http11_prefix]
    mov edx, http11_prefix_len
    call buffer_append
    test rax, rax
    jnz .rollback

    mov rdi, r12
    lea rsi, [rsp + 0]
    mov edx, 3
    call buffer_append
    test rax, rax
    jnz .rollback

    mov rdi, r12
    lea rsi, [rel status_space]
    mov edx, status_space_len
    call buffer_append
    test rax, rax
    jnz .rollback

    mov rdx, [r13 + ARG_REASON_LEN]
    test rdx, rdx
    jz .append_status_crlf
    mov rdi, r12
    mov rsi, [r13 + ARG_REASON_PTR]
    call buffer_append
    test rax, rax
    jnz .rollback

.append_status_crlf:
    mov rdi, r12
    lea rsi, [rel crlf]
    mov edx, crlf_len
    call buffer_append
    test rax, rax
    jnz .rollback

    ; Preserve the caller-specified field-line order and duplicates.
    xor r14d, r14d
.append_fields:
    cmp r14, [r13 + ARG_FIELD_COUNT]
    jae .append_content_length
    mov rax, r14
    shl rax, 5
    add rax, [r13 + ARG_FIELDS_PTR]
    mov [rsp + 56], rax

    mov rdi, r12
    mov rsi, [rax + FIELD_NAME_PTR]
    mov rdx, [rax + FIELD_NAME_LEN]
    call buffer_append
    test rax, rax
    jnz .rollback

    mov rdi, r12
    lea rsi, [rel colon_space]
    mov edx, colon_space_len
    call buffer_append
    test rax, rax
    jnz .rollback

    mov rax, [rsp + 56]
    mov rdx, [rax + FIELD_VALUE_LEN]
    test rdx, rdx
    jz .append_field_crlf
    mov rdi, r12
    mov rsi, [rax + FIELD_VALUE_PTR]
    call buffer_append
    test rax, rax
    jnz .rollback

.append_field_crlf:
    mov rdi, r12
    lea rsi, [rel crlf]
    mov edx, crlf_len
    call buffer_append
    test rax, rax
    jnz .rollback

    inc r14
    jmp .append_fields

.append_content_length:
    cmp qword [r13 + ARG_EMIT_CONTENT_LENGTH], 0
    je .append_connection

    mov rdi, r12
    lea rsi, [rel content_length_prefix]
    mov edx, content_length_prefix_len
    call buffer_append
    test rax, rax
    jnz .rollback

    mov rdi, r12
    lea rsi, [rsp + 24]
    mov rdx, [rsp + 48]
    call buffer_append
    test rax, rax
    jnz .rollback

    mov rdi, r12
    lea rsi, [rel crlf]
    mov edx, crlf_len
    call buffer_append
    test rax, rax
    jnz .rollback

.append_connection:
    cmp qword [r13 + ARG_CLOSE], 0
    je .append_final_crlf
    mov rdi, r12
    lea rsi, [rel connection_close]
    mov edx, connection_close_len
    call buffer_append
    test rax, rax
    jnz .rollback

.append_final_crlf:
    mov rdi, r12
    lea rsi, [rel crlf]
    mov edx, crlf_len
    call buffer_append
    test rax, rax
    jnz .rollback

    cmp qword [r13 + ARG_SEND_BODY], 0
    je .success
    mov rdx, [r13 + ARG_BODY_LEN]
    test rdx, rdx
    jz .success
    mov rdi, r12
    mov rsi, [r13 + ARG_BODY_PTR]
    call buffer_append
    test rax, rax
    jnz .rollback

.success:
    xor eax, eax
    mov rdx, r15
    jmp .return

.rollback:
    mov [r12 + BUFFER_LENGTH], rbx
    xor edx, edx
    jmp .return
.unexpected:
    mov [r12 + BUFFER_LENGTH], rbx
    mov rax, ERR_EINVAL
    xor edx, edx
    jmp .return
.invalid:
    mov rax, ERR_EINVAL
    xor edx, edx
    jmp .return
.no_space:
    mov rax, ERR_ENOSPC
    xor edx, edx
    jmp .return
.overflow:
    mov rax, ERR_EOVERFLOW
    xor edx, edx
.return:
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
