; Arborcore write_all tests
;
; Exit status:
;   0 = all tests passed
;   1 = zero-length behavior failed
;   2 = invalid-descriptor behavior failed


%define SYS_EXIT       60

%define EBADF           9

%define INVALID_FD     -1


global _start

extern write_all


section .rodata


one_byte:
    db "X"


section .text


_start:

    ; --------------------------------------------------------
    ; Test 1:
    ;
    ; Zero bytes must succeed without touching the buffer
    ; or attempting the invalid descriptor.
    ; --------------------------------------------------------

    mov edi, INVALID_FD
    xor esi, esi
    xor edx, edx

    call write_all

    test rax, rax
    jnz .fail_zero


    ; --------------------------------------------------------
    ; Test 2:
    ;
    ; A real write to an invalid descriptor must propagate
    ; the raw Linux -EBADF result.
    ; --------------------------------------------------------

    mov edi, INVALID_FD
    lea rsi, [rel one_byte]
    mov edx, 1

    call write_all

    cmp rax, -EBADF
    jne .fail_ebadf


.success:
    xor edi, edi
    jmp .exit


.fail_zero:
    mov edi, 1
    jmp .exit


.fail_ebadf:
    mov edi, 2


.exit:
    mov eax, SYS_EXIT
    syscall


section .note.GNU-stack noalloc noexec nowrite progbits
