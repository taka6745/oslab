; Authored Intel/PCI config1 and i440fx/PIIX3 bytes. Bounded minimal-board setup.
; Only PVH executes this path. BIOS driver/image remain byte-identical.
.section text
.org 111000
@raw_pvh_pci
; Check CPUID/MSR before touching APIC base; singleCPU PIC board.
9C 58 89 C1 35 00 00 20 00 50 9D 9C 58 51 9D
31 C8 A9 00 00 20 00 0F 84 rel32:raw_pvh_fail
31 C0 0F A2 83 F8 01 0F 82 rel32:raw_pvh_fail
B8 01 00 00 00 0F A2
@raw_pvh_cpu_features
F6 C2 20 0F 84 rel32:raw_pvh_fail
F7 C2 00 02 00 00 74 rel8:raw_pvh_apic_done
B9 1B 00 00 00 0F 32 25 FF F3 FF FF 0F 30
@raw_pvh_apic_done
B8 00 00 00 80 E8 rel32:raw_pvh_read
3D 86 80 37 12 0F 85 rel32:raw_pvh_fail
31 F6 31 DB BD 00 00 00 80
@raw_pvh_scan
89 E8 E8 rel32:raw_pvh_read
83 F8 FF 0F 84 rel32:raw_pvh_scan_next
3D 86 80 0E 10 74 rel8:raw_pvh_nic
3D 86 80 D3 10 74 rel8:raw_pvh_nic
3D 86 80 00 70 75 rel8:raw_pvh_header
85 DB 0F 85 rel32:raw_pvh_fail
89 EB EB rel8:raw_pvh_header
@raw_pvh_nic
; Shared kernel supports one root-bus function0 NIC, exactly one allowed here.
F7 C5 00 07 00 00 0F 85 rel32:raw_pvh_fail
85 F6 0F 85 rel32:raw_pvh_fail
89 EE
@raw_pvh_header
89 E8 83 C8 0C E8 rel32:raw_pvh_read
25 00 00 7F 00 0F 85 rel32:raw_pvh_fail
BF 10 00 00 00
@raw_pvh_bars
89 E8 09 F8 E8 rel32:raw_pvh_read
A8 01 75 rel8:raw_pvh_bar_next
89 C1 83 E1 F0 0F 85 rel32:raw_pvh_fail
83 E0 06 74 rel8:raw_pvh_bar_next
83 F8 04 0F 85 rel32:raw_pvh_fail
83 FF 24 0F 84 rel32:raw_pvh_fail
83 C7 04 89 E8 09 F8 E8 rel32:raw_pvh_read
85 C0 0F 85 rel32:raw_pvh_fail
@raw_pvh_bar_next
83 C7 04 83 FF 28 72 rel8:raw_pvh_bars
@raw_pvh_scan_next
81 C5 00 01 00 00 81 FD 00 00 01 80
0F 82 rel32:raw_pvh_scan
85 F6 0F 84 rel32:raw_pvh_fail
85 DB 0F 84 rel32:raw_pvh_fail
; Probe real BAR0 with memory decode and bus master disabled.
89 F0 83 C8 04 E8 rel32:raw_pvh_read
0F B7 F8 89 F9 83 E1 F9
89 F0 83 C8 04 E8 rel32:raw_pvh_write
B9 FF FF FF FF 89 F0 83 C8 10 E8 rel32:raw_pvh_write
89 F0 83 C8 10 E8 rel32:raw_pvh_read
@raw_pvh_bar_probe_result
83 E0 F0 3D 00 00 FE FF 0F 85 rel32:raw_pvh_fail
B9 00 00 00 C0 89 F0 83 C8 10 E8 rel32:raw_pvh_write
89 F0 83 C8 10 E8 rel32:raw_pvh_read
@raw_pvh_bar_assignment_result
25 F0 FF FF FF 3D 00 00 00 C0 0F 85 rel32:raw_pvh_fail
89 F9 89 F0 83 C8 04 E8 rel32:raw_pvh_write
; Actual INTA..D swizzle into actual PIIX3, route IRQ11 and verify both writes.
89 F0 83 C8 3C E8 rel32:raw_pvh_read
89 C7 C1 EF 08 81 E7 FF 00 00 00
4F 83 FF 03 0F 87 rel32:raw_pvh_fail
89 F5 C1 ED 0B 4D 01 FD 83 E5 03
; Four-byte PIRQ register has independently writable byte ports.
89 D8 83 C8 60 BA F8 0C 00 00 EF
8D 95 FC 0C 00 00 B0 0B EE EC
@raw_pvh_pirq_result
3C 0B
0F 85 rel32:raw_pvh_fail
89 F0 83 C8 3C E8 rel32:raw_pvh_read
89 C1 81 E1 00 FF FF FF 83 C9 0B
89 F0 83 C8 3C E8 rel32:raw_pvh_write
89 F0 83 C8 3C E8 rel32:raw_pvh_read
3C 0B 0F 85 rel32:raw_pvh_fail
BA D1 04 00 00 EC 0C 08 EE EC A8 08
0F 84 rel32:raw_pvh_fail
C3
@raw_pvh_read
BA F8 0C 00 00 EF 66 BA FC 0C ED C3
@raw_pvh_write
BA F8 0C 00 00 EF 89 C8 66 BA FC 0C EF C3
@raw_pvh_pci_end
