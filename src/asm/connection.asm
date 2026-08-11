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

section .rodata

; Rows: current state ACCEPTED..CLOSED.
; Bits: destination state ACCEPTED..CLOSED.
connection_transition_masks:
    db (1 << (CONN_READING       - CONN_ACCEPTED)) | (1 << (CONN_CLOSING - CONN_ACCEPTED))
    db (1 << (CONN_REQUEST_READY - CONN_ACCEPTED)) | (1 << (CONN_CLOSING - CONN_ACCEPTED))
    db (1 << (CONN_DISPATCHING   - CONN_ACCEPTED)) | (1 << (CONN_CLOSING - CONN_ACCEPTED))
    db (1 << (CONN_WRITING       - CONN_ACCEPTED)) | (1 << (CONN_CLOSING - CONN_ACCEPTED))
    db (1 << (CONN_KEEP_ALIVE    - CONN_ACCEPTED)) | (1 << (CONN_CLOSING - CONN_ACCEPTED))
    db (1 << (CONN_READING       - CONN_ACCEPTED)) | (1 << (CONN_CLOSING - CONN_ACCEPTED))
    db (1 << (CONN_CLOSED        - CONN_ACCEPTED))
    db 0

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
;
; The transition relation is encoded directly as an eight-byte table.
; Each table byte corresponds to current state (state-1); bit
; (new_state-1) is set exactly when the edge is legal.
connection_transition:
    test rdi, rdi
    jz .invalid

    lea rax, [rsi - CONN_ACCEPTED]
    cmp rax, CONN_CLOSED - CONN_ACCEPTED
    ja .invalid

    mov rcx, [rdi + CONN_STATE]
    lea rax, [rcx - CONN_ACCEPTED]
    cmp rax, CONN_CLOSED - CONN_ACCEPTED
    ja .invalid

    lea r8, [rel connection_transition_masks]
    movzx eax, byte [r8 + rax]

    mov ecx, esi
    sub ecx, CONN_ACCEPTED
    bt eax, ecx
    jnc .invalid

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
