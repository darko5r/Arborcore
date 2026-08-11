; Arborcore Retrofit D3 HTTP response contract qualification
; exit 0 pass, 1 fail
;
; Covers exact capacity, body/output aliasing with whole-operation snapshot
; semantics, and backing-storage preservation on every expected failure path.

%define SYS_EXIT       60
%define ERR_EINVAL    -22
%define ERR_ENOSPC    -28
%define ERR_EOVERFLOW -75

extern buffer_init
extern http_response_serialize

global _start

section .rodata
body_hello: db "HELLO"
body_hello_len equ $ - body_hello

expected_200_close:
    db "HTTP/1.1 200 OK",13,10
    db "Content-Length: 5",13,10
    db "Connection: close",13,10
    db 13,10
    db "HELLO"
expected_200_close_len equ $ - expected_200_close
expected_200_close_metadata_len equ expected_200_close_len - body_hello_len

section .bss
alignb 16
buf: resb 24
storage: resb 256
failure_storage: resb 128

section .text
_start:
    ; ------------------------------------------------------------
    ; Exact-capacity success: no hidden spare byte is required.
    ; ------------------------------------------------------------
    lea rdi, [rel buf]
    lea rsi, [rel storage]
    mov edx, expected_200_close_len
    call buffer_init
    test rax, rax
    jnz fail

    mov byte [rel storage + expected_200_close_len], 0xA5
    lea rdi, [rel buf]
    mov esi, 200
    lea rdx, [rel body_hello]
    mov ecx, body_hello_len
    xor r8d, r8d
    call http_response_serialize
    test rax, rax
    jnz fail
    cmp rdx, expected_200_close_len
    jne fail
    cmp qword [rel buf + 8], expected_200_close_len
    jne fail
    cmp byte [rel storage + expected_200_close_len], 0xA5
    jne fail
    lea rdi, [rel storage]
    lea rsi, [rel expected_200_close]
    mov edx, expected_200_close_len
    call bytes_match
    cmp eax, 1
    jne fail

    ; ------------------------------------------------------------
    ; Body source overlaps metadata destination. The serializer must
    ; snapshot entry-time body bytes before writing status/header data.
    ; ------------------------------------------------------------
    lea rdi, [rel storage]
    mov esi, 256
    xor edx, edx
    call fill_bytes

    mov dword [rel storage + 8], 0x4c4c4548  ; "HELL"
    mov byte  [rel storage + 12], 'O'

    lea rdi, [rel buf]
    lea rsi, [rel storage]
    mov edx, 256
    call buffer_init
    test rax, rax
    jnz fail

    lea rdi, [rel buf]
    mov esi, 200
    lea rdx, [rel storage + 8]
    mov ecx, body_hello_len
    xor r8d, r8d
    call http_response_serialize
    test rax, rax
    jnz fail
    cmp rdx, expected_200_close_len
    jne fail
    lea rdi, [rel storage]
    lea rsi, [rel expected_200_close]
    mov edx, expected_200_close_len
    call bytes_match
    cmp eax, 1
    jne fail

    ; ------------------------------------------------------------
    ; Body already resides at its final append destination. Generic
    ; snapshot-safe buffer_append must treat source==destination correctly.
    ; ------------------------------------------------------------
    lea rdi, [rel storage]
    mov esi, 256
    xor edx, edx
    call fill_bytes

    mov dword [rel storage + expected_200_close_metadata_len], 0x4c4c4548
    mov byte  [rel storage + expected_200_close_metadata_len + 4], 'O'

    lea rdi, [rel buf]
    lea rsi, [rel storage]
    mov edx, 256
    call buffer_init
    test rax, rax
    jnz fail

    lea rdi, [rel buf]
    mov esi, 200
    lea rdx, [rel storage + expected_200_close_metadata_len]
    mov ecx, body_hello_len
    xor r8d, r8d
    call http_response_serialize
    test rax, rax
    jnz fail
    lea rdi, [rel storage]
    lea rsi, [rel expected_200_close]
    mov edx, expected_200_close_len
    call bytes_match
    cmp eax, 1
    jne fail

    ; ------------------------------------------------------------
    ; ENOSPC: complete backing storage and logical length unchanged.
    ; ------------------------------------------------------------
    call fill_failure_storage
    lea rdi, [rel buf]
    lea rsi, [rel failure_storage]
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
    cmp rax, ERR_ENOSPC
    jne fail
    test rdx, rdx
    jnz fail
    cmp qword [rel buf + 8], 0
    jne fail
    call failure_storage_unchanged
    test eax, eax
    jz fail

    ; Invalid status: no storage mutation.
    call fill_failure_storage
    lea rdi, [rel buf]
    lea rsi, [rel failure_storage]
    mov edx, 128
    call buffer_init
    test rax, rax
    jnz fail

    lea rdi, [rel buf]
    mov esi, 418
    xor edx, edx
    xor ecx, ecx
    xor r8d, r8d
    call http_response_serialize
    cmp rax, ERR_EINVAL
    jne fail
    call failure_storage_unchanged
    test eax, eax
    jz fail

    ; 204 with a body is invalid before touching storage.
    call fill_failure_storage
    lea rdi, [rel buf]
    lea rsi, [rel failure_storage]
    mov edx, 128
    call buffer_init
    test rax, rax
    jnz fail

    lea rdi, [rel buf]
    mov esi, 204
    lea rdx, [rel body_hello]
    mov ecx, body_hello_len
    xor r8d, r8d
    call http_response_serialize
    cmp rax, ERR_EINVAL
    jne fail
    call failure_storage_unchanged
    test eax, eax
    jz fail

    ; Body span wrap is rejected before any dereference or output mutation.
    call fill_failure_storage
    lea rdi, [rel buf]
    lea rsi, [rel failure_storage]
    mov edx, 128
    call buffer_init
    test rax, rax
    jnz fail

    lea rdi, [rel buf]
    mov esi, 200
    mov rdx, -8
    mov ecx, 16
    xor r8d, r8d
    call http_response_serialize
    cmp rax, ERR_EOVERFLOW
    jne fail
    call failure_storage_unchanged
    test eax, eax
    jz fail

    xor edi, edi
    jmp exit

fill_failure_storage:
    lea rdi, [rel failure_storage]
    mov esi, 128
    mov dl, 0xA5
    jmp fill_bytes

failure_storage_unchanged:
    lea rdi, [rel failure_storage]
    mov ecx, 128
.check:
    cmp byte [rdi], 0xA5
    jne .bad
    inc rdi
    dec ecx
    jnz .check
    mov eax, 1
    ret
.bad:
    xor eax, eax
    ret

; fill_bytes(RDI=ptr, ESI=len, DL=value)
fill_bytes:
    test esi, esi
    jz .done
.loop:
    mov [rdi], dl
    inc rdi
    dec esi
    jnz .loop
.done:
    ret

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
