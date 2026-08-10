; Arborcore HTTP request-target decomposition foundation
;
; Output layout (32 bytes):
;   +0  path pointer
;   +8  path length
;   +16 query pointer
;   +24 query length
;
; request_target_split(target, length, out)
; request_target_from_request(request, out)
;
; Scope:
;   - origin-form targets beginning with '/' are supported.
;   - '*' is supported as the HTTP asterisk-form target.
;   - absolute-form and authority-form are deliberately not accepted yet.
;   - the first '?' separates path and query without copying.
;   - raw '#' fragments and non-visible ASCII are rejected.
;   - percent decoding is intentionally deferred to the consumer.

%define ERR_EINVAL    -22
%define ERR_EOVERFLOW -75

%define TARGET_PATH_PTR   0
%define TARGET_PATH_LEN   8
%define TARGET_QUERY_PTR 16
%define TARGET_QUERY_LEN 24

%define REQ_TARGET_PTR   16
%define REQ_TARGET_LEN   24

global request_target_split:function
global request_target_from_request:function

section .text

request_target_from_request:
    test rdi, rdi
    jz .invalid
    mov rax, rsi
    mov rsi, [rdi + REQ_TARGET_LEN]
    mov rdi, [rdi + REQ_TARGET_PTR]
    mov rdx, rax
    jmp request_target_split
.invalid:
    mov rax, ERR_EINVAL
    xor edx, edx
    ret

; RDI=target RSI=len RDX=out
request_target_split:
    test rdx, rdx
    jz .invalid_no_output
    mov r8, rdx
    xor eax, eax
    mov [r8 + TARGET_PATH_PTR], rax
    mov [r8 + TARGET_PATH_LEN], rax
    mov [r8 + TARGET_QUERY_PTR], rax
    mov [r8 + TARGET_QUERY_LEN], rax

    test rsi, rsi
    jz .invalid
    test rdi, rdi
    jz .invalid
    mov rax, rdi
    add rax, rsi
    jc .overflow

    cmp rsi, 1
    jne .origin
    cmp byte [rdi], '*'
    jne .origin
    mov [r8 + TARGET_PATH_PTR], rdi
    mov qword [r8 + TARGET_PATH_LEN], 1
    xor eax, eax
    xor edx, edx
    ret

.origin:
    cmp byte [rdi], '/'
    jne .invalid
    xor ecx, ecx
.scan:
    cmp rcx, rsi
    jae .no_query
    mov al, [rdi + rcx]
    cmp al, 0x21
    jb .invalid
    cmp al, 0x7e
    ja .invalid
    cmp al, '#'
    je .invalid
    cmp al, '?'
    je .query
    inc rcx
    jmp .scan

.query:
    mov [r8 + TARGET_PATH_PTR], rdi
    mov [r8 + TARGET_PATH_LEN], rcx
    lea rax, [rdi + rcx + 1]
    mov [r8 + TARGET_QUERY_PTR], rax
    mov rdx, rsi
    sub rdx, rcx
    dec rdx
    mov [r8 + TARGET_QUERY_LEN], rdx
    xor eax, eax
    xor edx, edx
    ret

.no_query:
    mov [r8 + TARGET_PATH_PTR], rdi
    mov [r8 + TARGET_PATH_LEN], rsi
    xor eax, eax
    xor edx, edx
    ret

.invalid:
    xor eax, eax
    mov [r8 + TARGET_PATH_PTR], rax
    mov [r8 + TARGET_PATH_LEN], rax
    mov [r8 + TARGET_QUERY_PTR], rax
    mov [r8 + TARGET_QUERY_LEN], rax
.invalid_no_output:
    mov rax, ERR_EINVAL
    xor edx, edx
    ret
.overflow:
    xor eax, eax
    mov [r8 + TARGET_PATH_PTR], rax
    mov [r8 + TARGET_PATH_LEN], rax
    mov [r8 + TARGET_QUERY_PTR], rax
    mov [r8 + TARGET_QUERY_LEN], rax
    mov rax, ERR_EOVERFLOW
    xor edx, edx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
