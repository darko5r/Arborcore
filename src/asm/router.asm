; Arborcore exact-route lookup + request dispatch foundation
;
; Route record layout (40 bytes):
;   +0   method pointer
;   +8   method length
;   +16  path pointer
;   +24  path length
;   +32  handler function pointer
;
; Handler ABI:
;   RDI = parsed HTTP request structure
;   RSI = caller context pointer
;   RAX = handler-defined result/status
;
; router_find_exact(routes, count, method, method_len, path, path_len)
;   -> RAX = handler pointer or 0
;
; router_dispatch(routes, count, request, context)
;   -> handler RAX, or -2 (ENOENT) if no exact route matches.
;
; The router reuses the byte-span equality primitive and rejects
; representationally impossible route-table/input spans before reads.
; Catalog order is authoritative: the first valid exact method+path match
; with a non-NULL handler wins. Duplicate exact routes therefore preserve
; first-match semantics. Prepared/static index experiments must return the
; same handler for every valid lookup or be rejected.

%define ERR_ENOENT -2

%define ROUTE_METHOD_PTR  0
%define ROUTE_METHOD_LEN  8
%define ROUTE_PATH_PTR   16
%define ROUTE_PATH_LEN   24
%define ROUTE_HANDLER    32
%define ROUTE_SIZE       40

%define REQ_METHOD_PTR    0
%define REQ_METHOD_LEN    8
%define REQ_TARGET_PTR   16
%define REQ_TARGET_LEN   24

; floor(UINT64_MAX / 40)
%define ROUTE_COUNT_MAX 0x0666666666666666

extern bytes_equal

global router_find_exact:function
global router_dispatch:function

section .text

router_find_exact:
    ; Keep caller-visible span inputs stable across bytes_equal calls.
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 16

    mov rbx, rdi                  ; current route record
    mov r12, rsi                  ; routes remaining
    mov r13, rdx                  ; request method pointer
    mov r14, rcx                  ; request method length
    mov r15, r8                   ; request path pointer
    mov [rsp], r9                 ; request path length

    test r12, r12
    jz .not_found
    test rbx, rbx
    jz .not_found

    ; The route table must not wrap while stepping count * 40 bytes.
    mov rax, ROUTE_COUNT_MAX
    cmp r12, rax
    ja .not_found
    imul rax, r12, ROUTE_SIZE
    add rax, rbx
    jc .not_found
    mov [rsp + 8], rax            ; representable one-past-end

    ; Validate requested method span representation.
    test r14, r14
    jz .method_input_ok
    test r13, r13
    jz .not_found
    mov rax, r13
    add rax, r14
    jc .not_found
.method_input_ok:

    ; Validate requested path span representation.
    mov rax, [rsp]
    test rax, rax
    jz .path_input_ok
    test r15, r15
    jz .not_found
    add rax, r15
    jc .not_found
.path_input_ok:

.route_loop:
    cmp [rbx + ROUTE_METHOD_LEN], r14
    jne .next_route

    mov rax, [rsp]
    cmp [rbx + ROUTE_PATH_LEN], rax
    jne .next_route

    ; Route method span must itself be representable.
    mov rdi, [rbx + ROUTE_METHOD_PTR]
    mov rsi, [rbx + ROUTE_METHOD_LEN]
    test rsi, rsi
    jz .route_method_ok
    test rdi, rdi
    jz .next_route
    mov rax, rdi
    add rax, rsi
    jc .next_route
.route_method_ok:
    mov rdx, r13
    mov rcx, r14
    call bytes_equal
    cmp eax, 1
    jne .next_route

    ; Route path span must itself be representable.
    mov rdi, [rbx + ROUTE_PATH_PTR]
    mov rsi, [rbx + ROUTE_PATH_LEN]
    test rsi, rsi
    jz .route_path_ok
    test rdi, rdi
    jz .next_route
    mov rax, rdi
    add rax, rsi
    jc .next_route
.route_path_ok:
    mov rdx, r15
    mov rcx, [rsp]
    call bytes_equal
    cmp eax, 1
    jne .next_route

    mov rax, [rbx + ROUTE_HANDLER]
    test rax, rax
    jz .next_route
    jmp .return

.next_route:
    add rbx, ROUTE_SIZE
    dec r12
    jnz .route_loop

.not_found:
    xor eax, eax

.return:
    add rsp, 16
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

router_dispatch:
    ; Preserve request/context around lookup and handler call.
    push r12
    push r13
    push r14
    push r15
    sub rsp, 8                    ; keep 16-byte alignment before calls

    mov r12, rdi                  ; routes
    mov r13, rsi                  ; count
    mov r14, rdx                  ; request
    mov r15, rcx                  ; context

    test r14, r14
    jz .not_found

    mov rdi, r12
    mov rsi, r13
    mov rdx, [r14 + REQ_METHOD_PTR]
    mov rcx, [r14 + REQ_METHOD_LEN]
    mov r8,  [r14 + REQ_TARGET_PTR]
    mov r9,  [r14 + REQ_TARGET_LEN]
    call router_find_exact
    test rax, rax
    jz .not_found

    mov rdi, r14
    mov rsi, r15
    call rax
    jmp .dispatch_return

.not_found:
    mov rax, ERR_ENOENT

.dispatch_return:
    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
