bits 64
section .text
extern interrupt_dispatch
global isr_table
%assign v 0
%rep 48
isr_%+v:
%if v != 8 && v != 10 && v != 11 && v != 12 && v != 13 && v != 14 && v != 17 && v != 21 && v != 29 && v != 30
    push qword 0
%endif
    push qword v
    jmp interrupt_common
%assign v v+1
%endrep
interrupt_common:
    cld
    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi,rsp
    mov rbx,rsp
    and rsp,-16
    call interrupt_dispatch
    mov rsp,rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp,16
    iretq
section .rodata
isr_table:
%assign v 0
%rep 48
    dq isr_%+v
%assign v v+1
%endrep
section .note.GNU-stack noalloc noexec nowrite progbits
