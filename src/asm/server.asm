; Arborcore event-driven HTTP server lifecycle foundation
;
; This stage intentionally keeps the application-handler contract small:
; exact-router handlers return a supported HTTP status code and the server
; emits an empty-body HTTP/1.1 response. The standalone response engine is
; already body-capable; a richer response object is a later framework ABI.
;
; server_open_listener(sockaddr*, addrlen, backlog)
;   -> RAX=nonblocking TCP listener fd or negative errno
;
; server_create_epoll(listener_fd)
;   -> RAX=epoll fd with listener registered, or negative errno
;
; server_accept_connection(listener_fd, epfd, conn*, inbuf*, outbuf*, arena*)
;   -> RAX=0, RDX=accepted fd; or negative errno
;
; server_handle_http_once(conn*, request*, routes*, route_count, context*, epfd)
;   -> RAX=0 when one complete request/response lifecycle finishes
;      RAX=-EAGAIN when more read/write readiness is required
;      other negative errno on fatal failure
;      RDX=completed request count on success
;
; server_close_connection(epfd, conn*) -> RAX=0 or close/remove error

%define ERR_ENOENT      -2
%define ERR_EAGAIN     -11
%define ERR_EINVAL     -22
%define ERR_ENOSPC     -28
%define ERR_EOVERFLOW  -75
%define ERR_ECONNRESET -104

%define SOCK_NONBLOCK 0x800
%define SOCK_CLOEXEC  0x80000
%define EPOLL_CLOEXEC 0x80000

%define EPOLLIN      0x001
%define EPOLLOUT     0x004
%define EPOLLERR      0x008
%define EPOLLHUP      0x010
%define EPOLLRDHUP    0x2000

%define CONN_FD             0
%define CONN_STATE          8
%define CONN_INPUT_BUFFER  24
%define CONN_OUTPUT_BUFFER 32
%define CONN_ARENA         40
%define CONN_WRITE_BYTES   56
%define CONN_REQUEST_COUNT 64

%define CONN_ACCEPTED       1
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

extern net_socket_tcp4
extern net_bind
extern net_listen
extern net_accept4
extern net_shutdown
extern net_close
extern io_set_nonblocking
extern io_read_retry
extern io_write_retry
extern io_close
extern event_epoll_create
extern event_epoll_add
extern event_epoll_modify
extern event_epoll_remove
extern connection_init
extern connection_transition
extern connection_note_read
extern connection_note_write
extern connection_complete_request
extern connection_set_error
extern http_parse_request
extern route_pattern_dispatch
extern http_response_serialize
extern buffer_reset
extern buffer_consume
extern arena_reset

global server_open_listener:function
global server_create_epoll:function
global server_accept_connection:function
global server_handle_http_once:function
global server_close_connection:function

section .text

server_open_listener:
    push rbx
    push r12
    push r13
    push r14
    sub rsp, 8                    ; 16-byte call alignment
    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    call net_socket_tcp4
    test rax, rax
    js .return
    mov rbx, rax                  ; listener survives nested calls

    mov rdi, rbx
    call io_set_nonblocking
    test rax, rax
    js .close_error

    mov rdi, rbx
    mov rsi, r12
    mov rdx, r13
    call net_bind
    test rax, rax
    js .close_error

    mov rdi, rbx
    mov rsi, r14
    call net_listen
    test rax, rax
    js .close_error

    mov rax, rbx
    jmp .return
.close_error:
    mov r12, rax
    mov rdi, rbx
    call net_close
    mov rax, r12
.return:
    add rsp, 8
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

server_create_epoll:
    push rbx
    push r12
    sub rsp, 8                    ; 16-byte call alignment
    mov r12, rdi                 ; listener
    mov edi, EPOLL_CLOEXEC
    call event_epoll_create
    test rax, rax
    js .return
    mov rbx, rax                 ; epfd survives event_epoll_add
    mov rdi, rbx
    mov rsi, r12
    mov edx, EPOLLIN | EPOLLERR | EPOLLHUP
    mov rcx, r12                 ; data = listener fd
    call event_epoll_add
    test rax, rax
    js .close_epoll
    mov rax, rbx
    jmp .return
.close_epoll:
    mov r12, rax
    mov rdi, rbx
    call io_close
    mov rax, r12
.return:
    add rsp, 8
    pop r12
    pop rbx
    ret

; Six arguments: listener, epfd, conn, inbuf, outbuf, arena
server_accept_connection:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 16
    mov r12, rdi                 ; listener
    mov r13, rsi                 ; epfd
    mov r14, rdx                 ; conn
    mov r15, rcx                 ; inbuf
    mov rbx, r8                  ; outbuf
    mov [rsp + 0], r9            ; arena

    test r14, r14
    jz .invalid
    mov rdi, r12
    xor esi, esi
    xor edx, edx
    mov ecx, SOCK_NONBLOCK | SOCK_CLOEXEC
    call net_accept4
    test rax, rax
    js .return
    mov [rsp + 8], rax           ; accepted fd

    mov rdi, r14
    mov rsi, rax
    mov rdx, r15
    mov rcx, rbx
    mov r8, [rsp + 0]
    call connection_init
    test rax, rax
    js .close_accepted

    mov rdi, r14
    mov esi, CONN_READING
    call connection_transition
    test rax, rax
    js .close_accepted

    mov rdi, r13
    mov rsi, [rsp + 8]
    mov edx, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP
    mov rcx, r14                 ; event data = connection pointer
    call event_epoll_add
    test rax, rax
    js .close_accepted

    mov rdx, [rsp + 8]
    xor eax, eax
    jmp .return
.close_accepted:
    mov r12, rax
    mov rdi, [rsp + 8]
    call net_close
    mov rax, r12
    xor edx, edx
    jmp .return
.invalid:
    mov rax, ERR_EINVAL
    xor edx, edx
.return:
    add rsp, 16
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; conn, request, routes, count, context, epfd
server_handle_http_once:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 288

    ; +0 context, +8 epfd, +16 status, +24 temporary result/count
    ; +32..+287 eight 32-byte route-parameter records
    mov r12, rdi                 ; conn
    mov r13, rsi                 ; request
    mov r14, rdx                 ; routes
    mov r15, rcx                 ; route count
    mov [rsp + 0], r8
    mov [rsp + 8], r9

    test r12, r12
    jz .invalid
    test r13, r13
    jz .invalid

    mov rax, [r12 + CONN_STATE]
    cmp rax, CONN_READING
    je .read_parse
    cmp rax, CONN_WRITING
    je .write_resume
    jmp .invalid

.read_parse:
    mov rbx, [r12 + CONN_INPUT_BUFFER]
    test rbx, rbx
    jz .invalid

.parse_existing:
    mov rsi, [rbx + BUFFER_LENGTH]
    test rsi, rsi
    jz .read_more
    mov rdi, [rbx + BUFFER_DATA]
    mov rdx, r13
    call http_parse_request
    test rax, rax
    jz .request_ready
    cmp rax, ERR_EAGAIN
    jne .fatal

.read_more:
    mov rax, [rbx + BUFFER_LENGTH]
    mov rcx, [rbx + BUFFER_CAPACITY]
    cmp rax, rcx
    ja .invalid
    cmp rax, rcx
    je .no_space
    mov rsi, [rbx + BUFFER_DATA]
    test rsi, rsi
    jz .invalid
    add rsi, rax
    jc .invalid
    mov rdx, rcx
    sub rdx, rax
    mov rdi, [r12 + CONN_FD]
    call io_read_retry
    test rax, rax
    js .read_error
    test rax, rax
    jz .peer_closed

    mov rcx, [rbx + BUFFER_LENGTH]
    add rcx, rax
    jc .fatal_overflow
    mov [rbx + BUFFER_LENGTH], rcx
    mov rsi, rax
    mov rdi, r12
    call connection_note_read
    test rax, rax
    js .fatal
    jmp .parse_existing

.read_error:
    cmp rax, ERR_EAGAIN
    je .again
    jmp .fatal

.request_ready:
    mov rdi, r12
    mov esi, CONN_REQUEST_READY
    call connection_transition
    test rax, rax
    js .fatal
    mov rdi, r12
    mov esi, CONN_DISPATCHING
    call connection_transition
    test rax, rax
    js .fatal

    mov rdi, r14
    mov rsi, r15
    mov rdx, r13
    mov rcx, [rsp + 0]
    lea r8, [rsp + 32]
    mov r9d, 8
    call route_pattern_dispatch
    cmp rax, ERR_ENOENT
    jne .handler_result
    mov eax, 404
.handler_result:
    test rax, rax
    js .fatal
    mov [rsp + 16], rax

    mov rbx, [r12 + CONN_OUTPUT_BUFFER]
    test rbx, rbx
    jz .invalid
    mov rdi, rbx
    call buffer_reset
    test rax, rax
    js .fatal

    mov rdi, rbx
    mov rsi, [rsp + 16]
    xor edx, edx
    xor ecx, ecx
    mov r8d, 1                   ; HTTP/1.1 keep-alive foundation
    call http_response_serialize
    test rax, rax
    js .fatal

    mov rdi, r12
    mov esi, CONN_WRITING
    call connection_transition
    test rax, rax
    js .fatal

    ; Switch interest to writable readiness while retaining error/hup.
    mov rdi, [rsp + 8]
    mov rsi, [r12 + CONN_FD]
    mov edx, EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLHUP
    mov rcx, r12
    call event_epoll_modify
    test rax, rax
    js .fatal

.write_resume:
    mov rbx, [r12 + CONN_OUTPUT_BUFFER]
    test rbx, rbx
    jz .invalid
    mov rax, [r12 + CONN_WRITE_BYTES]
    mov rcx, [rbx + BUFFER_LENGTH]
    cmp rax, rcx
    ja .invalid
    cmp rax, rcx
    je .write_complete
    mov rsi, [rbx + BUFFER_DATA]
    test rsi, rsi
    jz .invalid
    add rsi, rax
    jc .invalid
    mov rdx, rcx
    sub rdx, rax
    mov rdi, [r12 + CONN_FD]
    call io_write_retry
    test rax, rax
    js .write_error
    test rax, rax
    jz .again
    mov rsi, rax
    mov rdi, r12
    call connection_note_write
    test rax, rax
    js .fatal
    jmp .write_resume

.write_error:
    cmp rax, ERR_EAGAIN
    je .again
    jmp .fatal

.write_complete:
    ; Consume exactly the parsed request; preserve pipelined bytes.
    mov rdi, [r12 + CONN_INPUT_BUFFER]
    mov rsi, [r13 + REQ_MESSAGE_LENGTH]
    call buffer_consume
    test rax, rax
    js .fatal

    mov rdi, [r12 + CONN_OUTPUT_BUFFER]
    call buffer_reset
    test rax, rax
    js .fatal

    ; Request-lifetime arena is distinct from persistent connection buffers.
    mov rdi, [r12 + CONN_ARENA]
    call arena_reset
    test rax, rax
    js .fatal

    mov rdi, r12
    call connection_complete_request
    test rax, rax
    js .fatal
    mov [rsp + 24], rdx

    mov rdi, r12
    mov esi, CONN_KEEP_ALIVE
    call connection_transition
    test rax, rax
    js .fatal
    mov rdi, r12
    mov esi, CONN_READING
    call connection_transition
    test rax, rax
    js .fatal

    mov rdi, [rsp + 8]
    mov rsi, [r12 + CONN_FD]
    mov edx, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLHUP
    mov rcx, r12
    call event_epoll_modify
    test rax, rax
    js .fatal

    mov rdx, [rsp + 24]
    xor eax, eax
    jmp .return

.again:
    mov rax, ERR_EAGAIN
    xor edx, edx
    jmp .return
.no_space:
    mov rax, ERR_ENOSPC
    jmp .fatal
.peer_closed:
    mov rax, ERR_ECONNRESET
    jmp .fatal
.fatal_overflow:
    mov rax, ERR_EOVERFLOW
.fatal:
    mov rbx, rax
    mov rdi, r12
    mov rsi, rbx
    call connection_set_error
    mov rax, [r12 + CONN_STATE]
    cmp rax, CONN_CLOSING
    je .fatal_done
    cmp rax, CONN_CLOSED
    je .fatal_done
    mov rdi, r12
    mov esi, CONN_CLOSING
    call connection_transition
.fatal_done:
    mov rax, rbx
    xor edx, edx
    jmp .return
.invalid:
    mov rax, ERR_EINVAL
    xor edx, edx
.return:
    add rsp, 288
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; server_close_connection(epfd, conn*)
server_close_connection:
    push r12
    push r13
    sub rsp, 8
    mov r12, rdi
    mov r13, rsi
    test r13, r13
    jz .invalid
    mov rax, [r13 + CONN_STATE]
    cmp rax, CONN_CLOSED
    je .success
    cmp rax, CONN_CLOSING
    je .remove
    mov rdi, r13
    mov esi, CONN_CLOSING
    call connection_transition
    test rax, rax
    js .return
.remove:
    mov rdi, r12
    mov rsi, [r13 + CONN_FD]
    call event_epoll_remove
    ; Ignore DEL failure during teardown; close is authoritative.
    mov rdi, [r13 + CONN_FD]
    mov esi, 2
    call net_shutdown
    mov rdi, [r13 + CONN_FD]
    call net_close
    mov r12, rax
    mov rdi, r13
    mov esi, CONN_CLOSED
    call connection_transition
    test r12, r12
    js .close_result
.success:
    xor eax, eax
    xor edx, edx
    jmp .return
.close_result:
    mov rax, r12
    xor edx, edx
    jmp .return
.invalid:
    mov rax, ERR_EINVAL
    xor edx, edx
.return:
    add rsp, 8
    pop r13
    pop r12
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
