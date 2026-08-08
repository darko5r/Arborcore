; Arborcore hot-buffer memory-copy benchmark
;
; Required assembly definition:
;
;   COPY_FUNCTION
;
; Optional:
;
;   COPY_SIZE
;   ITERATIONS
;
; Output:
;   8 raw bytes containing elapsed nanoseconds.


%ifndef COPY_FUNCTION
    %error "COPY_FUNCTION must be defined"
%endif

%ifndef COPY_SIZE
    %define COPY_SIZE 64
%endif

%ifndef ITERATIONS
    %define ITERATIONS 10000000
%endif


%define SYS_CLOCK_GETTIME  228
%define SYS_EXIT            60

%define CLOCK_MONOTONIC      1

%define STDOUT_FILENO        1


global _start

extern COPY_FUNCTION
extern write_all


section .bss


alignb 64

source:
    resb COPY_SIZE


alignb 64

destination:
    resb COPY_SIZE


alignb 8

start_time:
    resq 2


alignb 8

end_time:
    resq 2


alignb 8

elapsed_ns:
    resq 1


section .text


_start:

    ; Touch and initialize source before timing.

    lea rdi, [rel source]
    mov rcx, COPY_SIZE

    test rcx, rcx
    jz .warm_up


.initialize_source:
    mov byte [rdi], 0xA5

    inc rdi

    dec rcx
    jnz .initialize_source


.warm_up:
    lea rdi, [rel destination]
    lea rsi, [rel source]
    mov edx, COPY_SIZE

    call COPY_FUNCTION


    ; Starting monotonic timestamp.

    mov eax, SYS_CLOCK_GETTIME
    mov edi, CLOCK_MONOTONIC
    lea rsi, [rel start_time]

    syscall

    test rax, rax
    js .start_clock_failure


    mov r12, ITERATIONS


.benchmark_loop:
    lea rdi, [rel destination]
    lea rsi, [rel source]
    mov edx, COPY_SIZE

    call COPY_FUNCTION

    dec r12
    jnz .benchmark_loop


    ; Ending timestamp.

    mov eax, SYS_CLOCK_GETTIME
    mov edi, CLOCK_MONOTONIC
    lea rsi, [rel end_time]

    syscall

    test rax, rax
    js .end_clock_failure


    ; elapsed =
    ;
    ;   seconds_delta * 1,000,000,000
    ; + nanoseconds_delta

    mov rax, [rel end_time]
    sub rax, [rel start_time]

    imul rax, rax, 1000000000

    mov rcx, [rel end_time + 8]
    sub rcx, [rel start_time + 8]

    add rax, rcx

    mov [rel elapsed_ns], rax


    ; Emit binary uint64 result.

    mov edi, STDOUT_FILENO
    lea rsi, [rel elapsed_ns]
    mov edx, 8

    call write_all

    test rax, rax
    jnz .write_failure


.success:
    xor edi, edi
    jmp .exit


.start_clock_failure:
    mov edi, 1
    jmp .exit


.end_clock_failure:
    mov edi, 2
    jmp .exit


.write_failure:
    mov edi, 3


.exit:
    mov eax, SYS_EXIT
    syscall


section .note.GNU-stack noalloc noexec nowrite progbits
