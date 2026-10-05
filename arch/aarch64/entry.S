// Project-authored Pi firmware handoff: AArch64 at 0x80000, MMU off, EL1/EL2.
.section .text.entry,"ax"
.global pi_entry
pi_entry:
    msr daifset, #15
    mrs x1, mpidr_el1
    and x1, x1, #255
    cbnz x1, park
    mrs x1, CurrentEL
    cmp x1, #8
    b.ne at_el1
    mov x1, #(1 << 31)
    msr hcr_el2, x1
    mov x1, #3
    msr cnthctl_el2, x1
    msr cntvoff_el2, xzr
    adr x1, at_el1
    msr elr_el2, x1
    mov x1, #0x3c5
    msr spsr_el2, x1
    eret
at_el1:
    mrs x1, CurrentEL
    cmp x1, #4
    b.ne park
    ldr x1, =stack_top
    mov sp, x1
    ldr x1, =__bss_start
    ldr x2, =__bss_end
zero:
    cmp x1, x2
    b.hs ready
    str xzr, [x1], #8
    b zero
ready:
    bl pi_main
park:
    wfe
    b park
.section .bss,"aw",%nobits
.balign 16
.space 8192
stack_top:
