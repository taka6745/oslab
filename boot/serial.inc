; Authored BIOS-stage UART/debug-port diagnostics, bounded transmitter wait.
uart_init:
    mov dx, 0x3fb
    mov al, 0x80
    out dx, al
    mov dx, 0x3f8
    mov al, 1
    out dx, al
    inc dx
    xor al, al
    out dx, al
    mov dx, 0x3fb
    mov al, 3
    out dx, al
    ret
print16:
    lodsb
    test al, al
    jz .done
    mov ah, al
    mov dx, 0xe9
    out dx, al
    mov cx, 65535
.wait:
    mov dx, 0x3fd
    in al, dx
    test al, 0x20
    jnz .write
    loop .wait
    jmp .done
.write:
    mov al, ah
    mov dx, 0x3f8
    out dx, al
    jmp print16
.done:
    ret
