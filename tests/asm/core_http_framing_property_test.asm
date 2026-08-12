; Arborcore Retrofit E3 incremental HTTP framing qualification
%define SYS_EXIT 60
%define ERR_EAGAIN -11

extern http_frame_scan
extern http_parse_request

global _start

section .rodata
request:
    db "POST /x HTTP/1.1",13,10
    db "Host: x",13,10
    db "Content-Length: 5",13,10
    db 13,10
body:
    db "abcde"
request_len equ $ - request
headers_len equ body - request

bare_lf: db "GET / HTTP/1.1",10,10
bare_lf_len equ $ - bare_lf
bare_cr: db "GET / HTTP/1.1",13,'X',13,10,13,10
bare_cr_len equ $ - bare_cr

section .bss
alignb 16
scan: resq 1
out: resb 96

section .text
_start:
    mov qword [rel scan], 0
    xor r12d, r12d               ; previous frontier
    xor r13d, r13d               ; prefix length
.prefix_loop:
    cmp r13, headers_len
    ja .scan_complete
    lea rdi, [rel request]
    mov rsi, r13
    lea rdx, [rel scan]
    call http_frame_scan
    cmp r13, headers_len
    je .expect_found
    cmp rax, ERR_EAGAIN
    jne fail
    mov rax, [rel scan]
    cmp rax, r12
    jb fail                       ; frontier never regresses
    mov rcx, rax
    sub rcx, r12
    cmp rcx, 2
    ja fail                       ; CRLF completion may amortize two validated bytes
    mov r12, rax
    inc r13
    jmp .prefix_loop
.expect_found:
    cmp rax, 1
    jne fail
    cmp rdx, headers_len
    jne fail
.scan_complete:

    ; Complete headers + incomplete body returns exact required frame in RDX
    ; while request_out remains zeroed/fail-closed.
    lea rdi, [rel request]
    mov rsi, headers_len
    lea rdx, [rel out]
    call http_parse_request
    cmp rax, ERR_EAGAIN
    jne fail
    cmp rdx, request_len
    jne fail
    lea r8, [rel out]
    xor ecx, ecx
.zero_loop:
    cmp qword [r8 + rcx*8], 0
    jne fail
    inc ecx
    cmp ecx, 12
    jb .zero_loop

    lea rdi, [rel request]
    mov esi, request_len
    lea rdx, [rel out]
    call http_parse_request
    test rax, rax
    jnz fail
    cmp qword [rel out + 88], request_len
    jne fail

    ; Incremental framing remains strict: malformed line endings are rejected
    ; before buffer exhaustion rather than degrading into ENOSPC.
    mov qword [rel scan], 0
    lea rdi, [rel bare_lf]
    mov esi, bare_lf_len
    lea rdx, [rel scan]
    call http_frame_scan
    cmp rax, -22
    jne fail

    mov qword [rel scan], 0
    lea rdi, [rel bare_cr]
    mov esi, bare_cr_len
    lea rdx, [rel scan]
    call http_frame_scan
    cmp rax, -22
    jne fail

    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
