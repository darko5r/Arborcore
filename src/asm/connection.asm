; Arborcore connection state engine
;
; Connection layout (80 bytes):
;   +0   fd (signed qword)
;   +8   state
;   +16  flags
;   +24  input buffer pointer
;   +32  output buffer pointer
;   +40  request arena pointer
;   +48  bytes read during current request
;   +56  bytes written during current response
;   +64  completed request count
;   +72  last error/status
;
; State ABI:
;   1 ACCEPTED
;   2 READING
;   3 REQUEST_READY
;   4 DISPATCHING
;   5 WRITING
;   6 KEEP_ALIVE
;   7 CLOSING
;   8 CLOSED
;
; Status convention:
;   RAX=0 success; -EINVAL invalid transition/input; -EOVERFLOW counter wrap.
;   Counter-mutating operations return the resulting counter in RDX.

%define ERR_EINVAL     -22
%define ERR_EOVERFLOW  -75

%define CONN_FD             0
%define CONN_STATE          8
%define CONN_FLAGS         16
%define CONN_INPUT_BUFFER  24
%define CONN_OUTPUT_BUFFER 32
%define CONN_ARENA         40
%define CONN_READ_BYTES    48
%define CONN_WRITE_BYTES   56
%define CONN_REQUEST_COUNT 64
%define CONN_LAST_ERROR    72

%define CONN_ACCEPTED       1
%define CONN_READING        2
%define CONN_REQUEST_READY  3
%define CONN_DISPATCHING    4
%define CONN_WRITING        5
%define CONN_KEEP_ALIVE     6
%define CONN_CLOSING        7
%define CONN_CLOSED         8


global connection_init:function
global connection_state:function
global connection_transition:function
global connection_note_read:function
global connection_note_write:function
global connection_complete_request:function
global connection_set_error:function

section .text

; connection_init(conn, fd, input_buffer, output_buffer, arena)
; RDI=conn* RSI=fd RDX=inbuf* RCX=outbuf* R8=arena*
;
; Initializes connection metadata only.  The referenced buffer/arena
; objects remain caller-owned and must already satisfy their own
; invariants.  Server-side reuse/acceptance is responsible for preparing
; pristine request storage before publishing a connection.
connection_init:
    test rdi, rdi
    jz .invalid
    test rsi, rsi
    js .invalid
    test rdx, rdx
    jz .invalid
    test rcx, rcx
    jz .invalid
    test r8, r8
    jz .invalid

    mov [rdi + CONN_FD], rsi
    mov qword [rdi + CONN_STATE], CONN_ACCEPTED
    mov qword [rdi + CONN_FLAGS], 0
    mov [rdi + CONN_INPUT_BUFFER], rdx
    mov [rdi + CONN_OUTPUT_BUFFER], rcx
    mov [rdi + CONN_ARENA], r8
    mov qword [rdi + CONN_READ_BYTES], 0
    mov qword [rdi + CONN_WRITE_BYTES], 0
    mov qword [rdi + CONN_REQUEST_COUNT], 0
    mov qword [rdi + CONN_LAST_ERROR], 0
    xor eax, eax
    xor edx, edx
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

connection_state:
    test rdi, rdi
    jz .zero
    mov rax, [rdi + CONN_STATE]
    ret
.zero:
    xor eax, eax
    ret

; connection_transition(conn, new_state)
connection_transition:
    test rdi, rdi
    jz .invalid
    cmp rsi, CONN_ACCEPTED
    jb .invalid
    cmp rsi, CONN_CLOSED
    ja .invalid

    mov rax, [rdi + CONN_STATE]
    cmp rax, CONN_ACCEPTED
    je .from_accepted
    cmp rax, CONN_READING
    je .from_reading
    cmp rax, CONN_REQUEST_READY
    je .from_request_ready
    cmp rax, CONN_DISPATCHING
    je .from_dispatching
    cmp rax, CONN_WRITING
    je .from_writing
    cmp rax, CONN_KEEP_ALIVE
    je .from_keep_alive
    cmp rax, CONN_CLOSING
    je .from_closing
    jmp .invalid

.from_accepted:
    cmp rsi, CONN_READING
    je .store
    cmp rsi, CONN_CLOSING
    je .store
    jmp .invalid
.from_reading:
    cmp rsi, CONN_REQUEST_READY
    je .store
    cmp rsi, CONN_CLOSING
    je .store
    jmp .invalid
.from_request_ready:
    cmp rsi, CONN_DISPATCHING
    je .store
    cmp rsi, CONN_CLOSING
    je .store
    jmp .invalid
.from_dispatching:
    cmp rsi, CONN_WRITING
    je .store
    cmp rsi, CONN_CLOSING
    je .store
    jmp .invalid
.from_writing:
    cmp rsi, CONN_KEEP_ALIVE
    je .store
    cmp rsi, CONN_CLOSING
    je .store
    jmp .invalid
.from_keep_alive:
    cmp rsi, CONN_READING
    je .store
    cmp rsi, CONN_CLOSING
    je .store
    jmp .invalid
.from_closing:
    cmp rsi, CONN_CLOSED
    jne .invalid
.store:
    mov [rdi + CONN_STATE], rsi
    xor eax, eax
    xor edx, edx
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

connection_note_read:
    test rdi, rdi
    jz .invalid
    cmp qword [rdi + CONN_STATE], CONN_READING
    jne .invalid
    mov rax, [rdi + CONN_READ_BYTES]
    add rax, rsi
    jc .overflow
    mov [rdi + CONN_READ_BYTES], rax
    mov rdx, rax
    xor eax, eax
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret
.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret

connection_note_write:
    test rdi, rdi
    jz .invalid
    cmp qword [rdi + CONN_STATE], CONN_WRITING
    jne .invalid
    mov rax, [rdi + CONN_WRITE_BYTES]
    add rax, rsi
    jc .overflow
    mov [rdi + CONN_WRITE_BYTES], rax
    mov rdx, rax
    xor eax, eax
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret
.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret

; Valid while WRITING, immediately before transition to KEEP_ALIVE.
connection_complete_request:
    test rdi, rdi
    jz .invalid
    cmp qword [rdi + CONN_STATE], CONN_WRITING
    jne .invalid
    mov rax, [rdi + CONN_REQUEST_COUNT]
    inc rax
    jz .overflow
    mov [rdi + CONN_REQUEST_COUNT], rax
    mov qword [rdi + CONN_READ_BYTES], 0
    mov qword [rdi + CONN_WRITE_BYTES], 0
    mov rdx, rax
    xor eax, eax
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret
.overflow:
    xor edx, edx
    mov rax, ERR_EOVERFLOW
    ret

connection_set_error:
    test rdi, rdi
    jz .invalid
    mov [rdi + CONN_LAST_ERROR], rsi
    xor eax, eax
    xor edx, edx
    ret
.invalid:
    xor edx, edx
    mov rax, ERR_EINVAL
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
