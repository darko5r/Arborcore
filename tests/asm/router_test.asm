; Arborcore HTTP parser + router Polish Gate #2 tests
; exit 0 pass, 1 fail

%define SYS_EXIT   60
%define ERR_ENOENT -2

%define ROUTE_COUNT_TOO_LARGE 0x0666666666666667

extern http_parse_request
extern router_find_exact
extern router_dispatch

global _start

section .rodata
method_get:     db "GET"
method_post:    db "POST"
path_users:     db "/users"
path_submit:    db "/submit"
path_final:     db "/final"
path_missing:   db "/missing"

get_users_request:
    db "GET /users HTTP/1.1",13,10,13,10
get_users_request_len equ $ - get_users_request

post_users_request:
    db "POST /users HTTP/1.1",13,10
    db "Content-Length: 0",13,10,13,10
post_users_request_len equ $ - post_users_request

post_submit_request:
    db "POST /submit HTTP/1.1",13,10
    db "Content-Length: 0",13,10,13,10
post_submit_request_len equ $ - post_submit_request

get_final_request:
    db "GET /final HTTP/1.1",13,10,13,10
get_final_request_len equ $ - get_final_request

missing_request:
    db "GET /missing HTTP/1.1",13,10,13,10
missing_request_len equ $ - missing_request

section .data
align 8
routes:
    dq method_get,  3, path_users,  6, handler_users
    dq method_post, 4, path_users,  6, handler_post_users
    dq method_post, 4, path_submit, 7, handler_submit
    dq method_get,  3, path_final,   6, handler_final
route_count equ 4

bad_pointer_route:
    dq 0, 3, path_users, 6, handler_users

zero_handler_route:
    dq method_get, 3, path_users, 6, 0

context_magic:
    dq 0x1122334455667788

section .bss
align 16
request_out: resb 96

section .text
_start:
    ; First route.
    lea rdi, [rel get_users_request]
    mov esi, get_users_request_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz router_test_fail

    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel request_out]
    lea rcx, [rel context_magic]
    call router_dispatch
    cmp rax, 101
    jne router_test_fail

    ; Same path, different method selects a different route.
    lea rdi, [rel post_users_request]
    mov esi, post_users_request_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz router_test_fail

    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel request_out]
    lea rcx, [rel context_magic]
    call router_dispatch
    cmp rax, 303
    jne router_test_fail

    ; Middle route.
    lea rdi, [rel post_submit_request]
    mov esi, post_submit_request_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz router_test_fail

    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel request_out]
    lea rcx, [rel context_magic]
    call router_dispatch
    cmp rax, 202
    jne router_test_fail

    ; Final table entry.
    lea rdi, [rel get_final_request]
    mov esi, get_final_request_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz router_test_fail

    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel request_out]
    lea rcx, [rel context_magic]
    call router_dispatch
    cmp rax, 404
    jne router_test_fail

    ; Same method but different path does not match.
    lea rdi, [rel missing_request]
    mov esi, missing_request_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz router_test_fail

    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel request_out]
    lea rcx, [rel context_magic]
    call router_dispatch
    cmp rax, ERR_ENOENT
    jne router_test_fail

    ; Zero routes.
    lea rdi, [rel routes]
    xor esi, esi
    lea rdx, [rel request_out]
    lea rcx, [rel context_magic]
    call router_dispatch
    cmp rax, ERR_ENOENT
    jne router_test_fail

    ; NULL request.
    lea rdi, [rel routes]
    mov esi, route_count
    xor edx, edx
    lea rcx, [rel context_magic]
    call router_dispatch
    cmp rax, ERR_ENOENT
    jne router_test_fail

    ; Direct lookup: zero count and NULL table are safe.
    lea rdi, [rel routes]
    xor esi, esi
    lea rdx, [rel method_get]
    mov ecx, 3
    lea r8, [rel path_users]
    mov r9d, 6
    call router_find_exact
    test rax, rax
    jnz router_test_fail

    xor edi, edi
    mov esi, 1
    lea rdx, [rel method_get]
    mov ecx, 3
    lea r8, [rel path_users]
    mov r9d, 6
    call router_find_exact
    test rax, rax
    jnz router_test_fail

    ; Route count multiplication overflow is rejected before table reads.
    lea rdi, [rel routes]
    mov rsi, ROUTE_COUNT_TOO_LARGE
    lea rdx, [rel method_get]
    mov ecx, 3
    lea r8, [rel path_users]
    mov r9d, 6
    call router_find_exact
    test rax, rax
    jnz router_test_fail

    ; Route table end-address wrap is rejected before dereference.
    mov rdi, -16
    mov esi, 1
    lea rdx, [rel method_get]
    mov ecx, 3
    lea r8, [rel path_users]
    mov r9d, 6
    call router_find_exact
    test rax, rax
    jnz router_test_fail

    ; Nonzero NULL request spans are rejected without dereference.
    lea rdi, [rel routes]
    mov esi, route_count
    xor edx, edx
    mov ecx, 3
    lea r8, [rel path_users]
    mov r9d, 6
    call router_find_exact
    test rax, rax
    jnz router_test_fail

    lea rdi, [rel routes]
    mov esi, route_count
    lea rdx, [rel method_get]
    mov ecx, 3
    xor r8d, r8d
    mov r9d, 6
    call router_find_exact
    test rax, rax
    jnz router_test_fail

    ; Invalid route spans and NULL handler entries are skipped.
    lea rdi, [rel bad_pointer_route]
    mov esi, 1
    lea rdx, [rel method_get]
    mov ecx, 3
    lea r8, [rel path_users]
    mov r9d, 6
    call router_find_exact
    test rax, rax
    jnz router_test_fail

    lea rdi, [rel zero_handler_route]
    mov esi, 1
    lea rdx, [rel method_get]
    mov ecx, 3
    lea r8, [rel path_users]
    mov r9d, 6
    call router_find_exact
    test rax, rax
    jnz router_test_fail

    xor edi, edi
    jmp router_test_exit


handler_users:
    lea rax, [rel request_out]
    cmp rdi, rax
    jne router_handler_fail
    mov rax, 0x1122334455667788
    cmp [rsi], rax
    jne router_handler_fail
    mov eax, 101
    ret

handler_post_users:
    lea rax, [rel request_out]
    cmp rdi, rax
    jne router_handler_fail
    mov rax, 0x1122334455667788
    cmp [rsi], rax
    jne router_handler_fail
    mov eax, 303
    ret

handler_submit:
    lea rax, [rel request_out]
    cmp rdi, rax
    jne router_handler_fail
    mov rax, 0x1122334455667788
    cmp [rsi], rax
    jne router_handler_fail
    mov eax, 202
    ret

handler_final:
    lea rax, [rel request_out]
    cmp rdi, rax
    jne router_handler_fail
    mov rax, 0x1122334455667788
    cmp [rsi], rax
    jne router_handler_fail
    mov eax, 404
    ret

router_handler_fail:
    mov rax, -99
    ret

router_test_fail:
    mov edi, 1

router_test_exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
