default rel

section .text

global af4_asm_transaction_begin
global af4_asm_transaction_commit
global af4_asm_transaction_rollback
global af4_asm_checkpoint_call

extern arbor_ddd_event_journal_checkpoint

; Test context layout:
; +0  begin_calls
; +8  commit_calls
; +16 rollback_calls
; +24 expected_state_size
; +32 expected_state
; +40 stack_misaligned
;
; Callback ABI:
; begin:    RDI=context, RSI=state, RDX=size, RAX=native status
; commit:   RDI=context, RSI=state, RAX=native status
; rollback: RDI=context, RSI=state, RAX=native status

af4_asm_transaction_begin:
    mov rax, rsp
    and eax, 15
    cmp eax, 8
    je af4_asm_transaction_begin_stack_ok
    inc qword [rdi + 40]
af4_asm_transaction_begin_stack_ok:
    inc qword [rdi + 0]
    cmp rsi, [rdi + 32]
    jne af4_asm_transaction_begin_invalid
    cmp rdx, [rdi + 24]
    jne af4_asm_transaction_begin_invalid
    test rdx, rdx
    jz af4_asm_transaction_begin_ok
    mov qword [rsi], 0x11223344
af4_asm_transaction_begin_ok:
    xor eax, eax
    ret
af4_asm_transaction_begin_invalid:
    mov rax, -22
    ret

af4_asm_transaction_commit:
    mov rax, rsp
    and eax, 15
    cmp eax, 8
    je af4_asm_transaction_commit_stack_ok
    inc qword [rdi + 40]
af4_asm_transaction_commit_stack_ok:
    inc qword [rdi + 8]
    cmp rsi, [rdi + 32]
    jne af4_asm_transaction_commit_invalid
    xor eax, eax
    ret
af4_asm_transaction_commit_invalid:
    mov rax, -22
    ret

af4_asm_transaction_rollback:
    mov rax, rsp
    and eax, 15
    cmp eax, 8
    je af4_asm_transaction_rollback_stack_ok
    inc qword [rdi + 40]
af4_asm_transaction_rollback_stack_ok:
    inc qword [rdi + 16]
    cmp rsi, [rdi + 32]
    jne af4_asm_transaction_rollback_invalid
    xor eax, eax
    ret
af4_asm_transaction_rollback_invalid:
    mov rax, -22
    ret

; arbor_status af4_asm_checkpoint_call(
;     const arbor_ddd_event_journal *journal,
;     arbor_ddd_event_checkpoint *out);
;
; arbor_status is returned by C in RAX:RDX under SysV AMD64. Preserve the
; pair unchanged after the call.
af4_asm_checkpoint_call:
    sub rsp, 8
    call arbor_ddd_event_journal_checkpoint
    add rsp, 8
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
