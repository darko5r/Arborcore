; Arborcore parameterized route-pattern engine
;
; Route record layout matches the exact-router 40-byte record:
;   +0  method pointer
;   +8  method length
;   +16 pattern pointer
;   +24 pattern length
;   +32 handler pointer
;
; Parameter record layout (32 bytes):
;   +0  parameter name pointer (without ':')
;   +8  parameter name length
;   +16 matched value pointer
;   +24 matched value length
;
; route_pattern_match(pattern, pattern_len, path, path_len,
;                     params_out, params_capacity)
;   -> RAX=1 match, 0 no-match, negative errno
;      RDX=parameter count only when RAX=1; RDX=0 otherwise.
;
; Parameter-output contract:
;   - PARAM records are valid only when RAX=1. A failed candidate may have
;     touched params_out while exploring earlier segments; callers must ignore
;     the storage whenever RAX!=1.
;   - PARAM_NAME_PTR aliases immutable route-pattern/catalog storage. It is
;     valid only while that catalog remains alive and unchanged.
;   - PARAM_VALUE_PTR aliases the matched request-target/path storage. It is
;     valid only for that request lifetime and is invalidated when the input
;     buffer is compacted, consumed, reset or reused.
;
; route_pattern_dispatch(routes, count, request, context,
;                        params_out, params_capacity)
;   -> handler-defined RAX or -ENOENT / other negative error.
;
; Handler ABI for patterned dispatch:
;   RDI=request* RSI=context* RDX=params* RCX=param_count
;
; Parameter names accept ASCII letters, digits and underscore.
; Parameter values must be non-empty path segments. Raw percent-encoded
; values remain spans; decoding belongs to the caller/handler.

%define ERR_ENOENT    -2
%define ERR_EINVAL   -22
%define ERR_ENOSPC   -28
%define ERR_EOVERFLOW -75

%define ROUTE_SIZE        40
%define ROUTE_METHOD_PTR  0
%define ROUTE_METHOD_LEN  8
%define ROUTE_PATTERN_PTR 16
%define ROUTE_PATTERN_LEN 24
%define ROUTE_HANDLER     32
%define ROUTE_COUNT_MAX 0x0666666666666666

%define REQ_METHOD_PTR   0
%define REQ_METHOD_LEN   8

%define TARGET_PATH_PTR   0
%define TARGET_PATH_LEN   8

%define ROUTE_PARAM_SIZE  32
%define PARAM_NAME_PTR    0
%define PARAM_NAME_LEN    8
%define PARAM_VALUE_PTR  16
%define PARAM_VALUE_LEN  24
%define PARAM_SIZE       ROUTE_PARAM_SIZE
%define PARAM_COUNT_MAX 0x07ffffffffffffff

global route_pattern_match:function
global route_pattern_dispatch:function

extern bytes_equal
extern request_target_from_request

section .text

; Six-argument SysV ABI.
route_pattern_match:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 48

    ; +0 params capacity, +8 pattern index, +16 path index,
    ; +24 param count, +32 pattern seg end, +40 path seg end
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx
    mov rbx, r8
    mov [rsp + 0], r9
    mov qword [rsp + 24], 0

    ; Parameter output span must itself be representable.
    test r9, r9
    jz .params_span_ok
    test rbx, rbx
    jz .invalid
    mov rax, PARAM_COUNT_MAX
    cmp r9, rax
    ja .overflow
    mov rax, r9
    shl rax, 5
    add rax, rbx
    jc .overflow
.params_span_ok:

    ; Representability and basic rooted path/pattern form.
    test r13, r13
    jz .no_match
    test r15, r15
    jz .no_match
    test r12, r12
    jz .invalid
    test r14, r14
    jz .invalid
    mov rax, r12
    add rax, r13
    jc .overflow
    mov rax, r14
    add rax, r15
    jc .overflow
    cmp byte [r12], '/'
    jne .invalid
    cmp byte [r14], '/'
    jne .no_match
    mov qword [rsp + 8], 1
    mov qword [rsp + 16], 1

.segment_loop:
    mov rax, [rsp + 8]
    cmp rax, r13
    jne .pattern_remaining
    mov rcx, [rsp + 16]
    cmp rcx, r15
    je .match
    jmp .no_match
.pattern_remaining:
    mov rcx, [rsp + 16]
    cmp rcx, r15
    jae .no_match

    ; Find pattern segment end.
    mov rdx, rax
.find_pattern_end:
    cmp rdx, r13
    jae .pattern_end
    cmp byte [r12 + rdx], '/'
    je .pattern_end
    inc rdx
    jmp .find_pattern_end
.pattern_end:
    mov [rsp + 32], rdx

    ; Find path segment end.
    mov r8, rcx
.find_path_end:
    cmp r8, r15
    jae .path_end
    cmp byte [r14 + r8], '/'
    je .path_end
    inc r8
    jmp .find_path_end
.path_end:
    mov [rsp + 40], r8

    ; Pattern segment begins with ':' => parameter.
    cmp rdx, rax
    je .static_segment
    cmp byte [r12 + rax], ':'
    jne .static_segment

    ; Parameter name must be non-empty and [A-Za-z0-9_]+.
    lea r9, [rax + 1]
    cmp r9, rdx
    jae .invalid
    mov r10, r9
.validate_name:
    cmp r10, rdx
    jae .name_valid
    movzx eax, byte [r12 + r10]
    cmp eax, '0'
    jb .name_upper
    cmp eax, '9'
    jbe .name_next
.name_upper:
    cmp eax, 'A'
    jb .name_lower
    cmp eax, 'Z'
    jbe .name_next
.name_lower:
    cmp eax, 'a'
    jb .name_underscore
    cmp eax, 'z'
    jbe .name_next
.name_underscore:
    cmp eax, '_'
    jne .invalid
.name_next:
    inc r10
    jmp .validate_name
.name_valid:

    mov rcx, [rsp + 16]
    mov r8, [rsp + 40]
    cmp r8, rcx
    je .no_match                  ; parameter value must be non-empty

    mov rax, [rsp + 24]
    cmp rax, [rsp + 0]
    jae .no_space
    test rbx, rbx
    jz .invalid
    mov r11, rax
    shl r11, 5                    ; *32
    lea r9, [r12 + r9]
    mov [rbx + r11 + PARAM_NAME_PTR], r9
    mov r10, [rsp + 32]
    sub r10, [rsp + 8]
    dec r10
    mov [rbx + r11 + PARAM_NAME_LEN], r10
    lea r10, [r14 + rcx]
    mov [rbx + r11 + PARAM_VALUE_PTR], r10
    mov r10, r8
    sub r10, rcx
    mov [rbx + r11 + PARAM_VALUE_LEN], r10
    inc qword [rsp + 24]
    jmp .advance

.static_segment:
    mov rsi, [rsp + 32]
    sub rsi, [rsp + 8]
    mov rcx, [rsp + 40]
    sub rcx, [rsp + 16]
    cmp rsi, rcx
    jne .no_match
    mov rdi, [rsp + 8]
    add rdi, r12
    mov rdx, [rsp + 16]
    add rdx, r14
    call bytes_equal
    cmp eax, 1
    jne .no_match

.advance:
    ; Slash structure is significant: /x and /x/ are distinct.
    mov rax, [rsp + 32]
    mov rcx, [rsp + 40]
    cmp rax, r13
    je .pattern_at_end
    cmp rcx, r15
    je .no_match
    inc rax
    inc rcx
    mov [rsp + 8], rax
    mov [rsp + 16], rcx
    jmp .segment_loop
.pattern_at_end:
    cmp rcx, r15
    jne .no_match
    mov [rsp + 8], rax
    mov [rsp + 16], rcx
    jmp .segment_loop

.match:
    mov rdx, [rsp + 24]
    mov eax, 1
    jmp .return
.no_match:
    xor eax, eax
    xor edx, edx
    jmp .return
.invalid:
    mov rax, ERR_EINVAL
    xor edx, edx
    jmp .return
.no_space:
    mov rax, ERR_ENOSPC
    xor edx, edx
    jmp .return
.overflow:
    mov rax, ERR_EOVERFLOW
    xor edx, edx
.return:
    add rsp, 48
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; route_pattern_dispatch(routes,count,request,context,params,capacity)
;
; Catalog-order semantics:
;   Routes are considered strictly in caller-supplied order and the first
;   valid method+pattern match wins. Static/exact patterns receive no implicit
;   priority over parameter patterns; precedence is therefore explicit in the
;   immutable catalog order. This is the reference behavior that prepared
;   routing experiments must preserve.
route_pattern_dispatch:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 48

    mov r12, rdi                 ; route cursor
    mov r13, rsi                 ; count
    mov r14, rdx                 ; request
    mov r15, rcx                 ; context
    mov rbx, r8                  ; params
    mov [rsp + 0], r9            ; params capacity

    test r14, r14
    jz .dispatch_invalid

    ; Request method span must be representable.
    mov rax, [r14 + REQ_METHOD_LEN]
    test rax, rax
    jz .dispatch_invalid
    mov rcx, [r14 + REQ_METHOD_PTR]
    test rcx, rcx
    jz .dispatch_invalid
    add rax, rcx
    jc .dispatch_overflow

    ; Decompose raw request target into local 32-byte target view.
    mov rdi, r14
    lea rsi, [rsp + 16]
    call request_target_from_request
    test rax, rax
    jnz .dispatch_return

    test r13, r13
    jz .not_found
    test r12, r12
    jz .not_found
    mov rax, ROUTE_COUNT_MAX
    cmp r13, rax
    ja .dispatch_overflow
    imul rax, r13, ROUTE_SIZE
    add rax, r12
    jc .dispatch_overflow

.route_loop:
    ; Method lengths first.
    mov rax, [r14 + REQ_METHOD_LEN]
    cmp [r12 + ROUTE_METHOD_LEN], rax
    jne .next
    mov rdi, [r12 + ROUTE_METHOD_PTR]
    mov rsi, [r12 + ROUTE_METHOD_LEN]
    test rsi, rsi
    jz .next
    test rdi, rdi
    jz .next
    mov rax, rdi
    add rax, rsi
    jc .next
    mov rdx, [r14 + REQ_METHOD_PTR]
    mov rcx, [r14 + REQ_METHOD_LEN]
    call bytes_equal
    cmp eax, 1
    jne .next

    mov rdi, [r12 + ROUTE_PATTERN_PTR]
    mov rsi, [r12 + ROUTE_PATTERN_LEN]
    mov rdx, [rsp + 16 + TARGET_PATH_PTR]
    mov rcx, [rsp + 16 + TARGET_PATH_LEN]
    mov r8, rbx
    mov r9, [rsp + 0]
    call route_pattern_match
    test rax, rax
    js .dispatch_return
    cmp eax, 1
    jne .next

    mov rcx, rdx                 ; param count for handler
    mov rax, [r12 + ROUTE_HANDLER]
    test rax, rax
    jz .next
    mov rdi, r14
    mov rsi, r15
    mov rdx, rbx
    call rax
    jmp .dispatch_return

.next:
    add r12, ROUTE_SIZE
    dec r13
    jnz .route_loop
.not_found:
    mov rax, ERR_ENOENT
    jmp .dispatch_return
.dispatch_invalid:
    mov rax, ERR_EINVAL
    jmp .dispatch_return
.dispatch_overflow:
    mov rax, ERR_EOVERFLOW
.dispatch_return:
    add rsp, 48
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
