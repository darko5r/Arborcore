; Arborcore parameter route-pattern test; exit 0 pass, 1 fail
%define SYS_EXIT 60
%define REQ_SIZE 96

global _start
extern route_pattern_match
extern route_pattern_dispatch

section .rodata
pattern1: db "/users/:id"
pattern1_len equ $ - pattern1
path1: db "/users/42"
path1_len equ $ - path1
pattern2: db "/posts/:post/comments/:comment"
pattern2_len equ $ - pattern2
path2: db "/posts/7/comments/9"
path2_len equ $ - path2
path1_slash: db "/users/42/"
path1_slash_len equ $ - path1_slash
method_get: db "GET"
target_query: db "/users/42?full=1"
target_query_len equ $ - target_query

section .data
align 8
routes:
    dq method_get, 3, pattern1, pattern1_len, pattern_handler
route_count equ 1
context_magic: dq 0x123456789abcdef0

section .bss
align 16
params: resb 128
request: resb REQ_SIZE

section .text
_start:
    lea rdi, [rel pattern1]
    mov esi, pattern1_len
    lea rdx, [rel path1]
    mov ecx, path1_len
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_match
    cmp rax, 1
    jne fail
    cmp rdx, 1
    jne fail
    cmp qword [rel params + 8], 2
    jne fail
    cmp qword [rel params + 24], 2
    jne fail
    cmp byte [rel path1 + 7], '4'
    jne fail

    lea rdi, [rel pattern2]
    mov esi, pattern2_len
    lea rdx, [rel path2]
    mov ecx, path2_len
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_match
    cmp rax, 1
    jne fail
    cmp rdx, 2
    jne fail

    ; Trailing slash is significant and must not match the no-slash pattern.
    lea rdi, [rel pattern1]
    mov esi, pattern1_len
    lea rdx, [rel path1_slash]
    mov ecx, path1_slash_len
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_match
    test rax, rax
    jnz fail

    ; Insufficient parameter capacity.
    lea rdi, [rel pattern2]
    mov esi, pattern2_len
    lea rdx, [rel path2]
    mov ecx, path2_len
    lea r8, [rel params]
    mov r9d, 1
    call route_pattern_match
    cmp rax, -28
    jne fail

    ; Parameter-record span multiplication must reject capacities
    ; larger than UINT64_MAX / 32 before any output access.
    lea rdi, [rel pattern1]
    mov esi, pattern1_len
    lea rdx, [rel path1]
    mov ecx, path1_len
    lea r8, [rel params]
    mov r9, 0x0800000000000000
    call route_pattern_match
    cmp rax, -75
    jne fail
    test rdx, rdx
    jnz fail

    ; Dispatch splits query away and passes parameter records to handler.
    lea rax, [rel method_get]
    mov [rel request + 0], rax
    mov qword [rel request + 8], 3
    lea rax, [rel target_query]
    mov [rel request + 16], rax
    mov qword [rel request + 24], target_query_len

    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel request]
    lea rcx, [rel context_magic]
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_dispatch
    cmp rax, 707
    jne fail

    xor edi, edi
    jmp exit

pattern_handler:
    mov rax, 0x123456789abcdef0
    cmp [rsi], rax
    jne .bad
    cmp rcx, 1
    jne .bad
    cmp qword [rdx + 8], 2        ; name "id"
    jne .bad
    cmp qword [rdx + 24], 2       ; value "42"
    jne .bad
    mov rax, 707
    ret
.bad:
    mov rax, -99
    ret
fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
