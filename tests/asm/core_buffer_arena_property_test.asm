; Arborcore Core Retrofit C1 buffer/arena algebra qualification
; Exit 0 pass, nonzero fail.

%define SYS_EXIT 60
%define ERR_ENOSPC -28

%define BUFFER_LENGTH 8
%define ARENA_CAPACITY 8
%define ARENA_OFFSET 16

%define BUF_CAP 64

global _start

extern buffer_init
extern buffer_reset
extern buffer_append
extern buffer_consume
extern buffer_remaining
extern arena_init
extern arena_alloc_aligned
extern arena_mark
extern arena_rewind

section .rodata
x_data: db 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17
y_data: db 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87
xy_data: db 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87
seed16: db 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
alignments: dq 1,2,4,8,16

section .bss
alignb 16
buf_a: resb 24
buf_b: resb 24
store_a: resb BUF_CAP
store_b: resb BUF_CAP
concat_temp: resb 16

alignb 16
arena_obj: resb 24
arena_storage: resb 64
ref_aligned: resq 1
ref_end: resq 1
ref_aligned_offset: resq 1

section .text
_start:
    call test_buffer_append_associativity
    test eax, eax
    jnz .fail_buffer
    call test_buffer_consume_composition
    test eax, eax
    jnz .fail_buffer
    call test_buffer_reset_idempotence
    test eax, eax
    jnz .fail_buffer
    call test_arena_geometry
    test eax, eax
    jnz .fail_arena

    xor edi, edi
    jmp .exit
.fail_buffer:
    mov edi, 1
    jmp .exit
.fail_arena:
    mov edi, 2
.exit:
    mov eax, SYS_EXIT
    syscall

; append(append(B,X),Y) == append(B,X||Y) for |X|,|Y| in 0..8.
test_buffer_append_associativity:
    sub rsp, 8
    xor r12d, r12d
.x_loop:
    cmp r12d, 9
    jae .pass
    xor r13d, r13d
.y_loop:
    cmp r13d, 9
    jae .next_x

    lea rdi, [rel buf_a]
    lea rsi, [rel store_a]
    mov edx, BUF_CAP
    call buffer_init
    test rax, rax
    jnz .fail
    lea rdi, [rel buf_b]
    lea rsi, [rel store_b]
    mov edx, BUF_CAP
    call buffer_init
    test rax, rax
    jnz .fail

    lea rdi, [rel buf_a]
    lea rsi, [rel x_data]
    mov rdx, r12
    call buffer_append
    test rax, rax
    jnz .fail
    lea rdi, [rel buf_a]
    lea rsi, [rel y_data]
    mov rdx, r13
    call buffer_append
    test rax, rax
    jnz .fail

    ; Test-side independent concatenation into concat_temp.
    lea r9, [rel x_data]
    lea r10, [rel concat_temp]
    xor ecx, ecx
.copy_x:
    cmp ecx, r12d
    jae .copy_y_start
    mov al, [r9 + rcx]
    mov [r10 + rcx], al
    inc ecx
    jmp .copy_x
.copy_y_start:
    lea r9, [rel y_data]
    lea r10, [rel concat_temp]
    xor ecx, ecx
.copy_y:
    cmp ecx, r13d
    jae .single_append
    mov al, [r9 + rcx]
    mov r8, r12
    add r8, rcx
    mov [r10 + r8], al
    inc ecx
    jmp .copy_y

.single_append:
    lea rdi, [rel buf_b]
    lea rsi, [rel concat_temp]
    mov rdx, r12
    add rdx, r13
    call buffer_append
    test rax, rax
    jnz .fail

    mov rax, [rel buf_a + BUFFER_LENGTH]
    cmp rax, [rel buf_b + BUFFER_LENGTH]
    jne .fail
    mov rcx, rax
    lea r9, [rel store_a]
    lea r10, [rel store_b]
    xor r8d, r8d
.compare:
    cmp r8, rcx
    jae .remaining_check
    mov al, [r9 + r8]
    cmp al, [r10 + r8]
    jne .fail
    inc r8
    jmp .compare
.remaining_check:
    lea rdi, [rel buf_a]
    call buffer_remaining
    mov rcx, BUF_CAP
    sub rcx, r12
    sub rcx, r13
    cmp rax, rcx
    jne .fail

    inc r13d
    jmp .y_loop
.next_x:
    inc r12d
    jmp .x_loop
.pass:
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

; consume(consume(B,a),b) == consume(B,a+b) for a+b<=16.
test_buffer_consume_composition:
    sub rsp, 8
    xor r12d, r12d
.a_loop:
    cmp r12d, 17
    jae .pass
    xor r13d, r13d
.b_loop:
    mov eax, r12d
    add eax, r13d
    cmp eax, 16
    ja .next_a

    lea rdi, [rel buf_a]
    lea rsi, [rel store_a]
    mov edx, BUF_CAP
    call buffer_init
    test rax, rax
    jnz .fail
    lea rdi, [rel buf_b]
    lea rsi, [rel store_b]
    mov edx, BUF_CAP
    call buffer_init
    test rax, rax
    jnz .fail

    lea rdi, [rel buf_a]
    lea rsi, [rel seed16]
    mov edx, 16
    call buffer_append
    test rax, rax
    jnz .fail
    lea rdi, [rel buf_b]
    lea rsi, [rel seed16]
    mov edx, 16
    call buffer_append
    test rax, rax
    jnz .fail

    lea rdi, [rel buf_a]
    mov rsi, r12
    call buffer_consume
    test rax, rax
    jnz .fail
    lea rdi, [rel buf_a]
    mov rsi, r13
    call buffer_consume
    test rax, rax
    jnz .fail

    lea rdi, [rel buf_b]
    mov rsi, r12
    add rsi, r13
    call buffer_consume
    test rax, rax
    jnz .fail

    mov rax, [rel buf_a + BUFFER_LENGTH]
    cmp rax, [rel buf_b + BUFFER_LENGTH]
    jne .fail
    mov rcx, rax
    lea r9, [rel store_a]
    lea r10, [rel store_b]
    xor r8d, r8d
.compare:
    cmp r8, rcx
    jae .next_b
    mov al, [r9 + r8]
    cmp al, [r10 + r8]
    jne .fail
    inc r8
    jmp .compare
.next_b:
    inc r13d
    jmp .b_loop
.next_a:
    inc r12d
    jmp .a_loop
.pass:
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

test_buffer_reset_idempotence:
    sub rsp, 8
    lea rdi, [rel buf_a]
    lea rsi, [rel store_a]
    mov edx, BUF_CAP
    call buffer_init
    test rax, rax
    jnz .fail
    lea rdi, [rel buf_a]
    lea rsi, [rel seed16]
    mov edx, 16
    call buffer_append
    test rax, rax
    jnz .fail
    lea rdi, [rel buf_a]
    call buffer_reset
    test rax, rax
    jnz .fail
    cmp qword [rel buf_a + BUFFER_LENGTH], 0
    jne .fail
    lea rdi, [rel buf_a]
    call buffer_reset
    test rax, rax
    jnz .fail
    cmp qword [rel buf_a + BUFFER_LENGTH], 0
    jne .fail
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

; Exhaust base residues 0..7, capacities 0..16, offsets 0..capacity,
; sizes 0..8, alignments {1,2,4,8,16}.  Reference math is computed
; independently from the actual backing address.
test_arena_geometry:
    sub rsp, 8
    xor r12d, r12d                 ; base residue
.base_loop:
    cmp r12d, 8
    jae .pass
    xor r13d, r13d                 ; capacity
.cap_loop:
    cmp r13d, 17
    jae .next_base
    xor r14d, r14d                 ; starting offset
.off_loop:
    cmp r14d, r13d
    ja .next_cap
    xor r15d, r15d                 ; alignment index
.align_loop:
    cmp r15d, 5
    jae .next_off
    xor ebx, ebx                    ; size
.size_loop:
    cmp ebx, 9
    jae .next_align

    lea rdi, [rel arena_obj]
    lea rsi, [rel arena_storage]
    add rsi, r12
    mov rdx, r13
    call arena_init
    test rax, rax
    jnz .fail
    mov [rel arena_obj + ARENA_OFFSET], r14

    ; Independent reference.
    lea r8, [rel arena_storage]
    add r8, r12                     ; base
    mov r9, r8
    add r9, r14                     ; current
    lea r10, [rel alignments]
    mov r10, [r10 + r15*8]
    mov r11, r10
    dec r11                         ; mask
    mov rax, r9
    add rax, r11
    not r11
    and rax, r11                    ; aligned address
    mov rcx, rax
    sub rcx, r8                     ; aligned offset
    mov rdx, rcx
    add rdx, rbx                    ; end offset

    mov [rel ref_aligned], rax
    mov [rel ref_end], rdx
    mov [rel ref_aligned_offset], rcx

    lea rdi, [rel arena_obj]
    mov rsi, rbx
    mov rdx, r10
    call arena_alloc_aligned

    mov rcx, [rel ref_aligned_offset]
    mov r10, [rel ref_end]
    mov r11, [rel ref_aligned]

    cmp rcx, r13
    ja .expect_nospace
    cmp r10, r13
    ja .expect_nospace

    test rax, rax
    jnz .fail
    cmp rdx, r11
    jne .fail
    cmp [rel arena_obj + ARENA_OFFSET], r10
    jne .fail

    ; mark is current end; rewind to original offset must restore frontier.
    lea rdi, [rel arena_obj]
    call arena_mark
    cmp rax, r10
    jne .fail
    lea rdi, [rel arena_obj]
    mov rsi, r14
    call arena_rewind
    test rax, rax
    jnz .fail
    cmp [rel arena_obj + ARENA_OFFSET], r14
    jne .fail
    jmp .next_size

.expect_nospace:
    cmp rax, ERR_ENOSPC
    jne .fail
    cmp [rel arena_obj + ARENA_OFFSET], r14
    jne .fail

.next_size:
    inc ebx
    jmp .size_loop
.next_align:
    inc r15d
    jmp .align_loop
.next_off:
    inc r14d
    jmp .off_loop
.next_cap:
    inc r13d
    jmp .cap_loop
.next_base:
    inc r12d
    jmp .base_loop
.pass:
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
