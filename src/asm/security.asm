; Arborcore security-sensitive memory primitives
;
; These routines are deliberately separate from ordinary memory_zero and
; memory_compare so normal hot paths retain their existing semantics.
;
; memory_secure_clear guarantees an architectural overwrite of every byte in
; the caller-supplied span when length > 0.  It does not claim to erase copies
; held elsewhere, CPU caches, swap, snapshots or device-level remnants.
;
; memory_equal_constant_time performs content-independent equality over the
; supplied length: every byte is read and folded into the result.  Runtime may
; depend on the public length but not on the first differing byte position.


global memory_secure_clear:function
global memory_equal_constant_time:function


section .text


; memory_secure_clear(destination, length)
; RDI=destination RSI=length
; RAX=original destination
; Zero length performs no memory access.
memory_secure_clear:
    mov r8, rdi
    mov rcx, rsi
    xor eax, eax
    rep stosb
    mov rax, r8
    ret


; memory_equal_constant_time(left, right, length)
; RDI=left RSI=right RDX=length
; RAX=1 equal, RAX=0 different
; Zero length is equal and performs no memory access.
memory_equal_constant_time:
    xor eax, eax                    ; accumulated XOR difference
    test rdx, rdx
    jz .finish
.loop:
    movzx ecx, byte [rdi]
    movzx r8d, byte [rsi]
    xor ecx, r8d
    or eax, ecx
    inc rdi
    inc rsi
    dec rdx
    jnz .loop
.finish:
    test eax, eax
    sete al
    movzx eax, al
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
