; Arborcore Retrofit D4 prepared exact-route-index experiment qualification
; exit 0 pass, 1 fail
;
; The experimental index must be functionally equivalent to router_find_exact
; for valid prepared catalogs, including first-valid duplicate semantics.

%define SYS_EXIT       60
%define ERR_EINVAL    -22
%define ERR_ENOSPC    -28

extern router_find_exact
extern route_index_prepare
extern route_index_find

global _start

section .rodata
method_get: db "GET"
method_get_len equ $ - method_get
method_post: db "POST"
method_post_len equ $ - method_post
path_a: db "/a"
path_a_len equ $ - path_a
path_b: db "/b"
path_b_len equ $ - path_b
path_c: db "/c"
path_c_len equ $ - path_c
path_missing: db "/missing"
path_missing_len equ $ - path_missing

section .data
align 8
routes:
    dq method_get,  method_get_len,  path_a, path_a_len, handler_a
    dq method_get,  method_get_len,  path_b, path_b_len, handler_b_first
    dq method_get,  method_get_len,  path_b, path_b_len, handler_b_second
    dq method_post, method_post_len, path_c, path_c_len, handler_c
routes_count equ 4

; First duplicate is unusable; both reference and prepared index must select
; the later valid duplicate.
routes_zero_then_valid:
    dq method_get, method_get_len, path_a, path_a_len, 0
    dq method_get, method_get_len, path_a, path_a_len, handler_a
routes_zero_then_valid_count equ 2

section .bss
alignb 16
slots: resq 16
small_slots: resq 2

section .text
_start:
    lea rdi, [rel routes]
    mov esi, routes_count
    lea rdx, [rel slots]
    mov ecx, 16
    call route_index_prepare
    test rax, rax
    jnz fail

    ; Every catalog lookup must equal the ordered linear reference.
    lea rdx, [rel method_get]
    mov ecx, method_get_len
    lea r8, [rel path_a]
    mov r9d, path_a_len
    call compare_lookup
    test eax, eax
    jz fail

    lea rdx, [rel method_get]
    mov ecx, method_get_len
    lea r8, [rel path_b]
    mov r9d, path_b_len
    call compare_lookup
    test eax, eax
    jz fail

    lea rdx, [rel method_post]
    mov ecx, method_post_len
    lea r8, [rel path_c]
    mov r9d, path_c_len
    call compare_lookup
    test eax, eax
    jz fail

    lea rdx, [rel method_get]
    mov ecx, method_get_len
    lea r8, [rel path_missing]
    mov r9d, path_missing_len
    call compare_lookup
    test eax, eax
    jz fail

    ; Duplicate exact route keeps the first valid handler.
    lea rdi, [rel slots]
    mov esi, 16
    lea rdx, [rel method_get]
    mov ecx, method_get_len
    lea r8, [rel path_b]
    mov r9d, path_b_len
    call route_index_find
    lea rcx, [rel handler_b_first]
    cmp rax, rcx
    jne fail

    ; Invalid/NULL-handler route is skipped just like router_find_exact.
    lea rdi, [rel routes_zero_then_valid]
    mov esi, routes_zero_then_valid_count
    lea rdx, [rel slots]
    mov ecx, 16
    call route_index_prepare
    test rax, rax
    jnz fail

    lea rdi, [rel slots]
    mov esi, 16
    lea rdx, [rel method_get]
    mov ecx, method_get_len
    lea r8, [rel path_a]
    mov r9d, path_a_len
    call route_index_find
    lea rcx, [rel handler_a]
    cmp rax, rcx
    jne fail

    ; Slot count is an explicit power-of-two prepared-index contract.
    lea rdi, [rel routes]
    mov esi, routes_count
    lea rdx, [rel slots]
    mov ecx, 12
    call route_index_prepare
    cmp rax, ERR_EINVAL
    jne fail

    ; Insufficient prepared capacity fails explicitly rather than silently
    ; dropping routes.
    lea rdi, [rel routes]
    mov esi, routes_count
    lea rdx, [rel small_slots]
    mov ecx, 2
    call route_index_prepare
    cmp rax, ERR_ENOSPC
    jne fail

    xor edi, edi
    jmp exit

; Inputs: RDX method, RCX method_len, R8 path, R9 path_len.
; Uses the primary routes catalog and returns EAX=1 iff both engines agree.
compare_lookup:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 16

    mov r12, rdx
    mov r13, rcx
    mov r14, r8
    mov r15, r9

    lea rdi, [rel routes]
    mov esi, routes_count
    mov rdx, r12
    mov rcx, r13
    mov r8, r14
    mov r9, r15
    call router_find_exact
    mov rbx, rax

    lea rdi, [rel slots]
    mov esi, 16
    mov rdx, r12
    mov rcx, r13
    mov r8, r14
    mov r9, r15
    call route_index_find
    cmp rax, rbx
    sete al
    movzx eax, al

    add rsp, 16
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

handler_a:
    mov eax, 101
    ret
handler_b_first:
    mov eax, 202
    ret
handler_b_second:
    mov eax, 303
    ret
handler_c:
    mov eax, 404
    ret

fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
