bits 64
section .text.entry
global kernel_entry
extern __bss_start, __bss_end, kernel_main
kernel_entry:
    cld
    lea rdi,[rel __bss_start]
    lea rcx,[rel __bss_end]
    sub rcx,rdi
    xor eax,eax
    rep stosb
    lea rsp,[rel stack_top]
    xor ebp,ebp
    call kernel_main
    ; Returning from kernel_main is a real fatal error, never successful completion.
    ud2
section .bss
align 16
stack_bottom: resb 32768
stack_top:
section .note.GNU-stack noalloc noexec nowrite progbits
