; Load kernel via BIOS EDD, collect E820, validate payload and enter long mode.
bits 16
section .text
global stage2, protected_entry, long_entry
stage2:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov [drive], dl
%if !OSLAB_PRODUCTION
    call uart_init
    mov si, marker
    call print16
%endif
    ; E820 records at 0x5010; count at 0x5000. Maximum 64 records of 24 bytes.
    mov word [0x5000],0
    xor ebx, ebx
    mov di,0x5010
.e820:
    mov dword [es:di+20],1
    mov eax,0xe820
    mov edx,0x534d4150
    mov ecx,24
    int 0x15
    jc failed
    cmp eax,0x534d4150
    jne failed
    cmp ecx,20
    jb failed
    inc word [0x5000]
    add di,24
    test ebx,ebx
    jz .load
    cmp word [0x5000],64
    jb .e820
    jmp failed
.load:
    mov bp, KERNEL_SECTORS
.next:
    mov ax,bp
    cmp ax,32
    jbe .count
    mov ax,32
.count:
    mov [dap+2],ax
    mov byte [retries],3
.retry:
    mov si,dap
    mov ah,0x42
    mov dl,[drive]
    int 0x13
    jnc .loaded
    xor ah,ah
    int 0x13
    dec byte [retries]
    jnz .retry
    jmp failed
.loaded:
    mov ax,[dap+2]
    sub bp,ax
    add [dap+8],ax
    shl ax,5
    add [dap+6],ax
    test bp,bp
    jnz .next
    cli
    ; Fast A20 gate; later copy/hash detects a wrapped high-memory destination.
    in al,0x92
    or al,2
    and al,0xfe
    out 0x92,al
    lgdt [gdtr]
    mov eax,cr0
    or eax,1
    mov cr0,eax
    jmp 0x08:protected_entry
failed:
%if !OSLAB_PRODUCTION
    mov si,error
    call print16
    mov dx,0xf4
    mov al,0x11
    out dx,al
%endif
    cli
.loop: hlt
    jmp .loop
%if !OSLAB_PRODUCTION
%include 'boot/serial.inc'
marker: db 'OSL1 BOOT stage2',10,0
error: db 'OSL1 BOOT_ERROR stage2',10,0
%endif
align 4
dap: db 16,0
    dw 0,0,0x1000
    dq KERNEL_LBA
drive: db 0
retries: db 0
align 8
gdt: dq 0
    dq 0x00cf9a000000ffff ; flat 32-bit code
    dq 0x00cf92000000ffff ; flat data
    dq 0x00af9a000000ffff ; 64-bit code
gdtr: dw $-gdt-1
    dd gdt
bits 32
protected_entry:
    cld ; BIOS must not determine the direction of our bounded copies.
    mov ax,0x10
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,0x7c00
    mov esi,0x10000
    mov edi,0x100000
%if KERNEL_COMPRESSED
    ; Decode our literal/back-reference format with both buffers bounded.
    mov ebx,0x10000+KERNEL_STORAGE_BYTES
    mov ebp,0x100000+KERNEL_BYTES
.decode:
    cmp esi,ebx
    je .decoded
    ja protected_failed
    movzx ecx,byte [esi]
    inc esi
    test cl,0x80
    jnz .match
    inc ecx
    mov eax,ebx
    sub eax,esi
    cmp ecx,eax
    ja protected_failed
    mov eax,ebp
    sub eax,edi
    cmp ecx,eax
    ja protected_failed
    rep movsb
    jmp .decode
.match:
    and ecx,127
    add ecx,3
    mov eax,ebp
    sub eax,edi
    cmp ecx,eax
    ja protected_failed
    mov eax,ebx
    sub eax,esi
    cmp eax,2
    jb protected_failed
    movzx edx,word [esi]
    add esi,2
    test edx,edx
    jz protected_failed
    mov eax,edi
    sub eax,0x100000
    cmp edx,eax
    ja protected_failed
    push esi
    mov esi,edi
    sub esi,edx
    rep movsb
    pop esi
    jmp .decode
.decoded:
    cmp edi,ebp
    jne protected_failed
%else
    mov ecx,KERNEL_BYTES
    rep movsb
%endif
    mov esi,0x100000
    mov ecx,KERNEL_BYTES
    mov eax,2166136261
.hash:
    movzx edx,byte [esi]
    xor eax,edx
    imul eax,eax,16777619
    inc esi
    loop .hash
    cmp eax,KERNEL_HASH
    jne protected_failed
    ; Verify the CPU has CPUID and long mode before enabling paging.
    pushfd
    pop eax
    mov ecx,eax
    xor eax,1<<21
    push eax
    popfd
    pushfd
    pop eax
    xor eax,ecx
    test eax,1<<21
    jz protected_failed
    mov eax,0x80000000
    cpuid
    cmp eax,0x80000001
    jb protected_failed
    mov eax,0x80000001
    cpuid
    test edx,1<<29
    jz protected_failed
    ; 4 GiB identity map: PML4 + PDPT + four page directories at 0x90000.
    mov edi,0x90000
    xor eax,eax
    mov ecx,6144
    rep stosd
    mov dword [0x90000],0x91003
    mov edi,0x91000
    mov eax,0x92003
    mov ecx,4
.pdpt:
    stosd
    add edi,4
    add eax,4096
    loop .pdpt
    mov edi,0x92000
    mov eax,0x83
    mov ecx,2048
.pages:
    stosd
    add edi,4
    add eax,0x200000
    loop .pages
    mov eax,cr4
    or eax,1<<5
    mov cr4,eax
    mov eax,0x90000
    mov cr3,eax
    mov ecx,0xc0000080
    rdmsr
    or eax,1<<8
    wrmsr
    mov eax,cr0
    or eax,1<<31
    mov cr0,eax
    jmp 0x18:long_entry
protected_failed:
%if !OSLAB_PRODUCTION
    mov esi,protected_error
.print:
    lodsb
    test al,al
    jz .stop
    mov dx,0xe9
    out dx,al
    mov dx,0x3f8
    out dx,al
    jmp .print
.stop:
    mov dx,0xf4
    mov al,0x11
    out dx,al
%endif
    cli
.halt: hlt
    jmp .halt
%if !OSLAB_PRODUCTION
protected_error: db 'OSL1 BOOT_ERROR kernel-integrity-or-cpu',10,0
%endif
bits 64
long_entry:
    mov ax,0x10
    mov ds,ax
    mov es,ax
    mov ss,ax
    xor eax,eax
    mov fs,ax
    mov gs,ax
    mov rsp,0x7c00
    mov rax,0x100000 ; kernel entry is asserted by the linker script
    jmp rax
times STAGE2_BYTES-($-$$) db 0
