; Authored PVH32 adapter from Xen x86/HVM direct boot ABI.
; Optional alternate entry; the BIOS disk chain remains independent.
%ifndef OSLAB_PRODUCTION
%define OSLAB_PRODUCTION 0
%endif
%ifndef KERNEL_PRELOADED
%define KERNEL_PRELOADED 0
%endif
%if KERNEL_PRELOADED != 0 && KERNEL_PRELOADED != 1
%error "KERNEL_PRELOADED must be 0 or 1"
%endif
%if KERNEL_BYTES <= 0 || KERNEL_BYTES > 524288
%error "kernel payload must be 1..524288 bytes"
%endif
bits 32
section .note.Xen note alloc align=4
    dd 4,4,18 ; XEN_ELFNOTE_PHYS32_ENTRY
    db 'Xen',0
    dd pvh_entry
section .text
global pvh_entry
pvh_entry:
    cli
    cld
    lgdt [gdtr]
    jmp 0x08:.segments
.segments:
    mov ax,0x10
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,0x7c00
    ; Bound every loader pointer to low mapped memory before dereference.
    cmp ebx,0x1000
    jb pvh_failed
    cmp ebx,0x400000-56
    ja pvh_failed
    cmp dword [ebx],0x336ec578
    jne pvh_failed
    cmp dword [ebx+4],1
    jne pvh_failed
    cmp dword [ebx+44],0
    jne pvh_failed
    mov esi,[ebx+40]
    mov ecx,[ebx+48]
    test ecx,ecx
    jz pvh_failed
    cmp ecx,64
    ja pvh_failed
    cmp esi,0x1000
    jb pvh_failed
    mov eax,ecx
    imul eax,24
    mov edx,esi
    add edx,eax
    jc pvh_failed
    cmp edx,0x400000
    ja pvh_failed
    ; Never permit source records to overlap adapter destinations.
    cmp esi,0x5610
    jae .map_safe
    cmp edx,0x5000
    ja pvh_failed
.map_safe:
    mov [0x5000],cx
    mov edi,0x5010
    mov ebp,ecx
.map:
    ; Reject wrapping 64-bit base + size before copying records.
    mov eax,[esi]
    mov edx,[esi+4]
    add eax,[esi+8]
    adc edx,[esi+12]
    jc pvh_failed
    mov ecx,5
    rep movsd
    add esi,4 ; PVH reserved field is not E820 attributes.
    mov eax,1
    stosd
    dec ebp
    jnz .map
%if !KERNEL_PRELOADED
    mov esi,kernel_payload
    mov edi,0x100000
    mov ecx,KERNEL_BYTES
    rep movsb
%endif
    ; Both entries hash the actual bytes executing at the kernel load address.
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
    jne pvh_failed
    ; Keep descriptor tables outside both the kernel BSS and disposable adapter.
    mov esi,gdt
    mov edi,0x8000
    mov ecx,(gdtr-gdt)/4
    rep movsd
    lgdt [low_gdtr]
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
    jz pvh_failed
    mov eax,0x80000000
    cpuid
    cmp eax,0x80000001
    jb pvh_failed
    mov eax,0x80000001
    cpuid
    test edx,1<<29
    jz pvh_failed
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
pvh_failed:
%if !OSLAB_PRODUCTION
    mov dx,0xe9
    mov al,'P'
    out dx,al
    mov dx,0xf4
    mov al,0x11
    out dx,al
%endif
.halt:
    cli
    hlt
    jmp .halt
align 8
gdt: dq 0
    dq 0x00cf9a000000ffff
    dq 0x00cf92000000ffff
    dq 0x00af9a000000ffff
gdtr: dw $-gdt-1
    dd gdt
low_gdtr: dw gdtr-gdt-1
    dd 0x8000
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
    mov eax,0x100000
    jmp rax
%if !KERNEL_PRELOADED
section .rodata align=16
kernel_payload:
    incbin KERNEL_FILE
kernel_payload_end:
%if ($-kernel_payload) != KERNEL_BYTES
    %error "kernel payload size mismatch"
%endif
%endif
section .note.GNU-stack noalloc noexec nowrite progbits
