; Arborcore HTTP/1.1 Polish Gate #2 tests
; exit 0 pass, 1 fail

%define SYS_EXIT       60
%define ERR_EAGAIN    -11
%define ERR_EINVAL    -22
%define ERR_EOVERFLOW -75

%define REQ_METHOD_PTR       0
%define REQ_METHOD_LEN       8
%define REQ_TARGET_PTR      16
%define REQ_TARGET_LEN      24
%define REQ_VERSION_PTR     32
%define REQ_VERSION_LEN     40
%define REQ_HEADERS_PTR     48
%define REQ_HEADERS_LEN     56
%define REQ_BODY_PTR        64
%define REQ_BODY_AVAILABLE  72
%define REQ_CONTENT_LENGTH  80
%define REQ_MESSAGE_LENGTH  88

extern http_parse_request
extern bytes_equal

global _start

section .rodata

simple_request:
    db "GET /users HTTP/1.1",13,10
    db "Host: example.com",13,10
    db 13,10
simple_request_len equ $ - simple_request

post_request:
    db "POST /submit HTTP/1.1",13,10
    db "host: x",13,10
    db "content-length: 5",13,10
    db 13,10
    db "helloNEXT"
post_request_len equ $ - post_request
post_message_len equ post_request_len - 4

valid_token_request:
    db "M!X /token HTTP/1.1",13,10
    db "X!#$%&'*+-.^_`|~: ",9,"ok",0x80,13,10
    db 13,10
valid_token_request_len equ $ - valid_token_request

content_length_ows:
    db "POST /ows HTTP/1.1",13,10
    db "Content-Length:",9," 5 ",9,13,10
    db 13,10
    db "hello"
content_length_ows_len equ $ - content_length_ows

incomplete_body:
    db "POST /submit HTTP/1.1",13,10
    db "Content-Length: 5",13,10
    db 13,10
    db "hell"
incomplete_body_len equ $ - incomplete_body

incomplete_headers:
    db "GET / HTTP/1.1",13,10
    db "Host: x",13,10
incomplete_headers_len equ $ - incomplete_headers

trailing_cr:
    db "GET / HTTP/1.1",13
trailing_cr_len equ $ - trailing_cr

bare_lf:
    db "GET / HTTP/1.1",10,13,10
bare_lf_len equ $ - bare_lf

bare_cr:
    db "GET /bad",13,"x HTTP/1.1",13,10,13,10
bare_cr_len equ $ - bare_cr

bad_version:
    db "GET / HTTP/2.0",13,10,13,10
bad_version_len equ $ - bad_version

bad_method_paren:
    db "GE(T / HTTP/1.1",13,10,13,10
bad_method_paren_len equ $ - bad_method_paren

bad_method_at:
    db "GE@T / HTTP/1.1",13,10,13,10
bad_method_at_len equ $ - bad_method_at

missing_target:
    db "GET HTTP/1.1",13,10,13,10
missing_target_len equ $ - missing_target

empty_target:
    db "GET  HTTP/1.1",13,10,13,10
empty_target_len equ $ - empty_target

target_tab:
    db "GET /bad",9,"x HTTP/1.1",13,10,13,10
target_tab_len equ $ - target_tab

target_high:
    db "GET /",0x80," HTTP/1.1",13,10,13,10
target_high_len equ $ - target_high

bad_header_paren:
    db "GET / HTTP/1.1",13,10
    db "Bad(Header): x",13,10,13,10
bad_header_paren_len equ $ - bad_header_paren

bad_header_at:
    db "GET / HTTP/1.1",13,10
    db "Bad@Header: x",13,10,13,10
bad_header_at_len equ $ - bad_header_at

bad_header_comma:
    db "GET / HTTP/1.1",13,10
    db "Bad,Header: x",13,10,13,10
bad_header_comma_len equ $ - bad_header_comma

malformed_header:
    db "GET / HTTP/1.1",13,10
    db "Host example.com",13,10,13,10
malformed_header_len equ $ - malformed_header

spaced_transfer_name:
    db "POST / HTTP/1.1",13,10
    db "Transfer-Encoding : chunked",13,10,13,10
spaced_transfer_name_len equ $ - spaced_transfer_name

field_value_ctl:
    db "GET / HTTP/1.1",13,10
    db "X-Test: abc",0x01,"def",13,10,13,10
field_value_ctl_len equ $ - field_value_ctl

field_value_del:
    db "GET / HTTP/1.1",13,10
    db "X-Test: abc",0x7f,"def",13,10,13,10
field_value_del_len equ $ - field_value_del

duplicate_length:
    db "POST / HTTP/1.1",13,10
    db "Content-Length: 1",13,10
    db "content-length: 1",13,10
    db 13,10,"x"
duplicate_length_len equ $ - duplicate_length

transfer_encoding:
    db "POST / HTTP/1.1",13,10
    db "tRaNsFeR-EnCoDiNg: chunked",13,10,13,10
transfer_encoding_len equ $ - transfer_encoding

te_and_cl:
    db "POST / HTTP/1.1",13,10
    db "Content-Length: 0",13,10
    db "Transfer-Encoding: chunked",13,10,13,10
te_and_cl_len equ $ - te_and_cl

invalid_length:
    db "POST / HTTP/1.1",13,10
    db "Content-Length: x",13,10,13,10
invalid_length_len equ $ - invalid_length

plus_length:
    db "POST / HTTP/1.1",13,10
    db "Content-Length: +1",13,10,13,10
plus_length_len equ $ - plus_length

minus_length:
    db "POST / HTTP/1.1",13,10
    db "Content-Length: -1",13,10,13,10
minus_length_len equ $ - minus_length

spaced_length:
    db "POST / HTTP/1.1",13,10
    db "Content-Length: 1 0",13,10,13,10
spaced_length_len equ $ - spaced_length

overflow_length:
    db "POST / HTTP/1.1",13,10
    db "Content-Length: 18446744073709551616",13,10,13,10
overflow_length_len equ $ - overflow_length

max_length_no_body:
    db "POST / HTTP/1.1",13,10
    db "Content-Length: 18446744073709551615",13,10,13,10
max_length_no_body_len equ $ - max_length_no_body

method_get:    db "GET"
target_users:  db "/users"
method_post:   db "POST"
target_submit: db "/submit"
method_mix:    db "M!X"
target_token:  db "/token"

section .bss
align 16
request_out: resb 96

section .text
_start:
    ; Simple GET.
    lea rdi, [rel simple_request]
    mov esi, simple_request_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz http_test_fail

    mov rdi, [rel request_out + REQ_METHOD_PTR]
    mov rsi, [rel request_out + REQ_METHOD_LEN]
    lea rdx, [rel method_get]
    mov ecx, 3
    call bytes_equal
    cmp rax, 1
    jne http_test_fail

    mov rdi, [rel request_out + REQ_TARGET_PTR]
    mov rsi, [rel request_out + REQ_TARGET_LEN]
    lea rdx, [rel target_users]
    mov ecx, 6
    call bytes_equal
    cmp rax, 1
    jne http_test_fail

    cmp qword [rel request_out + REQ_VERSION_LEN], 8
    jne http_test_fail
    cmp qword [rel request_out + REQ_CONTENT_LENGTH], 0
    jne http_test_fail
    cmp qword [rel request_out + REQ_BODY_AVAILABLE], 0
    jne http_test_fail
    cmp qword [rel request_out + REQ_MESSAGE_LENGTH], simple_request_len
    jne http_test_fail

    ; POST framing leaves pipelined bytes available but consumes only body.
    lea rdi, [rel post_request]
    mov esi, post_request_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz http_test_fail

    mov rdi, [rel request_out + REQ_METHOD_PTR]
    mov rsi, [rel request_out + REQ_METHOD_LEN]
    lea rdx, [rel method_post]
    mov ecx, 4
    call bytes_equal
    cmp rax, 1
    jne http_test_fail

    mov rdi, [rel request_out + REQ_TARGET_PTR]
    mov rsi, [rel request_out + REQ_TARGET_LEN]
    lea rdx, [rel target_submit]
    mov ecx, 7
    call bytes_equal
    cmp rax, 1
    jne http_test_fail

    cmp qword [rel request_out + REQ_CONTENT_LENGTH], 5
    jne http_test_fail
    cmp qword [rel request_out + REQ_BODY_AVAILABLE], 9
    jne http_test_fail
    cmp qword [rel request_out + REQ_MESSAGE_LENGTH], post_message_len
    jne http_test_fail

    ; Legal token punctuation, HTAB and obs-text in an unknown field value.
    lea rdi, [rel valid_token_request]
    mov esi, valid_token_request_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz http_test_fail

    mov rdi, [rel request_out + REQ_METHOD_PTR]
    mov rsi, [rel request_out + REQ_METHOD_LEN]
    lea rdx, [rel method_mix]
    mov ecx, 3
    call bytes_equal
    cmp rax, 1
    jne http_test_fail

    mov rdi, [rel request_out + REQ_TARGET_PTR]
    mov rsi, [rel request_out + REQ_TARGET_LEN]
    lea rdx, [rel target_token]
    mov ecx, 6
    call bytes_equal
    cmp rax, 1
    jne http_test_fail

    ; Content-Length accepts surrounding HTTP OWS only.
    lea rdi, [rel content_length_ows]
    mov esi, content_length_ows_len
    lea rdx, [rel request_out]
    call http_parse_request
    test rax, rax
    jnz http_test_fail
    cmp qword [rel request_out + REQ_CONTENT_LENGTH], 5
    jne http_test_fail
    cmp qword [rel request_out + REQ_BODY_AVAILABLE], 5
    jne http_test_fail
    cmp qword [rel request_out + REQ_MESSAGE_LENGTH], content_length_ows_len
    jne http_test_fail

    ; Every failure path below must also zero all 96 output bytes.

    lea rdi, [rel incomplete_body]
    mov esi, incomplete_body_len
    mov rcx, ERR_EAGAIN
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel incomplete_headers]
    mov esi, incomplete_headers_len
    mov rcx, ERR_EAGAIN
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel trailing_cr]
    mov esi, trailing_cr_len
    mov rcx, ERR_EAGAIN
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel bare_lf]
    mov esi, bare_lf_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel bare_cr]
    mov esi, bare_cr_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel bad_version]
    mov esi, bad_version_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel bad_method_paren]
    mov esi, bad_method_paren_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel bad_method_at]
    mov esi, bad_method_at_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel missing_target]
    mov esi, missing_target_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel empty_target]
    mov esi, empty_target_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel target_tab]
    mov esi, target_tab_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel target_high]
    mov esi, target_high_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel bad_header_paren]
    mov esi, bad_header_paren_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel bad_header_at]
    mov esi, bad_header_at_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel bad_header_comma]
    mov esi, bad_header_comma_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel malformed_header]
    mov esi, malformed_header_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel spaced_transfer_name]
    mov esi, spaced_transfer_name_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel field_value_ctl]
    mov esi, field_value_ctl_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel field_value_del]
    mov esi, field_value_del_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel duplicate_length]
    mov esi, duplicate_length_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel transfer_encoding]
    mov esi, transfer_encoding_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel te_and_cl]
    mov esi, te_and_cl_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel invalid_length]
    mov esi, invalid_length_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel plus_length]
    mov esi, plus_length_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel minus_length]
    mov esi, minus_length_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel spaced_length]
    mov esi, spaced_length_len
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel overflow_length]
    mov esi, overflow_length_len
    mov rcx, ERR_EOVERFLOW
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    lea rdi, [rel max_length_no_body]
    mov esi, max_length_no_body_len
    mov rcx, ERR_EAGAIN
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    ; Empty input is incomplete and NULL-safe.
    xor edi, edi
    xor esi, esi
    mov rcx, ERR_EAGAIN
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    ; Nonzero NULL input is invalid.
    xor edi, edi
    mov esi, 1
    mov rcx, ERR_EINVAL
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    ; Wrapped input span is rejected before dereference.
    mov rdi, -8
    mov esi, 16
    mov rcx, ERR_EOVERFLOW
    call http_test_expect_failure
    cmp eax, 1
    jne http_test_fail

    ; NULL output structure is invalid.
    lea rdi, [rel simple_request]
    mov esi, simple_request_len
    xor edx, edx
    call http_parse_request
    cmp rax, ERR_EINVAL
    jne http_test_fail

    xor edi, edi
    jmp http_test_exit


; RDI=request pointer, RSI=length, RCX=expected status.
; Returns EAX=1 when status and fail-closed zero output both match.
http_test_expect_failure:
    push r12
    mov r12, rcx

    ; Seed the output with nonzero data to prove the parser clears it.
    lea r8, [rel request_out]
    xor ecx, ecx
http_test_seed_output:
    cmp ecx, 12
    jae http_test_call_failure
    mov qword [r8 + rcx*8], -1
    inc ecx
    jmp http_test_seed_output

http_test_call_failure:
    lea rdx, [rel request_out]
    call http_parse_request
    cmp rax, r12
    jne http_test_expect_failure_bad

    lea r8, [rel request_out]
    xor ecx, ecx
http_test_check_zero:
    cmp ecx, 12
    jae http_test_expect_failure_ok
    cmp qword [r8 + rcx*8], 0
    jne http_test_expect_failure_bad
    inc ecx
    jmp http_test_check_zero

http_test_expect_failure_ok:
    mov eax, 1
    pop r12
    ret

http_test_expect_failure_bad:
    xor eax, eax
    pop r12
    ret

http_test_fail:
    mov edi, 1

http_test_exit:
    mov eax, SYS_EXIT
    syscall

section .note.GNU-stack noalloc noexec nowrite progbits
