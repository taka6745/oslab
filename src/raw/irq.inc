; Authored instruction bytes; Intel SDM interrupt gate/IRETQ formats.
.section text
.org 1021b8
@irq_init
FA 53
8B 1C 25 1C 01 18 00
83 FB 03 72 rel8:irq_init_bad
83 FB 10 73 rel8:irq_init_bad
BF 00 02 1A 00 B9 10 00 00 00
; Text writer bounds imply middle0x10 and upper0; arena clear supplied upperqwords.
BA abs32:raw_irq 0F B7 C2
48 BA 00 00 18 00 00 8E 10 00 48 09 D0
@irq_gate_next
48 AB 48 83 C7 08 E2 rel8:irq_gate_next
; PIC ICW1, vector bases32/40, slave wiringIRQ2, 8086 mode.
B0 FF E6 21 E6 A1
B0 11 E6 20 E6 A0
B0 20 E6 21 B0 28 E6 A1
B0 04 E6 21 B0 02 E6 A1
B0 01 E6 21 E6 A1
; PIT channel0 binary mode3, divisor11932; nominal99.9985Hz.
B0 36 E6 43 B0 9C E6 40 B0 2E E6 40
; PIT, cascade, NIC unmasked; all other PIC inputs masked.
89 D9 BA 01 00 00 00 D3 E2 83 CA 05 F7 D2
89 D0 E6 21 C1 E8 08 E6 A1
E8 rel32:nic_irq_enable
B8 01 00 00 00 5B C3
@irq_init_bad
31 C0 5B C3
@raw_irq
; Preserve every GPR exceptRSP (restored via savedframe); interrupt has noerrorcode.
50 51 52 53 55 56 57
41 50 41 51 41 52 41 53 41 54 41 55 41 56 41 57
FC 48 89 E3 48 83 E4 F0
E8 rel32:nic_interrupt
; Acknowledge both controllers; nonspecific slave EOI harmless for masterIRQ.
B0 20 E6 A0 E6 20
48 89 DC
41 5F 41 5E 41 5D 41 5C 41 5B 41 5A 41 59 41 58
5F 5E 5D 5B 5A 59 58
48 CF
@idle_wait
FA 48 83 EC 08
E8 rel32:nic_pending
48 83 C4 08 85 C0
75 rel8:idle_return
48 8B 05 rel32:net_state
48 2B 04 25 28 01 18 00 48 83 F8 02
72 rel8:idle_return
FB F4 FA
@idle_return
C3
@irq_end
