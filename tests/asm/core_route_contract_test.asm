; Arborcore Retrofit D2 route-pattern/router contract qualification
; exit 0 pass, 1 fail
;
; Qualifies capture boundaries, output validity, pointer lifetimes, parameter
; capacity, and explicit catalog-order precedence between exact and parameter
; patterns.

%define SYS_EXIT       60
%define ERR_ENOENT     -2
%define ERR_EINVAL    -22
%define ERR_ENOSPC    -28
%define ERR_EOVERFLOW -75
%define REQ_SIZE       96
%define ROUTE_COUNT_TOO_LARGE 0x0666666666666667

extern route_pattern_match
extern route_pattern_dispatch

global _start

section .rodata
method_get: db "GET"
method_get_len equ $ - method_get

exact_users_42: db "/users/42"
exact_users_42_len equ $ - exact_users_42
param_users: db "/users/:id"
param_users_len equ $ - param_users
query_target: db "/users/42?view=full"
query_target_len equ $ - query_target
bad_query_target: db "/users/42?view#frag"
bad_query_target_len equ $ - bad_query_target

one_pattern: db "/:x"
one_pattern_len equ $ - one_pattern
one_path: db "/a"
one_path_len equ $ - one_path
empty_value_path: db "/"
empty_value_path_len equ $ - empty_value_path
invalid_name_pattern: db "/:bad-name"
invalid_name_pattern_len equ $ - invalid_name_pattern

three_pattern: db "/:a/:b/:c"
three_pattern_len equ $ - three_pattern
three_path: db "/1/22/333"
three_path_len equ $ - three_path

mismatch_pattern: db "/:x/static"
mismatch_pattern_len equ $ - mismatch_pattern
mismatch_path: db "/a/miss"
mismatch_path_len equ $ - mismatch_path

; Labels make the alias/lifetime contract test exact rather than inferred.
lifetime_pattern:
    db "/users/"
lifetime_name1: db ":id"
    db "/posts/"
lifetime_name2: db ":post"
lifetime_pattern_len equ $ - lifetime_pattern

lifetime_target:
    db "/users/"
lifetime_value1: db "42"
    db "/posts/"
lifetime_value2: db "9001"
    db "?draft=0"
lifetime_target_len equ $ - lifetime_target

section .data
align 8
routes_exact_first:
    dq method_get, method_get_len, exact_users_42, exact_users_42_len, exact_handler
    dq method_get, method_get_len, param_users, param_users_len, param_handler
routes_exact_first_count equ 2

routes_param_first:
    dq method_get, method_get_len, param_users, param_users_len, param_handler
    dq method_get, method_get_len, exact_users_42, exact_users_42_len, exact_handler
routes_param_first_count equ 2

routes_duplicate_exact:
    dq method_get, method_get_len, exact_users_42, exact_users_42_len, duplicate_first
    dq method_get, method_get_len, exact_users_42, exact_users_42_len, duplicate_second
routes_duplicate_exact_count equ 2

routes_lifetime:
    dq method_get, method_get_len, lifetime_pattern, lifetime_pattern_len, lifetime_handler
routes_lifetime_count equ 1

context_magic: dq 0x7d2a9f013355aaee

section .bss
alignb 16
params: resb 128
request: resb REQ_SIZE

section .text
_start:
    ; Static exact pattern succeeds with zero parameter capacity.
    lea rdi, [rel exact_users_42]
    mov esi, exact_users_42_len
    lea rdx, [rel exact_users_42]
    mov ecx, exact_users_42_len
    xor r8d, r8d
    xor r9d, r9d
    call route_pattern_match
    cmp rax, 1
    jne fail
    test rdx, rdx
    jnz fail

    ; Smallest legal parameter: one-character name and one-character value.
    lea rdi, [rel one_pattern]
    mov esi, one_pattern_len
    lea rdx, [rel one_path]
    mov ecx, one_path_len
    lea r8, [rel params]
    mov r9d, 1
    call route_pattern_match
    cmp rax, 1
    jne fail
    cmp rdx, 1
    jne fail
    cmp qword [rel params + 8], 1
    jne fail
    cmp qword [rel params + 24], 1
    jne fail

    ; Parameter values are never empty.
    lea rdi, [rel one_pattern]
    mov esi, one_pattern_len
    lea rdx, [rel empty_value_path]
    mov ecx, empty_value_path_len
    lea r8, [rel params]
    mov r9d, 1
    call route_pattern_match
    test rax, rax
    jnz fail
    test rdx, rdx
    jnz fail

    ; Invalid parameter-name grammar is a catalog error, not a no-match.
    lea rdi, [rel invalid_name_pattern]
    mov esi, invalid_name_pattern_len
    lea rdx, [rel one_path]
    mov ecx, one_path_len
    lea r8, [rel params]
    mov r9d, 1
    call route_pattern_match
    cmp rax, ERR_EINVAL
    jne fail
    test rdx, rdx
    jnz fail

    ; Three captures fit exactly in capacity three.
    lea rdi, [rel three_pattern]
    mov esi, three_pattern_len
    lea rdx, [rel three_path]
    mov ecx, three_path_len
    lea r8, [rel params]
    mov r9d, 3
    call route_pattern_match
    cmp rax, 1
    jne fail
    cmp rdx, 3
    jne fail

    ; The same match fails closed when capture capacity is insufficient.
    lea rdi, [rel three_pattern]
    mov esi, three_pattern_len
    lea rdx, [rel three_path]
    mov ecx, three_path_len
    lea r8, [rel params]
    mov r9d, 2
    call route_pattern_match
    cmp rax, ERR_ENOSPC
    jne fail
    test rdx, rdx
    jnz fail

    ; Parameter-record span multiplication is bounded before output access.
    lea rdi, [rel one_pattern]
    mov esi, one_pattern_len
    lea rdx, [rel one_path]
    mov ecx, one_path_len
    lea r8, [rel params]
    mov r9, 0x0800000000000000
    call route_pattern_match
    cmp rax, ERR_EOVERFLOW
    jne fail
    test rdx, rdx
    jnz fail

    ; A later static mismatch returns RAX=0/RDX=0. Earlier speculative PARAM
    ; records are deliberately unspecified and must be ignored by the caller.
    lea rdi, [rel mismatch_pattern]
    mov esi, mismatch_pattern_len
    lea rdx, [rel mismatch_path]
    mov ecx, mismatch_path_len
    lea r8, [rel params]
    mov r9d, 2
    call route_pattern_match
    test rax, rax
    jnz fail
    test rdx, rdx
    jnz fail

    ; Lifetime/alias contract: names point into route catalog storage; values
    ; point directly into this request target, even when a query follows.
    call prepare_lifetime_request
    lea rdi, [rel routes_lifetime]
    mov esi, routes_lifetime_count
    lea rdx, [rel request]
    lea rcx, [rel context_magic]
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_dispatch
    cmp rax, 909
    jne fail

    ; Exact-before-parameter wins because catalog order is authoritative.
    call prepare_query_request
    lea rdi, [rel routes_exact_first]
    mov esi, routes_exact_first_count
    lea rdx, [rel request]
    lea rcx, [rel context_magic]
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_dispatch
    cmp rax, 111
    jne fail

    ; Reversing catalog order intentionally reverses precedence.
    lea rdi, [rel routes_param_first]
    mov esi, routes_param_first_count
    lea rdx, [rel request]
    lea rcx, [rel context_magic]
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_dispatch
    cmp rax, 222
    jne fail

    ; Duplicate exact patterns preserve first-match semantics.
    lea rdi, [rel routes_duplicate_exact]
    mov esi, routes_duplicate_exact_count
    lea rdx, [rel request]
    lea rcx, [rel context_magic]
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_dispatch
    cmp rax, 333
    jne fail

    ; Hardened request-target validation propagates through dispatch.
    call prepare_bad_query_request
    lea rdi, [rel routes_exact_first]
    mov esi, routes_exact_first_count
    lea rdx, [rel request]
    lea rcx, [rel context_magic]
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_dispatch
    cmp rax, ERR_EINVAL
    jne fail

    ; Route-table multiplication overflow fails before table traversal.
    call prepare_query_request
    lea rdi, [rel routes_exact_first]
    mov rsi, ROUTE_COUNT_TOO_LARGE
    lea rdx, [rel request]
    lea rcx, [rel context_magic]
    lea r8, [rel params]
    mov r9d, 4
    call route_pattern_dispatch
    cmp rax, ERR_EOVERFLOW
    jne fail

    xor edi, edi
    jmp exit

prepare_query_request:
    lea rax, [rel method_get]
    mov [rel request + 0], rax
    mov qword [rel request + 8], method_get_len
    lea rax, [rel query_target]
    mov [rel request + 16], rax
    mov qword [rel request + 24], query_target_len
    ret

prepare_bad_query_request:
    lea rax, [rel method_get]
    mov [rel request + 0], rax
    mov qword [rel request + 8], method_get_len
    lea rax, [rel bad_query_target]
    mov [rel request + 16], rax
    mov qword [rel request + 24], bad_query_target_len
    ret

prepare_lifetime_request:
    lea rax, [rel method_get]
    mov [rel request + 0], rax
    mov qword [rel request + 8], method_get_len
    lea rax, [rel lifetime_target]
    mov [rel request + 16], rax
    mov qword [rel request + 24], lifetime_target_len
    ret

exact_handler:
    mov eax, 111
    ret
param_handler:
    mov eax, 222
    ret
duplicate_first:
    mov eax, 333
    ret
duplicate_second:
    mov eax, 444
    ret

lifetime_handler:
    mov rax, 0x7d2a9f013355aaee
    cmp [rsi], rax
    jne .bad
    cmp rcx, 2
    jne .bad

    lea rax, [rel lifetime_name1 + 1]
    cmp [rdx + 0], rax
    jne .bad
    cmp qword [rdx + 8], 2
    jne .bad
    lea rax, [rel lifetime_value1]
    cmp [rdx + 16], rax
    jne .bad
    cmp qword [rdx + 24], 2
    jne .bad

    lea rax, [rel lifetime_name2 + 1]
    cmp [rdx + 32 + 0], rax
    jne .bad
    cmp qword [rdx + 32 + 8], 4
    jne .bad
    lea rax, [rel lifetime_value2]
    cmp [rdx + 32 + 16], rax
    jne .bad
    cmp qword [rdx + 32 + 24], 4
    jne .bad

    mov eax, 909
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
