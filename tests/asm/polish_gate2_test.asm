; Arborcore Polish Gate #2 cross-layer request-lifecycle test
;
; Exercises:
;   arena -> buffer -> incremental HTTP parse -> router dispatch
;   buffer consume -> arena reset -> second request lifecycle
;
; exit 0 pass, 1 fail

%define SYS_EXIT    60
%define ERR_EAGAIN -11

%define REQ_MESSAGE_LENGTH 88

extern arena_init
extern arena_alloc_aligned
extern arena_reset

extern buffer_init
extern buffer_append
extern buffer_consume
extern buffer_length

extern http_parse_request
extern router_dispatch

global _start

section .rodata
method_get:    db "GET"
method_post:   db "POST"
path_users:    db "/users"
path_submit:   db "/submit"

request1_part1:
    db "GET /users HTTP/1.1",13,10
    db "Host: example.com",13,10
request1_part1_len equ $ - request1_part1

request1_part2:
    db 13,10
request1_part2_len equ $ - request1_part2

request2_part1:
    db "POST /submit HTTP/1.1",13,10
    db "Content-Length: 4",13,10
    db 13,10
    db "da"
request2_part1_len equ $ - request2_part1

request2_part2:
    db "ta"
request2_part2_len equ $ - request2_part2

section .data
align 8
routes:
    dq method_get, 3, path_users, 6, gate2_handler_users
    dq method_post, 4, path_submit, 7, gate2_handler_submit
route_count equ 2

context_magic:
    dq 0xA1B2C3D4E5F60718

expected_request_ptr:
    dq 0

section .bss
align 16
arena_struct:  resb 24
arena_storage: resb 1024

section .text
_start:
    ; Initialize one request-lifetime arena.
    lea rdi, [rel arena_struct]
    lea rsi, [rel arena_storage]
    mov edx, 1024
    call arena_init
    test rax, rax
    jnz gate2_fail

    ; Allocate buffer structure.
    lea rdi, [rel arena_struct]
    mov esi, 24
    mov edx, 8
    call arena_alloc_aligned
    test rax, rax
    jnz gate2_fail
    mov r12, rdx
    mov r15, rdx                  ; remember first allocation for reset test

    ; Allocate buffer storage.
    lea rdi, [rel arena_struct]
    mov esi, 256
    mov edx, 16
    call arena_alloc_aligned
    test rax, rax
    jnz gate2_fail
    mov r13, rdx

    ; Allocate parsed request structure.
    lea rdi, [rel arena_struct]
    mov esi, 96
    mov edx, 16
    call arena_alloc_aligned
    test rax, rax
    jnz gate2_fail
    mov r14, rdx

    mov rdi, r12
    mov rsi, r13
    mov edx, 256
    call buffer_init
    test rax, rax
    jnz gate2_fail

    ; First chunk does not yet contain the terminating empty line.
    mov rdi, r12
    lea rsi, [rel request1_part1]
    mov edx, request1_part1_len
    call buffer_append
    test rax, rax
    jnz gate2_fail

    mov rdi, r12
    call buffer_length
    mov rsi, rax
    mov rdi, r13
    mov rdx, r14
    call http_parse_request
    cmp rax, ERR_EAGAIN
    jne gate2_fail

    ; Complete request 1.
    mov rdi, r12
    lea rsi, [rel request1_part2]
    mov edx, request1_part2_len
    call buffer_append
    test rax, rax
    jnz gate2_fail

    mov rdi, r12
    call buffer_length
    mov rsi, rax
    mov rdi, r13
    mov rdx, r14
    call http_parse_request
    test rax, rax
    jnz gate2_fail

    mov [rel expected_request_ptr], r14
    lea rdi, [rel routes]
    mov esi, route_count
    mov rdx, r14
    lea rcx, [rel context_magic]
    call router_dispatch
    cmp rax, 111
    jne gate2_fail

    ; Consume exactly the complete request from the connection buffer.
    mov rsi, [r14 + REQ_MESSAGE_LENGTH]
    mov rdi, r12
    call buffer_consume
    test rax, rax
    jnz gate2_fail
    test rdx, rdx
    jnz gate2_fail

    ; End request lifetime with one arena reset.
    lea rdi, [rel arena_struct]
    call arena_reset
    test rax, rax
    jnz gate2_fail

    ; Same allocation sequence must reuse the arena from its base.
    lea rdi, [rel arena_struct]
    mov esi, 24
    mov edx, 8
    call arena_alloc_aligned
    test rax, rax
    jnz gate2_fail
    cmp rdx, r15
    jne gate2_fail
    mov r12, rdx

    lea rdi, [rel arena_struct]
    mov esi, 256
    mov edx, 16
    call arena_alloc_aligned
    test rax, rax
    jnz gate2_fail
    mov r13, rdx

    lea rdi, [rel arena_struct]
    mov esi, 96
    mov edx, 16
    call arena_alloc_aligned
    test rax, rax
    jnz gate2_fail
    mov r14, rdx

    mov rdi, r12
    mov rsi, r13
    mov edx, 256
    call buffer_init
    test rax, rax
    jnz gate2_fail

    ; Second request has complete headers but an incomplete body.
    mov rdi, r12
    lea rsi, [rel request2_part1]
    mov edx, request2_part1_len
    call buffer_append
    test rax, rax
    jnz gate2_fail

    mov rdi, r12
    call buffer_length
    mov rsi, rax
    mov rdi, r13
    mov rdx, r14
    call http_parse_request
    cmp rax, ERR_EAGAIN
    jne gate2_fail

    ; Complete the body and parse/dispatch again.
    mov rdi, r12
    lea rsi, [rel request2_part2]
    mov edx, request2_part2_len
    call buffer_append
    test rax, rax
    jnz gate2_fail

    mov rdi, r12
    call buffer_length
    mov rsi, rax
    mov rdi, r13
    mov rdx, r14
    call http_parse_request
    test rax, rax
    jnz gate2_fail

    mov [rel expected_request_ptr], r14
    lea rdi, [rel routes]
    mov esi, route_count
    mov rdx, r14
    lea rcx, [rel context_magic]
    call router_dispatch
    cmp rax, 222
    jne gate2_fail

    xor edi, edi
    jmp gate2_exit


gate2_handler_users:
    cmp rdi, [rel expected_request_ptr]
    jne gate2_handler_fail
    mov rax, 0xA1B2C3D4E5F60718
    cmp [rsi], rax
    jne gate2_handler_fail
    mov eax, 111
    ret

gate2_handler_submit:
    cmp rdi, [rel expected_request_ptr]
    jne gate2_handler_fail
    mov rax, 0xA1B2C3D4E5F60718
    cmp [rsi], rax
    jne gate2_handler_fail
    mov eax, 222
    ret

gate2_handler_fail:
    mov rax, -99
    ret

gate2_fail:
    mov edi, 1

gate2_exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
