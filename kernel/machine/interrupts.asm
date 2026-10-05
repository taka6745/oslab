; Project-authored bytes from Intel SDM Vol. 2; existing interrupt ABI preserved.
; No registers omitted: normalized vector/error pair and full register frame.
bits 64
%include "kernel/machine/branch.inc"
section .text
extern interrupt_dispatch
global isr_table
%assign v 0
%rep 48
isr_%+v:
%if v != 8 && v != 10 && v != 11 && v != 12 && v != 13 && v != 14 && v != 17 && v != 21 && v != 29 && v != 30
    db 0x6a,0                       ; push qword 0 (sign-extended imm8)
%endif
    db 0x6a,v                       ; push qword vector
    ; Identical branch layout to reference: first 26 near, final 22 short.
    ; Each final stub is <=126 bytes from common. NASM overflow warnings are
    ; combined with explicit signed checks, preventing truncated displacements.
%if v < 26
    db 0xe9
    dd interrupt_common-($+4)
%else
    MACHINE_REL8 0xeb,interrupt_common
%endif
%assign v v+1
%endrep
interrupt_common:
    db 0xfc                         ; cld
    db 0x50,0x53,0x51,0x52,0x55,0x56,0x57 ; rax,rbx,rcx,rdx,rbp,rsi,rdi
    db 0x41,0x50,0x41,0x51,0x41,0x52,0x41,0x53 ; r8..r11
    db 0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57 ; r12..r15
    db 0x48,0x89,0xe7               ; mov rdi,rsp (frame)
    db 0x48,0x89,0xe3               ; mov rbx,rsp (callee-preserved original)
    db 0x48,0x83,0xe4,0xf0          ; and rsp,-16
    db 0xe8
    dd interrupt_dispatch-($+4)
    db 0x48,0x89,0xdc               ; mov rsp,rbx
    db 0x41,0x5f,0x41,0x5e,0x41,0x5d,0x41,0x5c ; r15..r12
    db 0x41,0x5b,0x41,0x5a,0x41,0x59,0x41,0x58 ; r11..r8
    db 0x5f,0x5e,0x5d,0x5a,0x59,0x5b,0x58 ; rdi,rsi,rbp,rdx,rcx,rbx,rax
    db 0x48,0x83,0xc4,0x10          ; add rsp,16 (vector,error)
    db 0x48,0xcf                    ; iretq
MACHINE_BRANCH_FINISH
section .rodata
isr_table:
%assign v 0
%rep 48
    dd isr_%+v                     ; zero-extended low kernel address
%assign v v+1
%endrep
section .note.GNU-stack noalloc noexec nowrite progbits
