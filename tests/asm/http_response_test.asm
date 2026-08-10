; Arborcore HTTP response serializer test; exit 0 pass, 1 fail
%define SYS_EXIT 60

global _start
extern buffer_init
extern http_response_serialize

section .rodata
body_hello: db "Hello"
body_hello_len equ $ - body_hello
expected_200:
    db "HTTP/1.1 200 OK",13,10
    db "Content-Length: 5",13,10
    db "Connection: keep-alive",13,10
    db 13,10
    db "Hello"
expected_200_len equ $ - expected_200
expected_404:
    db "HTTP/1.1 404 Not Found",13,10
    db "Content-Length: 0",13,10
    db "Connection: close",13,10
    db 13,10
expected_404_len equ $ - expected_404

section .bss
align 16
buf: resb 24
storage: resb 256
small_storage: resb 8

section .text
_start:
    lea rdi, [rel buf]
    lea rsi, [rel storage]
    mov edx, 256
    call buffer_init
    test rax, rax
    jnz fail

    mov byte [rel storage + expected_200_len], 0xA5
    lea rdi, [rel buf]
    mov esi, 200
    lea rdx, [rel body_hello]
    mov ecx, body_hello_len
    mov r8d, 1
    call http_response_serialize
    test rax, rax
    jnz fail
    cmp rdx, expected_200_len
    jne fail
    cmp byte [rel storage + expected_200_len], 0xA5
    jne fail
    lea rdi, [rel storage]
    lea rsi, [rel expected_200]
    mov edx, expected_200_len
    call bytes_match
    cmp eax, 1
    jne fail

    ; Reinitialize and emit 404 close response.
    lea rdi, [rel buf]
    lea rsi, [rel storage]
    mov edx, 256
    call buffer_init
    test rax, rax
    jnz fail
    lea rdi, [rel buf]
    mov esi, 404
    xor edx, edx
    xor ecx, ecx
    xor r8d, r8d
    call http_response_serialize
    test rax, rax
    jnz fail
    cmp rdx, expected_404_len
    jne fail
    lea rdi, [rel storage]
    lea rsi, [rel expected_404]
    mov edx, expected_404_len
    call bytes_match
    cmp eax, 1
    jne fail

    ; ENOSPC must leave logical length unchanged.
    lea rdi, [rel buf]
    lea rsi, [rel small_storage]
    mov edx, 8
    call buffer_init
    test rax, rax
    jnz fail
    lea rdi, [rel buf]
    mov esi, 500
    xor edx, edx
    xor ecx, ecx
    xor r8d, r8d
    call http_response_serialize
    cmp rax, -28
    jne fail
    cmp qword [rel buf + 8], 0
    jne fail

    ; 204 cannot carry a body in this foundation serializer.
    lea rdi, [rel buf]
    mov esi, 204
    lea rdx, [rel body_hello]
    mov ecx, body_hello_len
    xor r8d, r8d
    call http_response_serialize
    cmp rax, -22
    jne fail

    xor edi, edi
    jmp exit

bytes_match:
    test rdx, rdx
    jz .yes
.loop:
    mov al, [rdi]
    cmp al, [rsi]
    jne .no
    inc rdi
    inc rsi
    dec rdx
    jnz .loop
.yes:
    mov eax, 1
    ret
.no:
    xor eax, eax
    ret
fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
