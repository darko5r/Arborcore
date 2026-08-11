; Arborcore Retrofit D4 experimental prepared exact-route index
;
; This is deliberately benchmark/qualification code, not production src/asm.
; It prepares an immutable power-of-two open-addressed table of route-record
; pointers while preserving catalog insertion order. Duplicate exact keys are
; inserted in catalog order; lookup therefore encounters the first valid
; duplicate first, matching router_find_exact semantics.
;
; route_index_prepare(routes, count, slots, slot_count)
;   RAX=0 success, negative errno on invalid/overflow/full table.
;
; route_index_find(slots, slot_count, method, method_len, path, path_len)
;   RAX=handler or 0 miss. slots must be an immutable successful prepare result.

%define ERR_EINVAL    -22
%define ERR_ENOSPC    -28
%define ERR_EOVERFLOW -75

%define ROUTE_METHOD_PTR  0
%define ROUTE_METHOD_LEN  8
%define ROUTE_PATH_PTR   16
%define ROUTE_PATH_LEN   24
%define ROUTE_HANDLER    32
%define ROUTE_SIZE       40
%define ROUTE_COUNT_MAX 0x0666666666666666
%define SLOT_COUNT_MAX  0x1fffffffffffffff

%define FNV_OFFSET 0xcbf29ce484222325
%define FNV_PRIME  0x00000100000001b3

extern bytes_equal

global route_index_prepare:function
global route_index_find:function

section .text

route_index_prepare:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 16

    mov r12, rdi                 ; route cursor
    mov r13, rsi                 ; route count
    mov r14, rdx                 ; slots
    mov r15, rcx                 ; slot count

    ; Empty catalog may use an empty index.
    test r13, r13
    jnz .need_index
    test r15, r15
    jz .success

.need_index:
    test r15, r15
    jz .invalid
    test r14, r14
    jz .invalid
    mov rax, SLOT_COUNT_MAX
    cmp r15, rax
    ja .overflow
    lea rax, [r15 - 1]
    test r15, rax                ; power-of-two slot count
    jnz .invalid
    mov rax, r15
    shl rax, 3
    add rax, r14
    jc .overflow

    ; Clear prepared table before any route insertion.
    xor eax, eax
    mov rcx, r15
    mov rdi, r14
    rep stosq

    test r13, r13
    jz .success
    test r12, r12
    jz .invalid
    mov rax, ROUTE_COUNT_MAX
    cmp r13, rax
    ja .overflow
    imul rax, r13, ROUTE_SIZE
    add rax, r12
    jc .overflow

.route_loop:
    mov rbx, r12

    ; Match router_find_exact treatment: unusable entries are skipped.
    mov rax, [rbx + ROUTE_HANDLER]
    test rax, rax
    jz .next_route

    mov rdi, [rbx + ROUTE_METHOD_PTR]
    mov rsi, [rbx + ROUTE_METHOD_LEN]
    test rsi, rsi
    jz .method_ok
    test rdi, rdi
    jz .next_route
    mov rax, rdi
    add rax, rsi
    jc .next_route
.method_ok:

    mov rdx, [rbx + ROUTE_PATH_PTR]
    mov rcx, [rbx + ROUTE_PATH_LEN]
    test rcx, rcx
    jz .path_ok
    test rdx, rdx
    jz .next_route
    mov rax, rdx
    add rax, rcx
    jc .next_route
.path_ok:

    call route_index_hash
    mov rcx, r15
    dec rcx
    and rax, rcx
    mov [rsp + 0], rax           ; current slot index
    mov [rsp + 8], r15           ; probes remaining

.probe_insert:
    mov rax, [rsp + 0]
    mov rdx, [r14 + rax * 8]
    test rdx, rdx
    jz .insert_here

    inc rax
    mov rcx, r15
    dec rcx
    and rax, rcx
    mov [rsp + 0], rax
    dec qword [rsp + 8]
    jnz .probe_insert
    jmp .no_space

.insert_here:
    mov [r14 + rax * 8], rbx

.next_route:
    add r12, ROUTE_SIZE
    dec r13
    jnz .route_loop

.success:
    xor eax, eax
    jmp .return
.invalid:
    mov rax, ERR_EINVAL
    jmp .return
.no_space:
    mov rax, ERR_ENOSPC
    jmp .return
.overflow:
    mov rax, ERR_EOVERFLOW
.return:
    add rsp, 16
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; Six-argument SysV ABI.
route_index_find:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 32

    mov r12, rdi                 ; slots
    mov r13, rsi                 ; slot count
    mov r14, rdx                 ; method ptr
    mov r15, rcx                 ; method len
    mov rbx, r8                  ; path ptr
    mov [rsp + 0], r9            ; path len

    test r13, r13
    jz .miss
    test r12, r12
    jz .miss
    lea rax, [r13 - 1]
    test r13, rax
    jnz .miss

    test r15, r15
    jz .request_method_ok
    test r14, r14
    jz .miss
    mov rax, r14
    add rax, r15
    jc .miss
.request_method_ok:

    mov rax, [rsp + 0]
    test rax, rax
    jz .request_path_ok
    test rbx, rbx
    jz .miss
    add rax, rbx
    jc .miss
.request_path_ok:

    mov rdi, r14
    mov rsi, r15
    mov rdx, rbx
    mov rcx, [rsp + 0]
    call route_index_hash
    mov rcx, r13
    dec rcx
    and rax, rcx
    mov [rsp + 8], rax           ; slot index
    mov [rsp + 16], r13          ; probes remaining

.probe_find:
    mov rax, [rsp + 8]
    mov r11, [r12 + rax * 8]
    test r11, r11
    jz .miss
    mov [rsp + 24], r11

    cmp [r11 + ROUTE_METHOD_LEN], r15
    jne .probe_next
    mov rcx, [rsp + 0]
    cmp [r11 + ROUTE_PATH_LEN], rcx
    jne .probe_next

    mov rdi, [r11 + ROUTE_METHOD_PTR]
    mov rsi, [r11 + ROUTE_METHOD_LEN]
    mov rdx, r14
    mov rcx, r15
    call bytes_equal
    cmp eax, 1
    jne .probe_next

    mov r11, [rsp + 24]
    mov rdi, [r11 + ROUTE_PATH_PTR]
    mov rsi, [r11 + ROUTE_PATH_LEN]
    mov rdx, rbx
    mov rcx, [rsp + 0]
    call bytes_equal
    cmp eax, 1
    jne .probe_next

    mov r11, [rsp + 24]
    mov rax, [r11 + ROUTE_HANDLER]
    jmp .return_find

.probe_next:
    mov rax, [rsp + 8]
    inc rax
    mov rcx, r13
    dec rcx
    and rax, rcx
    mov [rsp + 8], rax
    dec qword [rsp + 16]
    jnz .probe_find

.miss:
    xor eax, eax
.return_find:
    add rsp, 32
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; route_index_hash(method, method_len, path, path_len) -> RAX
route_index_hash:
    mov rax, FNV_OFFSET
    mov r8, FNV_PRIME

    test rsi, rsi
    jz .method_done
.method_loop:
    movzx r9d, byte [rdi]
    xor rax, r9
    imul rax, r8
    inc rdi
    dec rsi
    jnz .method_loop
.method_done:
    xor rax, 0xff
    imul rax, r8

    test rcx, rcx
    jz .hash_done
.path_loop:
    movzx r9d, byte [rdx]
    xor rax, r9
    imul rax, r8
    inc rdx
    dec rcx
    jnz .path_loop
.hash_done:
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
