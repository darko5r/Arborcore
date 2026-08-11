; Arborcore Core Retrofit C2 buffer alias/snapshot qualification
; Exhausts overlapping append geometry inside a 32-byte backing store.
; Exit 0 pass, nonzero fail.

%define SYS_EXIT 60
%define BUFFER_LENGTH 8
%define CAPACITY 32

extern buffer_init
extern buffer_append

global _start

section .bss
alignb 16
before_guard: resb 8
storage:      resb CAPACITY
after_guard:  resb 8
snapshot:     resb CAPACITY
expected:     resb CAPACITY
buffer_obj:   resb 24

section .text
_start:
    xor r12d, r12d                 ; old logical length 0..16
.old_loop:
    cmp r12d, 17
    jae .pass

    mov r13d, 1                    ; append length 1..16
.len_loop:
    cmp r13d, 17
    jae .next_old

    mov eax, r12d
    add eax, r13d
    cmp eax, CAPACITY
    ja .next_len

    xor r14d, r14d                 ; source offset
.src_loop:
    mov eax, CAPACITY
    sub eax, r13d
    cmp r14d, eax
    ja .next_len

    ; Guards catch writes outside backing storage.
    lea r8, [rel before_guard]
    lea r9, [rel after_guard]
    xor ecx, ecx
.guard_init:
    cmp ecx, 8
    jae .storage_init
    mov byte [r8 + rcx], 0xA5
    mov byte [r9 + rcx], 0x5A
    inc ecx
    jmp .guard_init

.storage_init:
    lea r8, [rel storage]
    lea r9, [rel snapshot]
    lea r10, [rel expected]
    xor ecx, ecx
.fill_loop:
    cmp ecx, CAPACITY
    jae .prepare_expected
    mov eax, ecx
    imul eax, eax, 37
    add eax, 11
    mov [r8 + rcx], al
    mov [r9 + rcx], al
    mov [r10 + rcx], al
    inc ecx
    jmp .fill_loop

.prepare_expected:
    ; expected[old+i] = snapshot[source+i]
    lea r8, [rel snapshot]
    lea r9, [rel expected]
    xor ecx, ecx
.expected_loop:
    cmp ecx, r13d
    jae .init_buffer
    mov r10, r14
    add r10, rcx
    mov al, [r8 + r10]
    mov r11, r12
    add r11, rcx
    mov [r9 + r11], al
    inc ecx
    jmp .expected_loop

.init_buffer:
    lea rdi, [rel buffer_obj]
    lea rsi, [rel storage]
    mov edx, CAPACITY
    call buffer_init
    test rax, rax
    jnz .fail
    mov [rel buffer_obj + BUFFER_LENGTH], r12

    lea rdi, [rel buffer_obj]
    lea rsi, [rel storage]
    add rsi, r14
    mov rdx, r13
    call buffer_append
    test rax, rax
    jnz .fail

    mov rax, r12
    add rax, r13
    cmp rdx, rax
    jne .fail
    cmp [rel buffer_obj + BUFFER_LENGTH], rax
    jne .fail

    ; The complete backing store must match snapshot semantics: only the
    ; append destination is modified.
    lea r8, [rel storage]
    lea r9, [rel expected]
    xor ecx, ecx
.compare_storage:
    cmp ecx, CAPACITY
    jae .check_guards
    mov al, [r8 + rcx]
    cmp al, [r9 + rcx]
    jne .fail
    inc ecx
    jmp .compare_storage

.check_guards:
    lea r8, [rel before_guard]
    lea r9, [rel after_guard]
    xor ecx, ecx
.guard_check:
    cmp ecx, 8
    jae .next_src
    cmp byte [r8 + rcx], 0xA5
    jne .fail
    cmp byte [r9 + rcx], 0x5A
    jne .fail
    inc ecx
    jmp .guard_check

.next_src:
    inc r14d
    jmp .src_loop
.next_len:
    inc r13d
    jmp .len_loop
.next_old:
    inc r12d
    jmp .old_loop

.pass:
    xor edi, edi
    jmp .exit
.fail:
    mov edi, 1
.exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
