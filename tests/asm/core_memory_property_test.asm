; Arborcore Core Retrofit C1 memory geometry/property qualification
; Exit 0 pass, nonzero fail.

%define SYS_EXIT 60
%define REGION_SIZE 32
%define COPY_CAP 80

global _start

extern memory_copy
extern memory_copy_scalar
extern memory_copy_qword
extern memory_copy_rep
extern memory_move

section .bss
alignb 64
move_work:     resb REGION_SIZE
move_snapshot: resb REGION_SIZE
move_expected: resb REGION_SIZE

alignb 64
copy_source:   resb COPY_CAP + 8
copy_expected: resb COPY_CAP + 8
copy_a:        resb COPY_CAP + 8
copy_b:        resb COPY_CAP + 8
copy_c:        resb COPY_CAP + 8
copy_d:        resb COPY_CAP + 8

section .text

_start:
    call test_memory_move_exhaustive
    test eax, eax
    jnz .fail_move

    call test_copy_equivalence
    test eax, eax
    jnz .fail_copy

    xor edi, edi
    jmp .exit

.fail_move:
    mov edi, 1
    jmp .exit
.fail_copy:
    mov edi, 2
.exit:
    mov eax, SYS_EXIT
    syscall

; Exhaust every valid (src,dst,len) over a 32-byte model region.
test_memory_move_exhaustive:
    sub rsp, 8
    xor r12d, r12d                 ; src offset
.src_loop:
    cmp r12d, REGION_SIZE
    jae .pass
    xor r13d, r13d                 ; dst offset
.dst_loop:
    cmp r13d, REGION_SIZE
    jae .next_src
    xor r14d, r14d                 ; length
.len_loop:
    cmp r14d, REGION_SIZE + 1
    jae .next_dst

    mov eax, r12d
    add eax, r14d
    cmp eax, REGION_SIZE
    ja .next_len
    mov eax, r13d
    add eax, r14d
    cmp eax, REGION_SIZE
    ja .next_len

    ; snapshot/work/expected = deterministic original bytes.
    lea r8, [rel move_snapshot]
    lea r9, [rel move_work]
    lea r10, [rel move_expected]
    xor ecx, ecx
.init:
    cmp ecx, REGION_SIZE
    jae .build_expected
    mov eax, ecx
    imul eax, eax, 17
    add eax, 3
    mov [r8 + rcx], al
    mov [r9 + rcx], al
    mov [r10 + rcx], al
    inc ecx
    jmp .init

.build_expected:
    lea r8, [rel move_snapshot]
    add r8, r12
    lea r9, [rel move_expected]
    add r9, r13
    xor ecx, ecx
.exp_loop:
    cmp ecx, r14d
    jae .invoke
    movzx eax, byte [r8 + rcx]
    mov [r9 + rcx], al
    inc ecx
    jmp .exp_loop

.invoke:
    lea rdi, [rel move_work]
    add rdi, r13
    mov r15, rdi                    ; expected returned destination
    lea rsi, [rel move_work]
    add rsi, r12
    mov rdx, r14
    call memory_move
    cmp rax, r15
    jne .fail

    lea r8, [rel move_work]
    lea r9, [rel move_expected]
    xor ecx, ecx
.compare:
    cmp ecx, REGION_SIZE
    jae .next_len
    mov al, [r8 + rcx]
    cmp al, [r9 + rcx]
    jne .fail
    inc ecx
    jmp .compare

.next_len:
    inc r14d
    jmp .len_loop
.next_dst:
    inc r13d
    jmp .dst_loop
.next_src:
    inc r12d
    jmp .src_loop
.pass:
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

; For lengths 0..64 and all source/destination residues 0..7,
; scalar/qword/REP/dispatcher must be observationally equivalent.
test_copy_equivalence:
    sub rsp, 8
    ; Fill source base with changing bytes once.
    lea r8, [rel copy_source]
    xor ecx, ecx
.src_init:
    cmp ecx, COPY_CAP + 8
    jae .length_start
    mov eax, ecx
    imul eax, eax, 29
    add eax, 11
    mov [r8 + rcx], al
    inc ecx
    jmp .src_init

.length_start:
    xor r12d, r12d                  ; length
.length_loop:
    cmp r12d, 65
    jae .pass
    xor r13d, r13d                  ; source residue
.src_res_loop:
    cmp r13d, 8
    jae .next_length
    xor r14d, r14d                  ; destination residue
.dst_res_loop:
    cmp r14d, 8
    jae .next_src_res

    ; expected and all destinations = 0xCC sentinel.
    lea r8, [rel copy_expected]
    lea r9, [rel copy_a]
    lea r10, [rel copy_b]
    lea r11, [rel copy_c]
    lea r15, [rel copy_d]
    xor ecx, ecx
.fill_dest:
    cmp ecx, COPY_CAP + 8
    jae .fill_expected_payload
    mov byte [r8 + rcx], 0xCC
    mov byte [r9 + rcx], 0xCC
    mov byte [r10 + rcx], 0xCC
    mov byte [r11 + rcx], 0xCC
    mov byte [r15 + rcx], 0xCC
    inc ecx
    jmp .fill_dest

.fill_expected_payload:
    lea r8, [rel copy_source]
    add r8, r13
    lea r9, [rel copy_expected]
    add r9, r14
    xor ecx, ecx
.exp_payload:
    cmp ecx, r12d
    jae .call_scalar
    mov al, [r8 + rcx]
    mov [r9 + rcx], al
    inc ecx
    jmp .exp_payload

.call_scalar:
    lea rdi, [rel copy_a]
    add rdi, r14
    lea rsi, [rel copy_source]
    add rsi, r13
    mov rdx, r12
    call memory_copy_scalar

    lea rdi, [rel copy_b]
    add rdi, r14
    lea rsi, [rel copy_source]
    add rsi, r13
    mov rdx, r12
    call memory_copy_qword

    lea rdi, [rel copy_c]
    add rdi, r14
    lea rsi, [rel copy_source]
    add rsi, r13
    mov rdx, r12
    call memory_copy_rep

    lea rdi, [rel copy_d]
    add rdi, r14
    lea rsi, [rel copy_source]
    add rsi, r13
    mov rdx, r12
    call memory_copy

    lea r8, [rel copy_expected]
    lea r9, [rel copy_a]
    lea r10, [rel copy_b]
    lea r11, [rel copy_c]
    lea r15, [rel copy_d]
    xor ecx, ecx
.compare_all:
    cmp ecx, COPY_CAP + 8
    jae .next_dst_res
    mov al, [r8 + rcx]
    cmp al, [r9 + rcx]
    jne .fail
    cmp al, [r10 + rcx]
    jne .fail
    cmp al, [r11 + rcx]
    jne .fail
    cmp al, [r15 + rcx]
    jne .fail
    inc ecx
    jmp .compare_all

.next_dst_res:
    inc r14d
    jmp .dst_res_loop
.next_src_res:
    inc r13d
    jmp .src_res_loop
.next_length:
    inc r12d
    jmp .length_loop
.pass:
    add rsp, 8
    xor eax, eax
    ret
.fail:
    add rsp, 8
    mov eax, 1
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
