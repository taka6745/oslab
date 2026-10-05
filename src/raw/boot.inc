; Authored BIOS16 -> protected32 -> long64 bytes. Intel SDM, BIOS EDD/E820.
.section text
.org 7c00
@raw_boot
EA abs16:boot_normal 00 00
@boot_normal
FA 31 C0 8E D8 8E C0 8E D0 BC 00 7C FC
88 16 abs16:boot_drive
CD 12 3D 58 02 0F 82 rel16:boot_fail
; E820 fixed bounded records at0x5000, require usable RAM1MiB..2MiB.
66 31 DB BF 00 50
@boot_e820
81 FF 00 56 0F 83 rel16:boot_fail
66 B8 20 E8 00 00 66 BA 50 41 4D 53 66 B9 18 00 00 00
26 66 C7 45 14 01 00 00 00 CD 15
0F 82 rel16:boot_fail
66 3D 50 41 4D 53 0F 85 rel16:boot_fail
66 83 F9 14 0F 82 rel16:boot_fail
66 83 F9 18 0F 87 rel16:boot_fail
83 C7 18 81 FF 00 56 0F 87 rel16:boot_fail
66 85 DB 75 rel8:boot_e820
89 3E 00 56
; Read bounded payload in one EDD request below64KiB.
FB 8A 16 abs16:boot_drive B4 41 BB AA 55 CD 13
0F 82 rel16:boot_fail 81 FB 55 AA 0F 85 rel16:boot_fail
F7 C1 01 00 0F 84 rel16:boot_fail
BE abs16:boot_dap B4 42 8A 16 abs16:boot_drive CD 13
0F 82 rel16:boot_fail FA E4 92 0C 02 24 FE E6 92
0F 01 16 abs16:boot_gdtr
0F 20 C0 66 83 C8 01 0F 22 C0
66 EA abs32:boot_protected 08 00
@boot_fail
FA F4 EB FC
.align 8
@boot_dap
10 00 value16:kernel_sectors 00 00 00 10 01 00 00 00 00 00 00 00
@boot_drive
00
.align 8
@boot_gdt
00 00 00 00 00 00 00 00
FF FF 00 00 00 9A CF 00
FF FF 00 00 00 92 CF 00
FF FF 00 00 00 9A AF 00
@boot_gdtr
1F 00 abs32:boot_gdt
@boot_protected
FC 66 B8 10 00 8E D8 8E C0 8E D0 BC 00 7C 00 00
; Validate actual enabled E820 usable RAM before first access above1MiB.
BE 00 50 00 00 0F B7 15 00 56 00 00
@boot_ram
39 D6 0F 83 rel32:boot_fail32
F6 46 14 01 74 rel8:boot_ram_next
83 7E 10 01 75 rel8:boot_ram_next
83 7E 04 00 75 rel8:boot_ram_next
81 3E 00 00 10 00 77 rel8:boot_ram_next
8B 06 03 46 08 8B 4E 0C 83 D1 00
0F 82 rel32:boot_fail32
85 C9 75 rel8:boot_ram_ok
3D 00 00 20 00 73 rel8:boot_ram_ok
@boot_ram_next
83 C6 18 EB rel8:boot_ram
@boot_ram_ok
; Invertedword above1MiB must not alias knownsafe conventionalRAM scratch.
0F B7 05 00 60 00 00 0F B7 15 00 60 10 00
89 C1 81 F1 FF FF 00 00
66 89 0D 00 60 10 00
66 39 05 00 60 00 00
66 89 15 00 60 10 00
0F 85 rel32:boot_fail32
BE 00 00 01 00 BF 00 00 10 00 B9 value32:kernel_size F3 A4
BE 00 00 10 00 B9 value32:kernel_size B8 C5 9D 1C 81
@boot_hash
0F B6 16 31 D0 69 C0 93 01 00 01 46 E2 rel8:boot_hash
3D value32:kernel_hash 0F 85 rel32:boot_fail32
EA 00 00 10 00 08 00
@boot_fail32
FA F4 EB FC
.org 7dfe
55 AA
