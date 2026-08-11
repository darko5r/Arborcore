; Arborcore Retrofit D1 request-target property qualification
; exit 0 pass, 1 fail
;
; Exhaustively checks every possible byte in a query position and a path
; position, validates first-'?' splitting, and proves invalid inputs clear the
; published target view.

%define SYS_EXIT      60
%define ERR_EINVAL   -22
%define ERR_EOVERFLOW -75

%define TARGET_PATH_PTR   0
%define TARGET_PATH_LEN   8
%define TARGET_QUERY_PTR 16
%define TARGET_QUERY_LEN 24

extern request_target_split

global _start

section .rodata
multi_query: db "/x?a?b"
multi_query_len equ $ - multi_query
bad_query_fragment: db "/x?a#b"
bad_query_fragment_len equ $ - bad_query_fragment
bad_query_space: db "/x?a b"
bad_query_space_len equ $ - bad_query_space
asterisk_query: db "*?x"
asterisk_query_len equ $ - asterisk_query

section .data
query_probe: db '/', 'x', '?', 0
path_probe:  db '/', 0

section .bss
alignb 16
out: resb 32

section .text
_start:
    ; ------------------------------------------------------------
    ; Query byte domain: visible ASCII except '#' is accepted.
    ; '?' after the separator is ordinary query data.
    ; ------------------------------------------------------------
    xor ebx, ebx
.query_domain:
    mov [rel query_probe + 3], bl
    call poison_out

    lea rdi, [rel query_probe]
    mov esi, 4
    lea rdx, [rel out]
    call request_target_split

    cmp ebx, 0x21
    jb .query_expect_invalid
    cmp ebx, 0x7e
    ja .query_expect_invalid
    cmp ebx, '#'
    je .query_expect_invalid

    test rax, rax
    jnz fail
    lea rax, [rel query_probe]
    cmp [rel out + TARGET_PATH_PTR], rax
    jne fail
    cmp qword [rel out + TARGET_PATH_LEN], 2
    jne fail
    lea rax, [rel query_probe + 3]
    cmp [rel out + TARGET_QUERY_PTR], rax
    jne fail
    cmp qword [rel out + TARGET_QUERY_LEN], 1
    jne fail
    jmp .query_next

.query_expect_invalid:
    cmp rax, ERR_EINVAL
    jne fail
    call require_zero_out
    test eax, eax
    jz fail

.query_next:
    inc ebx
    cmp ebx, 256
    jb .query_domain

    ; ------------------------------------------------------------
    ; Path byte domain. The first '?' becomes the separator; every
    ; other visible ASCII byte except '#' remains part of the path.
    ; ------------------------------------------------------------
    xor ebx, ebx
.path_domain:
    mov [rel path_probe + 1], bl
    call poison_out

    lea rdi, [rel path_probe]
    mov esi, 2
    lea rdx, [rel out]
    call request_target_split

    cmp ebx, 0x21
    jb .path_expect_invalid
    cmp ebx, 0x7e
    ja .path_expect_invalid
    cmp ebx, '#'
    je .path_expect_invalid

    test rax, rax
    jnz fail
    lea rax, [rel path_probe]
    cmp [rel out + TARGET_PATH_PTR], rax
    jne fail
    cmp ebx, '?'
    je .path_is_separator
    cmp qword [rel out + TARGET_PATH_LEN], 2
    jne fail
    cmp qword [rel out + TARGET_QUERY_PTR], 0
    jne fail
    cmp qword [rel out + TARGET_QUERY_LEN], 0
    jne fail
    jmp .path_next

.path_is_separator:
    cmp qword [rel out + TARGET_PATH_LEN], 1
    jne fail
    lea rax, [rel path_probe + 2]
    cmp [rel out + TARGET_QUERY_PTR], rax
    jne fail
    cmp qword [rel out + TARGET_QUERY_LEN], 0
    jne fail
    jmp .path_next

.path_expect_invalid:
    cmp rax, ERR_EINVAL
    jne fail
    call require_zero_out
    test eax, eax
    jz fail

.path_next:
    inc ebx
    cmp ebx, 256
    jb .path_domain

    ; Only the first '?' splits; later '?' remains query data.
    lea rdi, [rel multi_query]
    mov esi, multi_query_len
    lea rdx, [rel out]
    call request_target_split
    test rax, rax
    jnz fail
    cmp qword [rel out + TARGET_PATH_LEN], 2
    jne fail
    lea rax, [rel multi_query + 3]
    cmp [rel out + TARGET_QUERY_PTR], rax
    jne fail
    cmp qword [rel out + TARGET_QUERY_LEN], 3
    jne fail

    ; Raw fragment marker remains invalid after the query separator.
    call poison_out
    lea rdi, [rel bad_query_fragment]
    mov esi, bad_query_fragment_len
    lea rdx, [rel out]
    call request_target_split
    cmp rax, ERR_EINVAL
    jne fail
    call require_zero_out
    test eax, eax
    jz fail

    ; Non-visible ASCII remains invalid after the query separator.
    call poison_out
    lea rdi, [rel bad_query_space]
    mov esi, bad_query_space_len
    lea rdx, [rel out]
    call request_target_split
    cmp rax, ERR_EINVAL
    jne fail
    call require_zero_out
    test eax, eax
    jz fail

    ; Asterisk form is exactly '*', never '*?...'.
    call poison_out
    lea rdi, [rel asterisk_query]
    mov esi, asterisk_query_len
    lea rdx, [rel out]
    call request_target_split
    cmp rax, ERR_EINVAL
    jne fail
    call require_zero_out
    test eax, eax
    jz fail

    ; Target end-address overflow is rejected before dereference and clears out.
    call poison_out
    mov rdi, -8
    mov esi, 16
    lea rdx, [rel out]
    call request_target_split
    cmp rax, ERR_EOVERFLOW
    jne fail
    call require_zero_out
    test eax, eax
    jz fail

    ; NULL output is invalid and must not be dereferenced.
    lea rdi, [rel multi_query]
    mov esi, multi_query_len
    xor edx, edx
    call request_target_split
    cmp rax, ERR_EINVAL
    jne fail

    xor edi, edi
    jmp exit

poison_out:
    mov rax, -1
    mov [rel out + 0], rax
    mov [rel out + 8], rax
    mov [rel out + 16], rax
    mov [rel out + 24], rax
    ret

require_zero_out:
    xor eax, eax
    cmp qword [rel out + 0], 0
    jne .no
    cmp qword [rel out + 8], 0
    jne .no
    cmp qword [rel out + 16], 0
    jne .no
    cmp qword [rel out + 24], 0
    jne .no
    mov eax, 1
.no:
    ret

fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
