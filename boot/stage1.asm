; BIOS boot sector: validate EDD, read and validate our own second stage.
bits 16
section .text
global stage1
stage1:
    jmp 0:normalized
normalized:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    cld
    mov [drive], dl
    call uart_init
    mov si, marker
    call print16
    sti
    mov dl, [drive]
    mov ah, 0x41
    mov bx, 0x55aa
    int 0x13
    jc failed
    cmp bx, 0xaa55
    jne failed
    test cx, 1
    jz failed
    mov bp, 3
.retry:
    mov word [dap+2],8
    mov si, dap
    mov dl, [drive]
    mov ah, 0x42
    int 0x13
    jnc .read
    xor ah, ah
    int 0x13
    dec bp
    jnz .retry
    jmp failed
.read:
    mov eax, 2166136261
    mov si, 0x8000
    mov cx, 4096
.hash:
    movzx edx, byte [si]
    xor eax, edx
    imul eax, eax, 16777619
    inc si
    loop .hash
    cmp eax, STAGE2_HASH
    jne failed
    mov dl, [drive]
    jmp 0:0x8000
failed:
    mov si, error
    call print16
    mov dx, 0xf4
    mov al, 0x11
    out dx, al
    cli
.loop: hlt
    jmp .loop
%include 'boot/serial.inc'
marker: db 'OSL1 BOOT stage1',10,0
error: db 'OSL1 BOOT_ERROR stage1',10,0
align 4
dap: db 16,0
    dw 8,0x8000,0
    dq 1
drive: db 0
times 510-($-$$) db 0
dw 0xaa55
