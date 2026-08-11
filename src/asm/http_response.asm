; Arborcore bounded HTTP/1.1 response serializer
;
; http_response_serialize(buffer, status, body, body_length, keep_alive)
;   RDI = buffer*
;   RSI = numeric HTTP status
;   RDX = body pointer (may be NULL only when body_length=0)
;   RCX = body length
;   R8  = 0 => Connection: close, 1 => Connection: keep-alive
;
; Return:
;   RAX = 0 success
;   RAX = -EINVAL / -ENOSPC / -EOVERFLOW
;   RDX = bytes appended on success, 0 on error
;
; Supported status lines in this foundation serializer:
;   200 OK, 201 Created, 204 No Content, 400 Bad Request,
;   404 Not Found, 500 Internal Server Error.
;
; The routine preflights the complete destination requirement before
; appending. On any unexpected append failure it restores the original
; logical buffer length, so partial bytes are not exposed to callers.

%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75

%define BUFFER_DATA      0
%define BUFFER_LENGTH    8
%define BUFFER_CAPACITY 16

extern buffer_append
extern buffer_append_prechecked_disjoint
extern u64_decimal_length
extern u64_format_decimal

global http_response_serialize:function

section .rodata
status_200: db "HTTP/1.1 200 OK",13,10
status_200_len equ $ - status_200
status_201: db "HTTP/1.1 201 Created",13,10
status_201_len equ $ - status_201
status_204: db "HTTP/1.1 204 No Content",13,10
status_204_len equ $ - status_204
status_400: db "HTTP/1.1 400 Bad Request",13,10
status_400_len equ $ - status_400
status_404: db "HTTP/1.1 404 Not Found",13,10
status_404_len equ $ - status_404
status_500: db "HTTP/1.1 500 Internal Server Error",13,10
status_500_len equ $ - status_500

content_length_prefix: db "Content-Length: "
content_length_prefix_len equ $ - content_length_prefix
crlf: db 13,10
crlf_len equ $ - crlf
connection_keep_alive: db "Connection: keep-alive",13,10
connection_keep_alive_len equ $ - connection_keep_alive
connection_close: db "Connection: close",13,10
connection_close_len equ $ - connection_close

section .text

http_response_serialize:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    ; locals:
    ; +0 status ptr, +8 status len, +16 decimal len, +24 required
    ; +32..+51 decimal scratch

    mov r12, rdi                 ; buffer
    mov r13, rdx                 ; body
    mov r14, rcx                 ; body length
    mov r15, r8                  ; keep-alive

    test r12, r12
    jz .invalid
    cmp r15, 1
    ja .invalid
    test r14, r14
    jz .body_ok
    test r13, r13
    jz .invalid
    mov rax, r13
    add rax, r14
    jc .overflow
.body_ok:

    ; Select canonical status line.
    cmp rsi, 200
    je .status200
    cmp rsi, 201
    je .status201
    cmp rsi, 204
    je .status204
    cmp rsi, 400
    je .status400
    cmp rsi, 404
    je .status404
    cmp rsi, 500
    je .status500
    jmp .invalid
.status200:
    lea rax, [rel status_200]
    mov qword [rsp + 8], status_200_len
    jmp .status_ready
.status201:
    lea rax, [rel status_201]
    mov qword [rsp + 8], status_201_len
    jmp .status_ready
.status204:
    test r14, r14
    jnz .invalid
    lea rax, [rel status_204]
    mov qword [rsp + 8], status_204_len
    jmp .status_ready
.status400:
    lea rax, [rel status_400]
    mov qword [rsp + 8], status_400_len
    jmp .status_ready
.status404:
    lea rax, [rel status_404]
    mov qword [rsp + 8], status_404_len
    jmp .status_ready
.status500:
    lea rax, [rel status_500]
    mov qword [rsp + 8], status_500_len
.status_ready:
    mov [rsp + 0], rax

    ; Validate buffer invariants and remember original logical length.
    mov rbx, [r12 + BUFFER_LENGTH]
    mov r9, [r12 + BUFFER_CAPACITY]
    mov [rsp + 56], r9           ; capacity survives helper calls
    cmp rbx, r9
    ja .invalid
    test r9, r9
    jz .buffer_range_ok
    mov r10, [r12 + BUFFER_DATA]
    test r10, r10
    jz .invalid
    mov rax, r10
    add rax, r9
    jc .overflow
.buffer_range_ok:

    mov rdi, r14
    call u64_decimal_length
    mov [rsp + 16], rax

    ; required = status + CL prefix + digits + CRLF + Connection + CRLF + body
    mov rax, [rsp + 8]
    add rax, content_length_prefix_len
    jc .overflow
    add rax, [rsp + 16]
    jc .overflow
    add rax, crlf_len
    jc .overflow
    test r15, r15
    jz .add_close
    add rax, connection_keep_alive_len
    jc .overflow
    jmp .add_final
.add_close:
    add rax, connection_close_len
    jc .overflow
.add_final:
    add rax, crlf_len
    jc .overflow
    add rax, r14
    jc .overflow
    mov [rsp + 24], rax

    mov rdx, rbx
    add rdx, rax
    jc .overflow
    cmp rdx, [rsp + 56]
    ja .no_space

    ; Format Content-Length into private scratch before touching the buffer.
    mov rdi, r14
    lea rsi, [rsp + 32]
    mov edx, 20
    call u64_format_decimal
    test rax, rax
    jnz .unexpected_error

    ; The complete output capacity was preflighted above.  The status/header
    ; fragments below come from this module's .rodata or private stack scratch,
    ; so they are provably disjoint from the destination buffer.  Use the
    ; prechecked disjoint buffer path for those fragments; the caller-supplied
    ; body remains on generic snapshot-safe buffer_append because it may alias.
    ;
    ; Append status line.
    mov rdi, r12
    mov rsi, [rsp + 0]
    mov rdx, [rsp + 8]
    call buffer_append_prechecked_disjoint
    test rax, rax
    jnz .rollback

    mov rdi, r12
    lea rsi, [rel content_length_prefix]
    mov edx, content_length_prefix_len
    call buffer_append_prechecked_disjoint
    test rax, rax
    jnz .rollback

    mov rdi, r12
    lea rsi, [rsp + 32]
    mov rdx, [rsp + 16]
    call buffer_append_prechecked_disjoint
    test rax, rax
    jnz .rollback

    mov rdi, r12
    lea rsi, [rel crlf]
    mov edx, crlf_len
    call buffer_append_prechecked_disjoint
    test rax, rax
    jnz .rollback

    mov rdi, r12
    test r15, r15
    jz .append_close
    lea rsi, [rel connection_keep_alive]
    mov edx, connection_keep_alive_len
    jmp .append_connection
.append_close:
    lea rsi, [rel connection_close]
    mov edx, connection_close_len
.append_connection:
    call buffer_append_prechecked_disjoint
    test rax, rax
    jnz .rollback

    mov rdi, r12
    lea rsi, [rel crlf]
    mov edx, crlf_len
    call buffer_append_prechecked_disjoint
    test rax, rax
    jnz .rollback

    test r14, r14
    jz .success
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    call buffer_append
    test rax, rax
    jnz .rollback

.success:
    mov rdx, [rsp + 24]
    xor eax, eax
    jmp .return

.rollback:
    mov [r12 + BUFFER_LENGTH], rbx
    xor edx, edx
    jmp .return

.unexpected_error:
    xor edx, edx
    jmp .return
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    jmp .return
.no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    jmp .return
.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
.return:
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
