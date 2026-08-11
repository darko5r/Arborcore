; Arborcore server benchmark support
;
; bench_now_ns() -> RAX = CLOCK_MONOTONIC_RAW nanoseconds, or negative errno.
;
; bench_emit_result(name, name_len, iterations, total_ns)
;   RDI=name, RSI=name_len, RDX=iterations, RCX=total_ns
;   emits one tab-separated result line:
;       <name>\t<iterations>\t<total_ns>\n
;   returns RAX=0 or a negative error from the output helpers.

%define SYS_CLOCK_GETTIME 228
%define CLOCK_MONOTONIC_RAW 4
%define STDOUT_FILENO 1

global bench_now_ns:function
global bench_emit_result:function

extern u64_format_decimal
extern write_all

section .rodata
bench_tab: db 9
bench_nl:  db 10

section .text

bench_now_ns:
    sub rsp, 24
    mov edi, CLOCK_MONOTONIC_RAW
    lea rsi, [rsp]
    mov eax, SYS_CLOCK_GETTIME
    syscall
    test rax, rax
    js .return

    mov rax, [rsp]
    mov ecx, 1000000000
    mul rcx
    test rdx, rdx
    jnz .overflow
    add rax, [rsp + 8]
    jc .overflow
    jmp .return

.overflow:
    mov rax, -75
.return:
    add rsp, 24
    ret

bench_emit_result:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    mov r12, rdi                ; name
    mov r13, rsi                ; name length
    mov r14, rdx                ; iterations
    mov r15, rcx                ; total ns

    mov edi, STDOUT_FILENO
    mov rsi, r12
    mov rdx, r13
    call write_all
    test rax, rax
    js .done

    mov edi, STDOUT_FILENO
    lea rsi, [rel bench_tab]
    mov edx, 1
    call write_all
    test rax, rax
    js .done

    mov rdi, r14
    lea rsi, [rsp]
    mov edx, 32
    call u64_format_decimal
    test rax, rax
    js .done
    mov rbx, rdx

    mov edi, STDOUT_FILENO
    lea rsi, [rsp]
    mov rdx, rbx
    call write_all
    test rax, rax
    js .done

    mov edi, STDOUT_FILENO
    lea rsi, [rel bench_tab]
    mov edx, 1
    call write_all
    test rax, rax
    js .done

    mov rdi, r15
    lea rsi, [rsp + 32]
    mov edx, 32
    call u64_format_decimal
    test rax, rax
    js .done
    mov rbx, rdx

    mov edi, STDOUT_FILENO
    lea rsi, [rsp + 32]
    mov rdx, rbx
    call write_all
    test rax, rax
    js .done

    mov edi, STDOUT_FILENO
    lea rsi, [rel bench_nl]
    mov edx, 1
    call write_all

.done:
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
