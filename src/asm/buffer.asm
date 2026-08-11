; Arborcore bounded buffer engine
;
; Buffer layout (24 bytes):
;   +0   data pointer
;   +8   current length
;   +16  capacity
;
; Status convention:
;   RAX = 0      success
;   RAX = -22    EINVAL
;   RAX = -28    ENOSPC
;   RAX = -75    EOVERFLOW
;
; Mutating operations return the resulting buffer length in RDX on
; success and RDX = 0 on error.
;
; The buffer delegates bulk copies/moves to the qualified Arborcore
; memory engine rather than maintaining duplicate byte loops.

%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75

%define BUFFER_DATA      0
%define BUFFER_LENGTH    8
%define BUFFER_CAPACITY 16

extern memory_copy
extern memory_move

global buffer_init:function
global buffer_reset:function
global buffer_length:function
global buffer_remaining:function
global buffer_append:function
global buffer_append_byte:function
global buffer_consume:function

section .text

; buffer_init(buffer, data, capacity)
; RDI=buffer*, RSI=data*, RDX=capacity
buffer_init:
    test rdi, rdi
    jz .invalid

    test rdx, rdx
    jz .store

    test rsi, rsi
    jz .invalid

    ; Require the full caller-supplied storage range [data, data+capacity)
    ; to be representable without wrapping the 64-bit address space.
    mov rax, rsi
    add rax, rdx
    jc .overflow

.store:
    mov [rdi + BUFFER_DATA], rsi
    mov qword [rdi + BUFFER_LENGTH], 0
    mov [rdi + BUFFER_CAPACITY], rdx
    xor eax, eax
    xor edx, edx
    ret

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret

; buffer_reset(buffer)
buffer_reset:
    test rdi, rdi
    jz .invalid
    mov qword [rdi + BUFFER_LENGTH], 0
    xor eax, eax
    xor edx, edx
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

; buffer_length(buffer) -> RAX=length, or 0 for NULL
buffer_length:
    test rdi, rdi
    jz .zero
    mov rax, [rdi + BUFFER_LENGTH]
    ret
.zero:
    xor eax, eax
    ret

; buffer_remaining(buffer) -> RAX=capacity-length, or 0 if invalid
buffer_remaining:
    test rdi, rdi
    jz .zero
    mov rax, [rdi + BUFFER_CAPACITY]
    mov rcx, [rdi + BUFFER_LENGTH]
    cmp rcx, rax
    ja .zero
    sub rax, rcx
    ret
.zero:
    xor eax, eax
    ret

; buffer_append(buffer, source, length)
; RDI=buffer*, RSI=source*, RDX=length
;
; Snapshot semantics:
;   new logical contents = old logical contents || source_before_call.
;
; The source may alias the backing buffer, including the append
; destination.  Disjoint spans keep the qualified memory_copy fast path;
; overlapping spans use memory_move so unread source bytes are preserved.
; The caller must still provide a valid readable source span for nonzero
; length.
buffer_append:
    push rbx
    push r12
    push r13
    push r14
    sub rsp, 8                    ; 16-byte call alignment

    test rdi, rdi
    jz .invalid

    mov r12, rdi                  ; buffer
    mov r13, rsi                  ; source
    mov r14, rdx                  ; append length

    mov r8, [r12 + BUFFER_LENGTH]
    mov r9, [r12 + BUFFER_CAPACITY]
    cmp r8, r9
    ja .invalid

    test r14, r14
    jz .success_existing
    test r13, r13
    jz .invalid

    mov r10, r8
    add r10, r14
    jc .overflow
    cmp r10, r9
    ja .no_space
    mov rbx, r10                  ; resulting logical length

    mov r11, [r12 + BUFFER_DATA]
    test r11, r11
    jz .invalid

    mov rdi, r11
    add rdi, r8                   ; destination = data + old_length
    jc .overflow

    ; Equal-length spans overlap exactly when the unsigned distance
    ; between their starting addresses is smaller than the length.
    ; This avoids forming source+length solely for classification.
    cmp rdi, r13
    je .append_overlap
    ja .destination_above_source

    ; destination < source
    mov rcx, r13
    sub rcx, rdi
    cmp rcx, r14
    jb .append_overlap
    jmp .append_copy

.destination_above_source:
    mov rcx, rdi
    sub rcx, r13
    cmp rcx, r14
    jb .append_overlap

.append_copy:
    mov rsi, r13
    mov rdx, r14
    call memory_copy
    jmp .append_store_length

.append_overlap:
    mov rsi, r13
    mov rdx, r14
    call memory_move

.append_store_length:
    mov [r12 + BUFFER_LENGTH], rbx
    mov rdx, rbx
    xor eax, eax
    jmp .return

.success_existing:
    mov rdx, r8
    xor eax, eax
    jmp .return

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    jmp .return

.no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    jmp .return

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW

.return:
    add rsp, 8
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; buffer_append_byte(buffer, byte)
; RDI=buffer*, low byte of RSI=value
buffer_append_byte:
    test rdi, rdi
    jz .invalid
    mov r8, [rdi + BUFFER_LENGTH]
    mov r9, [rdi + BUFFER_CAPACITY]
    cmp r8, r9
    ja .invalid
    cmp r8, r9
    je .no_space
    mov r10, [rdi + BUFFER_DATA]
    test r10, r10
    jz .invalid
    add r10, r8
    jc .overflow
    mov [r10], sil
    inc r8
    mov [rdi + BUFFER_LENGTH], r8
    mov rdx, r8
    xor eax, eax
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret
.no_space:
    xor edx, edx
    mov rax, ERR_ENOSPC
    ret
.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret

; buffer_consume(buffer, count)
; Removes count bytes from the front, compacting remaining bytes.
; RDI=buffer*, RSI=count
buffer_consume:
    push rbx
    push r12
    sub rsp, 8                    ; 16-byte call alignment

    test rdi, rdi
    jz .invalid

    mov r12, rdi
    mov r8, [r12 + BUFFER_LENGTH]
    mov r9, [r12 + BUFFER_CAPACITY]
    cmp r8, r9
    ja .invalid
    cmp rsi, r8
    ja .invalid
    test rsi, rsi
    jz .success_existing

    mov rbx, r8
    sub rbx, rsi                  ; remaining length
    test rbx, rbx
    jz .set_zero

    mov r10, [r12 + BUFFER_DATA]
    test r10, r10
    jz .invalid

    mov r11, r10
    add r11, rsi                  ; source = data + count
    jc .overflow

    mov rdi, r10                  ; destination = data
    mov rsi, r11                  ; source
    mov rdx, rbx
    call memory_move

    mov [r12 + BUFFER_LENGTH], rbx
    mov rdx, rbx
    xor eax, eax
    jmp .return

.set_zero:
    mov qword [r12 + BUFFER_LENGTH], 0
    xor edx, edx
    xor eax, eax
    jmp .return

.success_existing:
    mov rdx, r8
    xor eax, eax
    jmp .return

.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    jmp .return

.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW

.return:
    add rsp, 8
    pop r12
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
