; HTTP1 C API SysV ABI consumer. Test-only; no production Assembly ABI extension.
default rel
section .text

global http1_asm_call_measure:function
extern arbor_http_mvc_application_measure

; arbor_status http1_asm_call_measure(uint64_t capacity, requirements *out)
; The C return aggregate fits RAX/RDX under SysV and is forwarded unchanged.
http1_asm_call_measure:
    sub rsp, 8
    call arbor_http_mvc_application_measure
    add rsp, 8
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
