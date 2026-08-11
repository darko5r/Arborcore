; Arborcore Core Retrofit C1 connection-state property qualification
; Exhausts the complete 8x8 valid-state transition domain.
; Exit 0 pass, nonzero fail.

%define SYS_EXIT 60
%define ERR_EINVAL -22
%define ERR_EOVERFLOW -75

%define ACCEPTED 1
%define READING 2
%define REQUEST_READY 3
%define DISPATCHING 4
%define WRITING 5
%define KEEP_ALIVE 6
%define CLOSING 7
%define CLOSED 8

%define CONN_STATE 8
%define CONN_READ_BYTES 48
%define CONN_WRITE_BYTES 56
%define CONN_REQUEST_COUNT 64

global _start
extern connection_init
extern connection_transition
extern connection_note_read
extern connection_note_write
extern connection_complete_request

section .rodata
; Bit n (1-based state) indicates an allowed destination from each source.
; index 0 unused.
transition_masks:
    dw 0
    dw (1 << READING) | (1 << CLOSING)
    dw (1 << REQUEST_READY) | (1 << CLOSING)
    dw (1 << DISPATCHING) | (1 << CLOSING)
    dw (1 << WRITING) | (1 << CLOSING)
    dw (1 << KEEP_ALIVE) | (1 << CLOSING)
    dw (1 << READING) | (1 << CLOSING)
    dw (1 << CLOSED)
    dw 0

invalid_states: dq 0, 9, -1

section .bss
align 16
conn: resb 80
inbuf: resb 24
outbuf: resb 24
arena: resb 24

section .text
_start:
    call test_transition_matrix
    test eax, eax
    jnz .fail_transition
    call test_invalid_states
    test eax, eax
    jnz .fail_invalid
    call test_counter_transactions
    test eax, eax
    jnz .fail_counter

    xor edi, edi
    jmp .exit
.fail_transition:
    mov edi, 1
    jmp .exit
.fail_invalid:
    mov edi, 2
    jmp .exit
.fail_counter:
    mov edi, 3
.exit:
    mov eax, SYS_EXIT
    syscall

init_conn:
    sub rsp, 8
    lea rdi, [rel conn]
    mov esi, 7
    lea rdx, [rel inbuf]
    lea rcx, [rel outbuf]
    lea r8, [rel arena]
    call connection_init
    add rsp, 8
    ret

test_transition_matrix:
    sub rsp, 8
    mov r12d, ACCEPTED
.old_loop:
    cmp r12d, CLOSED + 1
    jae .pass
    mov r13d, ACCEPTED
.new_loop:
    cmp r13d, CLOSED + 1
    jae .next_old

    call init_conn
    test rax, rax
    jnz .fail
    mov [rel conn + CONN_STATE], r12

    lea rdi, [rel conn]
    mov rsi, r13
    call connection_transition

    mov edx, 1
    mov ecx, r13d
    shl edx, cl
    lea r8, [rel transition_masks]
    movzx ecx, word [r8 + r12*2]
    test ecx, edx
    jz .expect_reject

    test rax, rax
    jnz .fail
    cmp [rel conn + CONN_STATE], r13
    jne .fail
    jmp .next_new

.expect_reject:
    cmp rax, ERR_EINVAL
    jne .fail
    cmp [rel conn + CONN_STATE], r12
    jne .fail

.next_new:
    inc r13d
    jmp .new_loop
.next_old:
    inc r12d
    jmp .old_loop
.pass:
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

test_invalid_states:
    sub rsp, 8
    ; Invalid destination must preserve every valid current state.
    mov r12d, ACCEPTED
.old_loop:
    cmp r12d, CLOSED + 1
    jae .invalid_current
    xor r13d, r13d
.inv_new_loop:
    cmp r13d, 3
    jae .next_old
    call init_conn
    test rax, rax
    jnz .fail
    mov [rel conn + CONN_STATE], r12
    lea rdi, [rel conn]
    lea r8, [rel invalid_states]
    mov rsi, [r8 + r13*8]
    call connection_transition
    cmp rax, ERR_EINVAL
    jne .fail
    cmp [rel conn + CONN_STATE], r12
    jne .fail
    inc r13d
    jmp .inv_new_loop
.next_old:
    inc r12d
    jmp .old_loop

.invalid_current:
    xor r12d, r12d
.inv_old_loop:
    cmp r12d, 3
    jae .pass
    mov r13d, ACCEPTED
.valid_new_loop:
    cmp r13d, CLOSED + 1
    jae .next_inv_old
    call init_conn
    test rax, rax
    jnz .fail
    lea r8, [rel invalid_states]
    mov rax, [r8 + r12*8]
    mov [rel conn + CONN_STATE], rax
    lea rdi, [rel conn]
    mov rsi, r13
    call connection_transition
    cmp rax, ERR_EINVAL
    jne .fail
    lea r8, [rel invalid_states]
    mov rax, [r8 + r12*8]
    cmp [rel conn + CONN_STATE], rax
    jne .fail
    inc r13d
    jmp .valid_new_loop
.next_inv_old:
    inc r12d
    jmp .inv_old_loop
.pass:
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

test_counter_transactions:
    sub rsp, 8
    call init_conn
    test rax, rax
    jnz .fail

    ; READING overflow leaves read counter unchanged.
    mov qword [rel conn + CONN_STATE], READING
    mov qword [rel conn + CONN_READ_BYTES], -1
    lea rdi, [rel conn]
    mov esi, 1
    call connection_note_read
    cmp rax, ERR_EOVERFLOW
    jne .fail
    cmp qword [rel conn + CONN_READ_BYTES], -1
    jne .fail

    ; WRITING overflow leaves write counter unchanged.
    mov qword [rel conn + CONN_STATE], WRITING
    mov qword [rel conn + CONN_WRITE_BYTES], -1
    lea rdi, [rel conn]
    mov esi, 1
    call connection_note_write
    cmp rax, ERR_EOVERFLOW
    jne .fail
    cmp qword [rel conn + CONN_WRITE_BYTES], -1
    jne .fail

    ; Complete-request overflow must not reset I/O counters.
    mov qword [rel conn + CONN_READ_BYTES], 123
    mov qword [rel conn + CONN_WRITE_BYTES], 456
    mov qword [rel conn + CONN_REQUEST_COUNT], -1
    lea rdi, [rel conn]
    call connection_complete_request
    cmp rax, ERR_EOVERFLOW
    jne .fail
    cmp qword [rel conn + CONN_REQUEST_COUNT], -1
    jne .fail
    cmp qword [rel conn + CONN_READ_BYTES], 123
    jne .fail
    cmp qword [rel conn + CONN_WRITE_BYTES], 456
    jne .fail

    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
