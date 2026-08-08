; Arborcore memory-threshold policy test
;
; Exit status:
;   0 = valid
;   1 = invalid qword threshold
;   2 = invalid REP threshold
;   3 = invalid policy version


%define SYS_EXIT 60


global _start


extern memory_threshold_policy_version
extern memory_threshold_qword_min
extern memory_threshold_rep_min


section .text


_start:

    ; Policy version must currently be 1.

    cmp qword [rel memory_threshold_policy_version], 1
    jne .fail_version


    ; Qword threshold must be nonzero.

    mov rax, [rel memory_threshold_qword_min]

    test rax, rax
    jz .fail_qword


    ; REP threshold must be strictly above qword threshold.

    mov rcx, [rel memory_threshold_rep_min]

    cmp rcx, rax
    jbe .fail_rep


.success:
    xor edi, edi
    jmp .exit


.fail_qword:
    mov edi, 1
    jmp .exit


.fail_rep:
    mov edi, 2
    jmp .exit


.fail_version:
    mov edi, 3


.exit:
    mov eax, SYS_EXIT
    syscall


section .note.GNU-stack noalloc noexec nowrite progbits
