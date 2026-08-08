; Arborcore process entry
;
; Responsibilities:
;   - provide the ELF process entry point
;   - invoke the first Arborcore runtime operation
;   - translate startup success/failure into a process exit status
;
; Exit status:
;   0 = startup succeeded
;   1 = startup failed


%define SYS_EXIT         60

%define STDOUT_FILENO     1

%define EXIT_FAILURE      1


global _start:function

extern write_all


section .rodata


message:
    db "Arborcore: Assembly core active", 10

message_length equ $ - message


section .text


_start:
    mov edi, STDOUT_FILENO
    lea rsi, [rel message]
    mov edx, message_length

    call write_all

    test rax, rax
    jnz .failure


.success:
    xor edi, edi
    jmp .exit


.failure:
    mov edi, EXIT_FAILURE


.exit:
    mov eax, SYS_EXIT
    syscall


section .note.GNU-stack noalloc noexec nowrite progbits
