; Arborcore foundational memory-engine test
;
; Exit status:
;
;   0 = all tests passed
;   1 = memory_copy family failed
;   2 = memory_set failed
;   3 = memory_zero failed
;   4 = memory_compare failed
;   5 = memory_find_byte failed
;   6 = memory_move failed


%define SYS_EXIT         60

%define BUFFER_SIZE     256

%define DEST_SENTINEL  0xCC


global _start


extern memory_copy
extern memory_copy_scalar
extern memory_copy_qword
extern memory_copy_rep

extern memory_set
extern memory_zero
extern memory_compare
extern memory_find_byte
extern memory_move


section .rodata


; ============================================================
; memory_copy test configuration
; ============================================================

copy_lengths:
    dq 0
    dq 1
    dq 7
    dq 8
    dq 9
    dq 31
    dq 32
    dq 63
    dq 64
    dq 127
    dq 128
    dq 143
    dq 144
    dq 145
    dq 255
    dq 256

COPY_LENGTH_COUNT equ ($ - copy_lengths) / 8


copy_functions:
    dq memory_copy
    dq memory_copy_scalar
    dq memory_copy_qword
    dq memory_copy_rep

COPY_FUNCTION_COUNT equ ($ - copy_functions) / 8


; ============================================================
; memory_set expectations
; ============================================================

expected_set:
    db 0xCC
    times 5 db 0x5A
    db 0xCC
    db 0xCC


; ============================================================
; memory_zero expectations
; ============================================================

expected_zero:
    db 0xA5
    times 4 db 0x00
    times 3 db 0xA5


; ============================================================
; memory_compare sources
; ============================================================

compare_equal_a:
    db 0x00, 0x01, 0x7F, 0x80, 0xFF

compare_equal_b:
    db 0x00, 0x01, 0x7F, 0x80, 0xFF


compare_less_a:
    db 0x10, 0x20, 0x30

compare_less_b:
    db 0x10, 0x21, 0x00


compare_unsigned_a:
    db 0x80

compare_unsigned_b:
    db 0x7F


; ============================================================
; memory_find_byte source
; ============================================================

find_source:
    db 0x10, 0x20, 0x30, 0x20, 0xFF


; ============================================================
; memory_move expectations
; ============================================================

move_forward_expected:
    db "cdefghijij"

move_backward_expected:
    db "ababcdefgh"

move_same_expected:
    db "abcdefghij"

move_nonoverlap_expected:
    db "abcdefghabcdmnop"


section .data


; ============================================================
; memory_set / zero mutable buffers
; ============================================================

set_buffer:
    times 8 db 0xCC


zero_buffer:
    times 8 db 0xA5


; ============================================================
; independent memory_move buffers
; ============================================================

move_forward_buffer:
    db "abcdefghij"


move_backward_buffer:
    db "abcdefghij"


move_same_buffer:
    db "abcdefghij"


move_nonoverlap_buffer:
    db "abcdefghijklmnop"


section .bss


alignb 64

copy_source:
    resb BUFFER_SIZE


alignb 64

copy_destination:
    resb BUFFER_SIZE


section .text


_start:

    ; ========================================================
    ; Initialize copy source with:
    ;
    ;   00 01 02 ... FE FF
    ;
    ; A changing pattern catches ordering errors that a buffer
    ; containing one repeated value might hide.
    ; ========================================================

    lea rdi, [rel copy_source]

    xor eax, eax
    mov ecx, BUFFER_SIZE


.initialize_copy_source:
    mov [rdi], al

    inc al
    inc rdi

    dec ecx
    jnz .initialize_copy_source


    ; ========================================================
    ; Test every copy implementation at every boundary.
    ; ========================================================

    lea r12, [rel copy_functions]
    xor r13d, r13d


.copy_function_loop:
    cmp r13d, COPY_FUNCTION_COUNT
    jae .test_set

    mov rbx, [r12 + r13 * 8]

    lea r14, [rel copy_lengths]
    xor r15d, r15d


.copy_length_loop:
    cmp r15d, COPY_LENGTH_COUNT
    jae .next_copy_function

    mov rdi, rbx
    mov rsi, [r14 + r15 * 8]

    call run_copy_case

    test eax, eax
    jnz .fail_copy

    inc r15d
    jmp .copy_length_loop


.next_copy_function:
    inc r13d
    jmp .copy_function_loop


    ; ========================================================
    ; memory_set
    ; ========================================================

.test_set:

    ; Zero length must not dereference NULL.

    xor edi, edi
    mov esi, 0x5A
    xor edx, edx

    call memory_set

    test rax, rax
    jnz .fail_set


    ; Fill bytes 1..5 with 0x5A.

    lea rdi, [rel set_buffer + 1]
    mov esi, 0x5A
    mov edx, 5

    call memory_set

    lea rcx, [rel set_buffer + 1]

    cmp rax, rcx
    jne .fail_set


    lea rdi, [rel set_buffer]
    lea rsi, [rel expected_set]
    mov edx, 8

    call bytes_equal

    test eax, eax
    jnz .fail_set


    ; ========================================================
    ; memory_zero
    ; ========================================================

.test_zero:

    ; Zero bytes 1..4.

    lea rdi, [rel zero_buffer + 1]
    mov esi, 4

    call memory_zero

    lea rcx, [rel zero_buffer + 1]

    cmp rax, rcx
    jne .fail_zero


    lea rdi, [rel zero_buffer]
    lea rsi, [rel expected_zero]
    mov edx, 8

    call bytes_equal

    test eax, eax
    jnz .fail_zero


    ; Zero-length NULL operation.

    xor edi, edi
    xor esi, esi

    call memory_zero

    test rax, rax
    jnz .fail_zero


    ; ========================================================
    ; memory_compare
    ; ========================================================

.test_compare:

    ; Zero-length comparison must not dereference NULL.

    xor edi, edi
    xor esi, esi
    xor edx, edx

    call memory_compare

    test rax, rax
    jnz .fail_compare


    ; Equal.

    lea rdi, [rel compare_equal_a]
    lea rsi, [rel compare_equal_b]
    mov edx, 5

    call memory_compare

    test rax, rax
    jnz .fail_compare


    ; Less than.

    lea rdi, [rel compare_less_a]
    lea rsi, [rel compare_less_b]
    mov edx, 3

    call memory_compare

    cmp rax, -1
    jne .fail_compare


    ; Greater than.

    lea rdi, [rel compare_less_b]
    lea rsi, [rel compare_less_a]
    mov edx, 3

    call memory_compare

    cmp rax, 1
    jne .fail_compare


    ; Verify unsigned comparison:
    ;
    ; 0x80 > 0x7F.

    lea rdi, [rel compare_unsigned_a]
    lea rsi, [rel compare_unsigned_b]
    mov edx, 1

    call memory_compare

    cmp rax, 1
    jne .fail_compare


    ; ========================================================
    ; memory_find_byte
    ; ========================================================

.test_find:

    ; Zero length must not dereference NULL.

    xor edi, edi
    mov esi, 0x20
    xor edx, edx

    call memory_find_byte

    test rax, rax
    jnz .fail_find


    ; Find first 0x20.

    lea rdi, [rel find_source]
    mov esi, 0x20
    mov edx, 5

    call memory_find_byte

    lea rcx, [rel find_source + 1]

    cmp rax, rcx
    jne .fail_find


    ; Find final 0xFF.

    lea rdi, [rel find_source]
    mov esi, 0xFF
    mov edx, 5

    call memory_find_byte

    lea rcx, [rel find_source + 4]

    cmp rax, rcx
    jne .fail_find


    ; Missing byte must return NULL.

    lea rdi, [rel find_source]
    mov esi, 0x40
    mov edx, 5

    call memory_find_byte

    test rax, rax
    jnz .fail_find


    ; ========================================================
    ; memory_move
    ; ========================================================

.test_move:

    ; --------------------------------------------------------
    ; Zero length with NULL pointers.
    ; --------------------------------------------------------

    xor edi, edi
    xor esi, esi
    xor edx, edx

    call memory_move

    test rax, rax
    jnz .fail_move


    ; --------------------------------------------------------
    ; Same source/destination.
    ; --------------------------------------------------------

    lea rdi, [rel move_same_buffer]
    lea rsi, [rel move_same_buffer]
    mov edx, 10

    call memory_move

    lea rcx, [rel move_same_buffer]

    cmp rax, rcx
    jne .fail_move


    lea rdi, [rel move_same_buffer]
    lea rsi, [rel move_same_expected]
    mov edx, 10

    call bytes_equal

    test eax, eax
    jnz .fail_move


    ; --------------------------------------------------------
    ; Forward-safe overlap:
    ;
    ; original:
    ;   abcdefghij
    ;
    ; move:
    ;   source      = +2
    ;   destination = +0
    ;   length      = 8
    ;
    ; result:
    ;   cdefghijij
    ; --------------------------------------------------------

    lea rdi, [rel move_forward_buffer]
    lea rsi, [rel move_forward_buffer + 2]
    mov edx, 8

    call memory_move

    lea rcx, [rel move_forward_buffer]

    cmp rax, rcx
    jne .fail_move


    lea rdi, [rel move_forward_buffer]
    lea rsi, [rel move_forward_expected]
    mov edx, 10

    call bytes_equal

    test eax, eax
    jnz .fail_move


    ; --------------------------------------------------------
    ; Destructive-forward overlap:
    ;
    ; original:
    ;   abcdefghij
    ;
    ; move:
    ;   source      = +0
    ;   destination = +2
    ;   length      = 8
    ;
    ; correct result:
    ;   ababcdefgh
    ;
    ; This requires a backward copy.
    ; --------------------------------------------------------

    lea rdi, [rel move_backward_buffer + 2]
    lea rsi, [rel move_backward_buffer]
    mov edx, 8

    call memory_move

    lea rcx, [rel move_backward_buffer + 2]

    cmp rax, rcx
    jne .fail_move


    lea rdi, [rel move_backward_buffer]
    lea rsi, [rel move_backward_expected]
    mov edx, 10

    call bytes_equal

    test eax, eax
    jnz .fail_move


    ; --------------------------------------------------------
    ; Non-overlapping move.
    ;
    ; Copy "abcd" to bytes 8..11.
    ;
    ; abcdefghijklmnop
    ;          ↓
    ; abcdefghabcdmnop
    ; --------------------------------------------------------

    lea rdi, [rel move_nonoverlap_buffer + 8]
    lea rsi, [rel move_nonoverlap_buffer]
    mov edx, 4

    call memory_move

    lea rcx, [rel move_nonoverlap_buffer + 8]

    cmp rax, rcx
    jne .fail_move


    lea rdi, [rel move_nonoverlap_buffer]
    lea rsi, [rel move_nonoverlap_expected]
    mov edx, 16

    call bytes_equal

    test eax, eax
    jnz .fail_move


    ; ========================================================
    ; All foundational memory tests passed.
    ; ========================================================

.success:
    xor edi, edi
    jmp .exit


.fail_copy:
    mov edi, 1
    jmp .exit


.fail_set:
    mov edi, 2
    jmp .exit


.fail_zero:
    mov edi, 3
    jmp .exit


.fail_compare:
    mov edi, 4
    jmp .exit


.fail_find:
    mov edi, 5
    jmp .exit


.fail_move:
    mov edi, 6


.exit:
    mov eax, SYS_EXIT
    syscall


; ============================================================
; run_copy_case(function, length)
;
; Input:
;   RDI = function address
;   RSI = copy length
;
; Return:
;   EAX = 0 pass
;   EAX = 1 wrong return pointer
;   EAX = 2 incorrect copied data
;   EAX = 3 wrote beyond requested length
;
; 24 bytes preserve local state while maintaining correct
; stack alignment before the indirect function call.
; ============================================================

run_copy_case:
    sub rsp, 24

    mov [rsp], rdi
    mov [rsp + 8], rsi


    ; Fill destination manually with sentinel bytes.

    lea rdi, [rel copy_destination]

    mov ecx, BUFFER_SIZE
    mov al, DEST_SENTINEL


.fill_destination:
    mov [rdi], al

    inc rdi

    dec ecx
    jnz .fill_destination


    ; Execute implementation.

    lea rdi, [rel copy_destination]
    lea rsi, [rel copy_source]
    mov rdx, [rsp + 8]

    call qword [rsp]


    ; Return pointer must equal destination start.

    lea rcx, [rel copy_destination]

    cmp rax, rcx
    jne .bad_return


    ; Check every requested byte.

    lea r8, [rel copy_source]
    lea r9, [rel copy_destination]

    xor ecx, ecx


.check_copied:
    cmp rcx, [rsp + 8]
    jae .check_tail_setup

    mov dl, [r8 + rcx]

    cmp dl, [r9 + rcx]
    jne .bad_data

    inc rcx
    jmp .check_copied


    ; Everything after LENGTH must still contain 0xCC.

.check_tail_setup:
    mov rcx, [rsp + 8]


.check_tail:
    cmp rcx, BUFFER_SIZE
    jae .pass

    cmp byte [r9 + rcx], DEST_SENTINEL
    jne .overcopy

    inc rcx
    jmp .check_tail


.pass:
    xor eax, eax
    jmp .return


.bad_return:
    mov eax, 1
    jmp .return


.bad_data:
    mov eax, 2
    jmp .return


.overcopy:
    mov eax, 3


.return:
    add rsp, 24
    ret


; ============================================================
; bytes_equal(left, right, length)
;
; Test-only helper.
;
; Return:
;   EAX = 0 equal
;   EAX = 1 different
; ============================================================

bytes_equal:
    test rdx, rdx
    jz .equal


.loop:
    mov al, [rdi]

    cmp al, [rsi]
    jne .different

    inc rdi
    inc rsi

    dec rdx
    jnz .loop


.equal:
    xor eax, eax
    ret


.different:
    mov eax, 1
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
