; Project-authored machine encoding from Intel SDM Vol. 2 instruction formats.
; Fixed low (<4 GiB) kernel placement is enforced by the project linker script.
; 32-bit MOVs zero-extend pointers; all BSS bytes still clear before C execution.
bits 64
section .text.entry
global kernel_entry, stack_bottom, stack_top
extern __bss_start, __bss_end, kernel_main
kernel_entry:
    db 0xfc                         ; cld
    db 0xbf                         ; mov edi, __bss_start
    dd __bss_start
    db 0xb9                         ; mov ecx, __bss_end
    dd __bss_end
    db 0x48,0x29,0xf9               ; sub rcx,rdi
    db 0x31,0xc0                    ; xor eax,eax
    db 0xf3,0xaa                    ; rep stosb
    db 0xbc                         ; mov esp,stack_top
    dd stack_top
    db 0x31,0xed                    ; xor ebp,ebp
    db 0xe8                         ; call kernel_main (rel32)
    dd kernel_main-($+4)
    db 0x0f,0x0b                    ; ud2: unexpected C return is fatal
section .bss
align 16
stack_bottom: resb STACK_BYTES
stack_top:
section .note.GNU-stack noalloc noexec nowrite progbits
