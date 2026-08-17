; Arborcore MVC0 parallel rich Application transport.
;
; This is an MVC-layer Assembly component, not an Assembly ABI v1 extension.
; It deliberately mirrors the qualified server lifecycle while replacing the
; legacy status-only route dispatch with an application callback that serializes
; a complete response into the existing output buffer.
;
; application_transport_handle_once(conn, request, dispatch, context, epfd)
;   RDI=connection* RSI=request* RDX=dispatch RCX=context R8=epfd
; Returns RAX status / RDX total completed request count like server_handle_http_once.
;
; Dispatch callback:
;   RDI=request* RSI=output* RDX=arena* RCX=context R8=keep_alive_out*
;   RAX=0 success; negative mechanism failure; positive invalid.

%define ERR_EIO         -5
%define ERR_EAGAIN     -11
%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75
%define ERR_ECONNRESET -104

%define SERVER_MORE_WORK 1
%define SERVER_REQUEST_BUDGET 8

%define EPOLLIN      0x001
%define EPOLLOUT     0x004
%define EPOLLERR     0x008
%define EPOLLHUP     0x010
%define EPOLLRDHUP   0x2000

%define CONN_FD             0
%define CONN_STATE          8
%define CONN_FLAGS         16
%define CONN_INPUT_BUFFER  24
%define CONN_OUTPUT_BUFFER 32
%define CONN_ARENA         40
%define CONN_WRITE_BYTES   56
%define CONN_REQUEST_COUNT 64
%define CONN_FRAME_SCAN    80
%define CONN_FRAME_NEEDED  88

%define CONN_FLAG_HEADERS_READY 1
%define CONN_FLAG_WRITE_ARMED   2
%define CONN_FLAG_APP_KEEP_ALIVE 4

%define CONN_READING        2
%define CONN_REQUEST_READY  3
%define CONN_DISPATCHING    4
%define CONN_WRITING        5
%define CONN_KEEP_ALIVE     6
%define CONN_CLOSING        7
%define CONN_CLOSED         8

%define BUFFER_DATA      0
%define BUFFER_LENGTH    8
%define BUFFER_CAPACITY 16
%define REQ_MESSAGE_LENGTH 88

extern io_read_retry
extern io_write_retry
extern event_epoll_modify
extern connection_transition
extern connection_note_read
extern connection_note_write
extern connection_complete_request
extern connection_set_error
extern http_frame_scan
extern http_parse_request
extern buffer_reset
extern buffer_consume
extern arena_reset
extern server_close_connection

global application_transport_handle_once:function

section .text

application_transport_handle_once:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    ; +0 epfd, +8 request count at entry, +16 request budget,
    ; +24 write-was-armed, +32 keep-alive decision.
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx
    mov r15, rcx
    mov [rsp + 0], r8

    test r12, r12
    jz mvc0_transport_invalid
    test r13, r13
    jz mvc0_transport_invalid
    test r14, r14
    jz mvc0_transport_invalid

    mov rax, [r12 + CONN_REQUEST_COUNT]
    mov [rsp + 8], rax
    mov qword [rsp + 16], SERVER_REQUEST_BUDGET
    mov qword [rsp + 32], 0

    mov rax, [r12 + CONN_STATE]
    cmp rax, CONN_READING
    je mvc0_transport_read_parse
    cmp rax, CONN_WRITING
    je mvc0_transport_write_resume
    jmp mvc0_transport_invalid

mvc0_transport_read_parse:
    mov rbx, [r12 + CONN_INPUT_BUFFER]
    test rbx, rbx
    jz mvc0_transport_invalid

mvc0_transport_frame_dispatch:
    mov rcx, [rbx + BUFFER_LENGTH]
    mov rax, [r12 + CONN_FRAME_NEEDED]
    test rax, rax
    jz mvc0_transport_need_header_state
    cmp rcx, rax
    jb mvc0_transport_read_more
    jmp mvc0_transport_parse_existing

mvc0_transport_need_header_state:
    test qword [r12 + CONN_FLAGS], CONN_FLAG_HEADERS_READY
    jnz mvc0_transport_parse_existing
    test rcx, rcx
    jz mvc0_transport_read_more
    mov rdi, [rbx + BUFFER_DATA]
    mov rsi, rcx
    lea rdx, [r12 + CONN_FRAME_SCAN]
    call http_frame_scan
    cmp rax, 1
    je mvc0_transport_headers_ready
    cmp rax, ERR_EAGAIN
    je mvc0_transport_read_more
    jmp mvc0_transport_fatal

mvc0_transport_headers_ready:
    or qword [r12 + CONN_FLAGS], CONN_FLAG_HEADERS_READY

mvc0_transport_parse_existing:
    mov rsi, [rbx + BUFFER_LENGTH]
    mov rdi, [rbx + BUFFER_DATA]
    mov rdx, r13
    call http_parse_request
    test rax, rax
    jz mvc0_transport_request_ready
    cmp rax, ERR_EAGAIN
    jne mvc0_transport_fatal
    test rdx, rdx
    jz mvc0_transport_read_more
    mov [r12 + CONN_FRAME_NEEDED], rdx
    jmp mvc0_transport_read_more

mvc0_transport_read_more:
    mov rax, [rbx + BUFFER_LENGTH]
    mov rcx, [rbx + BUFFER_CAPACITY]
    cmp rax, rcx
    ja mvc0_transport_invalid
    je mvc0_transport_no_space
    mov rsi, [rbx + BUFFER_DATA]
    test rsi, rsi
    jz mvc0_transport_invalid
    add rsi, rax
    jc mvc0_transport_fatal_overflow
    mov rdx, rcx
    sub rdx, rax
    mov rdi, [r12 + CONN_FD]
    call io_read_retry
    test rax, rax
    js mvc0_transport_read_error
    test rax, rax
    jz mvc0_transport_peer_closed

    mov rcx, [rbx + BUFFER_LENGTH]
    add rcx, rax
    jc mvc0_transport_fatal_overflow
    mov [rbx + BUFFER_LENGTH], rcx
    mov rsi, rax
    mov rdi, r12
    call connection_note_read
    test rax, rax
    js mvc0_transport_fatal
    jmp mvc0_transport_frame_dispatch

mvc0_transport_read_error:
    cmp rax, ERR_EAGAIN
    je mvc0_transport_quiescent_or_again
    jmp mvc0_transport_fatal

mvc0_transport_request_ready:
    mov rdi, r12
    mov esi, CONN_REQUEST_READY
    call connection_transition
    test rax, rax
    js mvc0_transport_fatal
    mov rdi, r12
    mov esi, CONN_DISPATCHING
    call connection_transition
    test rax, rax
    js mvc0_transport_fatal

    ; Output is reset before the callback so serialization is transactional at
    ; the transport boundary. Request input + arena remain live until write end.
    mov rbx, [r12 + CONN_OUTPUT_BUFFER]
    test rbx, rbx
    jz mvc0_transport_invalid
    mov rdi, rbx
    call buffer_reset
    test rax, rax
    js mvc0_transport_fatal

    mov rax, [r12 + CONN_ARENA]
    test rax, rax
    jz mvc0_transport_invalid
    mov qword [rsp + 32], 2

    mov rdi, r13
    mov rsi, rbx
    mov rdx, rax
    mov rcx, r15
    lea r8, [rsp + 32]
    call r14
    test rax, rax
    js mvc0_transport_fatal
    jz mvc0_transport_callback_ok
    mov rax, ERR_EINVAL
    jmp mvc0_transport_fatal

mvc0_transport_callback_ok:
    cmp qword [rsp + 32], 1
    ja mvc0_transport_invalid
    cmp qword [rsp + 32], 0
    je mvc0_transport_mark_close
    or qword [r12 + CONN_FLAGS], CONN_FLAG_APP_KEEP_ALIVE
    jmp mvc0_transport_keepalive_marked
mvc0_transport_mark_close:
    and qword [r12 + CONN_FLAGS], -5
mvc0_transport_keepalive_marked:
    mov rax, [rbx + BUFFER_LENGTH]
    test rax, rax
    jz mvc0_transport_invalid
    cmp rax, [rbx + BUFFER_CAPACITY]
    ja mvc0_transport_invalid
    ; BUFFER_DATA is a pointer; test via register.
    mov rax, [rbx + BUFFER_DATA]
    test rax, rax
    jz mvc0_transport_invalid

    mov rdi, r12
    mov esi, CONN_WRITING
    call connection_transition
    test rax, rax
    js mvc0_transport_fatal
    jmp mvc0_transport_write_resume

mvc0_transport_write_resume:
    mov rbx, [r12 + CONN_OUTPUT_BUFFER]
    test rbx, rbx
    jz mvc0_transport_invalid
    mov rax, [r12 + CONN_WRITE_BYTES]
    mov rcx, [rbx + BUFFER_LENGTH]
    cmp rax, rcx
    ja mvc0_transport_invalid
    je mvc0_transport_write_complete
    mov rsi, [rbx + BUFFER_DATA]
    test rsi, rsi
    jz mvc0_transport_invalid
    add rsi, rax
    jc mvc0_transport_fatal_overflow
    mov rdx, rcx
    sub rdx, rax
    mov rdi, [r12 + CONN_FD]
    call io_write_retry
    test rax, rax
    js mvc0_transport_write_error
    test rax, rax
    jz mvc0_transport_write_zero
    mov rsi, rax
    mov rdi, r12
    call connection_note_write
    test rax, rax
    js mvc0_transport_fatal
    jmp mvc0_transport_write_resume

mvc0_transport_write_error:
    cmp rax, ERR_EAGAIN
    jne mvc0_transport_fatal
    jmp mvc0_transport_write_wait

mvc0_transport_write_zero:
    mov rax, ERR_EIO
    jmp mvc0_transport_fatal

mvc0_transport_write_wait:
    test qword [r12 + CONN_FLAGS], CONN_FLAG_WRITE_ARMED
    jnz mvc0_transport_again
    mov rdi, [rsp + 0]
    mov rsi, [r12 + CONN_FD]
    mov edx, EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLHUP
    mov rcx, r12
    call event_epoll_modify
    test rax, rax
    js mvc0_transport_fatal
    or qword [r12 + CONN_FLAGS], CONN_FLAG_WRITE_ARMED
    jmp mvc0_transport_again

mvc0_transport_write_complete:
    mov rax, [r12 + CONN_FLAGS]
    mov rcx, rax
    and eax, CONN_FLAG_WRITE_ARMED
    mov [rsp + 24], rax
    and ecx, CONN_FLAG_APP_KEEP_ALIVE
    shr ecx, 2
    mov [rsp + 32], rcx

    mov rdi, [r12 + CONN_INPUT_BUFFER]
    mov rsi, [r13 + REQ_MESSAGE_LENGTH]
    call buffer_consume
    test rax, rax
    js mvc0_transport_fatal

    mov rdi, [r12 + CONN_OUTPUT_BUFFER]
    call buffer_reset
    test rax, rax
    js mvc0_transport_fatal
    mov rdi, [r12 + CONN_ARENA]
    call arena_reset
    test rax, rax
    js mvc0_transport_fatal

    mov rdi, r12
    call connection_complete_request
    test rax, rax
    js mvc0_transport_fatal

    cmp qword [rsp + 32], 0
    je mvc0_transport_close_after_response

    mov rdi, r12
    mov esi, CONN_KEEP_ALIVE
    call connection_transition
    test rax, rax
    js mvc0_transport_fatal
    mov rdi, r12
    mov esi, CONN_READING
    call connection_transition
    test rax, rax
    js mvc0_transport_fatal

    cmp qword [rsp + 24], 0
    je mvc0_transport_interest_ready
    mov rdi, [rsp + 0]
    mov rsi, [r12 + CONN_FD]
    mov edx, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP
    mov rcx, r12
    call event_epoll_modify
    test rax, rax
    js mvc0_transport_fatal

mvc0_transport_interest_ready:
    dec qword [rsp + 16]
    jz mvc0_transport_more_work
    jmp mvc0_transport_read_parse

mvc0_transport_close_after_response:
    mov rdi, [rsp + 0]
    mov rsi, r12
    call server_close_connection
    test rax, rax
    js mvc0_transport_return
    mov rdx, [r12 + CONN_REQUEST_COUNT]
    xor eax, eax
    jmp mvc0_transport_return

mvc0_transport_more_work:
    mov rdx, [r12 + CONN_REQUEST_COUNT]
    mov eax, SERVER_MORE_WORK
    jmp mvc0_transport_return

mvc0_transport_quiescent_or_again:
    mov rdx, [r12 + CONN_REQUEST_COUNT]
    cmp rdx, [rsp + 8]
    jne mvc0_transport_success
mvc0_transport_again:
    mov rax, ERR_EAGAIN
    xor edx, edx
    jmp mvc0_transport_return

mvc0_transport_success:
    xor eax, eax
    jmp mvc0_transport_return

mvc0_transport_no_space:
    mov rax, ERR_ENOSPC
    jmp mvc0_transport_fatal

mvc0_transport_peer_closed:
    mov rax, ERR_ECONNRESET
    jmp mvc0_transport_fatal

mvc0_transport_fatal_overflow:
    mov rax, ERR_EOVERFLOW

mvc0_transport_fatal:
    mov rbx, rax
    mov rdi, r12
    mov rsi, rbx
    call connection_set_error
    mov rax, [r12 + CONN_STATE]
    cmp rax, CONN_CLOSING
    je mvc0_transport_fatal_done
    cmp rax, CONN_CLOSED
    je mvc0_transport_fatal_done
    mov rdi, r12
    mov esi, CONN_CLOSING
    call connection_transition
mvc0_transport_fatal_done:
    mov rax, rbx
    xor edx, edx
    jmp mvc0_transport_return

mvc0_transport_invalid:
    mov rax, ERR_EINVAL
    xor edx, edx

mvc0_transport_return:
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
