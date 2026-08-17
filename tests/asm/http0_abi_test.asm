; Arborcore HTTP0 real NASM ABI qualification helpers.
; Linux x86-64 System V AMD64.

section .text

global http0_asm_call_field_validate:function
global http0_asm_call_response_validate:function
global http0_asm_call_connection_close:function

extern arbor_http_field_validate
extern arbor_http_response_validate
extern arbor_http_request_connection_close

; int64_t http0_asm_call_field_validate(const arbor_http_field *field)
; Returns the arbor_status.native lane from the C aggregate return.
http0_asm_call_field_validate:
    sub rsp, 8
    call arbor_http_field_validate
    add rsp, 8
    mov rax, rdx
    ret

; int64_t http0_asm_call_response_validate(const arbor_http_response *response)
http0_asm_call_response_validate:
    sub rsp, 8
    call arbor_http_response_validate
    add rsp, 8
    mov rax, rdx
    ret

; int64_t http0_asm_call_connection_close(request*, bool*)
http0_asm_call_connection_close:
    sub rsp, 8
    call arbor_http_request_connection_close
    add rsp, 8
    mov rax, rdx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
