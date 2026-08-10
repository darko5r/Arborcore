; Arborcore connection state engine test; exit 0 pass, 1 fail
%define SYS_EXIT 60
%define ACCEPTED 1
%define READING 2
%define REQUEST_READY 3
%define DISPATCHING 4
%define WRITING 5
%define KEEP_ALIVE 6
%define CLOSING 7
%define CLOSED 8

global _start
extern connection_init
extern connection_state
extern connection_transition
extern connection_note_read
extern connection_note_write
extern connection_complete_request
extern connection_set_error
extern connection_reset_io

section .bss
align 16
conn: resb 80
inbuf: resb 24
outbuf: resb 24
arena: resb 24

section .text
_start:
    lea rdi, [rel conn]
    mov esi, 7
    lea rdx, [rel inbuf]
    lea rcx, [rel outbuf]
    lea r8, [rel arena]
    call connection_init
    test rax, rax
    jnz fail

    lea rdi, [rel conn]
    call connection_state
    cmp rax, ACCEPTED
    jne fail

    lea rdi, [rel conn]
    mov esi, READING
    call connection_transition
    test rax, rax
    jnz fail

    lea rdi, [rel conn]
    mov esi, 12
    call connection_note_read
    test rax, rax
    jnz fail
    cmp rdx, 12
    jne fail

    ; Counter overflow is rejected without wrapping.
    mov qword [rel conn + 48], -1
    lea rdi, [rel conn]
    mov esi, 1
    call connection_note_read
    cmp rax, -75
    jne fail
    cmp qword [rel conn + 48], -1
    jne fail
    mov qword [rel conn + 48], 12

    lea rdi, [rel conn]
    mov esi, REQUEST_READY
    call connection_transition
    test rax, rax
    jnz fail
    lea rdi, [rel conn]
    mov esi, DISPATCHING
    call connection_transition
    test rax, rax
    jnz fail
    lea rdi, [rel conn]
    mov esi, WRITING
    call connection_transition
    test rax, rax
    jnz fail

    lea rdi, [rel conn]
    mov esi, 9
    call connection_note_write
    test rax, rax
    jnz fail
    cmp rdx, 9
    jne fail

    mov qword [rel conn + 56], -1
    lea rdi, [rel conn]
    mov esi, 1
    call connection_note_write
    cmp rax, -75
    jne fail
    mov qword [rel conn + 56], 9

    ; Completed-request counter is checked too.
    mov qword [rel conn + 64], -1
    lea rdi, [rel conn]
    call connection_complete_request
    cmp rax, -75
    jne fail
    mov qword [rel conn + 64], 0

    lea rdi, [rel conn]
    call connection_complete_request
    test rax, rax
    jnz fail
    cmp rdx, 1
    jne fail
    cmp qword [rel conn + 48], 0
    jne fail
    cmp qword [rel conn + 56], 0
    jne fail

    lea rdi, [rel conn]
    mov esi, KEEP_ALIVE
    call connection_transition
    test rax, rax
    jnz fail
    lea rdi, [rel conn]
    mov esi, READING
    call connection_transition
    test rax, rax
    jnz fail

    ; Illegal READING -> WRITING transition.
    lea rdi, [rel conn]
    mov esi, WRITING
    call connection_transition
    cmp rax, -22
    jne fail
    cmp qword [rel conn + 8], READING
    jne fail

    lea rdi, [rel conn]
    mov rsi, -99
    call connection_set_error
    test rax, rax
    jnz fail
    cmp qword [rel conn + 72], -99
    jne fail

    lea rdi, [rel conn]
    mov esi, CLOSING
    call connection_transition
    test rax, rax
    jnz fail
    lea rdi, [rel conn]
    mov esi, CLOSED
    call connection_transition
    test rax, rax
    jnz fail

    xor edi, edi
    jmp exit
fail:
    mov edi, 1
exit:
    mov eax, SYS_EXIT
    syscall
section .note.GNU-stack noalloc noexec nowrite progbits
