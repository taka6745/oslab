; Intel SDM manually encoded kernel entry, fixed1CPU BIOS setup.
.section text
.org 100000
@raw_start32
; Check EFLAGS.ID before executing CPUID; require PSE, PAE and MSR.
9C 58 89 C1 35 00 00 20 00 50 9D 9C 58 51 9D
31 C8 A9 00 00 20 00 0F 84 rel32:boot_fail32
31 C0 0F A2 83 F8 01 0F 82 rel32:boot_fail32
B8 01 00 00 00 0F A2 81 E2 68 00 00 00
83 FA 68 0F 85 rel32:boot_fail32
B8 00 00 00 80 0F A2 3D 01 00 00 80 0F 82 rel32:boot_fail32
B8 01 00 00 80 0F A2 F7 C2 00 00 00 20 0F 84 rel32:boot_fail32
BF 00 00 09 00 31 C0 B9 00 18 00 00 F3 AB
C7 05 00 00 09 00 03 10 09 00
BF 00 10 09 00 B8 03 20 09 00 B9 04 00 00 00
@boot_pdpt
AB 83 C7 04 05 00 10 00 00 E2 rel8:boot_pdpt
BF 00 20 09 00 B8 83 00 00 00 B9 00 08 00 00
@boot_pages
3D 00 00 00 C0 72 rel8:boot_page_store
83 C8 18
@boot_page_store
AB 83 C7 04 05 00 00 20 00 E2 rel8:boot_pages
0F 20 E0 83 C8 20 0F 22 E0 B8 00 00 09 00 0F 22 D8
B9 80 00 00 C0 0F 32 0D 00 01 00 00 0F 30
0F 20 C0 0D 00 00 00 80 0F 22 C0
EA abs32:raw_entry 18 00
@boot_fail32
FA F4 EB FC
@raw_entry
FA FC 66 B8 10 00 8E D8 8E C0 8E D0
48 C7 C4 00 F0 1F 00
; Clear through the end of fixed IDT/DMA/network state, not unused stack RAM.
BF 00 00 18 00 31 C0 B9 00 42 00 00 F3 48 AB
; Build all256 interrupt gates to terminal fault handler, no silent reset.
BF 00 00 1A 00 B9 00 01 00 00
; Writer bounds all text to0x100000..0x10fdff: gate middle=0x10, upper=0.
; High gate qword remains zero from the arena clear above.
BA abs32:raw_fault 0F B7 C2
48 BA 00 00 18 00 00 8E 10 00 48 09 D0
@raw_idt_next
48 AB 48 83 C7 08 E2 rel8:raw_idt_next
0F 01 1D rel32:raw_idtr
E8 rel32:nic_init 85 C0 74 rel8:raw_fault
E8 rel32:net_init
E8 rel32:irq_init 85 C0 74 rel8:raw_fault
@raw_loop
E8 rel32:net_poll E8 rel32:idle_wait EB rel8:raw_loop
@raw_fault
FA
@raw_halted
F4 EB rel8:raw_halted
@raw_idtr
FF 0F 00 00 1A 00 00 00 00 00
.section bss
.org 180100
@nic_mac
.zero 4
@nic_mac_tail
.zero 4
@nic_mmio
.zero 8
@nic_rx_head
.zero 4
@nic_tx_tail
.zero 4
@nic_rx_outstanding
.zero 4
@nic_irq_line
.zero 4
@hpet_period
.zero 8
@last_rx_ms
.zero 8
