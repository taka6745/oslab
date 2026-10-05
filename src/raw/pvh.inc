; Authored Intel SDM bytes for Xen PHYS32_ENTRY v1. No BIOS replacement claim.
; External writer loads exact kernel at1MiB and this adapter at0x110000,
; emits Xen ELF note18 with raw_pvh_entry, and resolves kernel size/FNV fields.
.section text
.org 110000
@raw_pvh_entry
FA FC
; Bound actual start-info and map before dereferencing; use no stack yet.
81 FB 00 10 00 00 0F 82 rel32:raw_pvh_fail
81 FB C8 FF 3F 00 0F 87 rel32:raw_pvh_fail
81 3B 78 C5 6E 33 0F 85 rel32:raw_pvh_fail
83 7B 04 01 0F 85 rel32:raw_pvh_fail
83 7B 2C 00 0F 85 rel32:raw_pvh_fail
83 7B 34 00 0F 85 rel32:raw_pvh_fail
8B 73 28 8B 6B 30
8D 45 FF 83 F8 3F 0F 87 rel32:raw_pvh_fail
81 FE 00 10 00 00 0F 82 rel32:raw_pvh_fail
6B C5 18 01 F0 0F 82 rel32:raw_pvh_fail
3D 00 00 40 00 0F 87 rel32:raw_pvh_fail
31 FF
@raw_pvh_map
; Reject overflow/reserved fields even for records that are not usable RAM.
8B 06 8B 56 04 03 46 08 13 56 0C
0F 82 rel32:raw_pvh_fail
83 7E 14 00 0F 85 rel32:raw_pvh_fail
; Board aperture must not intersect any loader-declared region.
83 7E 04 00 75 rel8:raw_pvh_aperture_ok
81 3E 00 00 00 E0 73 rel8:raw_pvh_aperture_ok
85 D2 0F 85 rel32:raw_pvh_fail
3D 00 00 00 C0 0F 87 rel32:raw_pvh_fail
@raw_pvh_aperture_ok
83 7E 10 01 75 rel8:raw_pvh_next
83 7E 04 00 75 rel8:raw_pvh_next
; Cover all low scratch/stack/GDT/page tables with one declared RAM record.
81 3E 00 50 00 00 77 rel8:raw_pvh_high
85 D2 75 rel8:raw_pvh_low_ok
3D 00 60 09 00 72 rel8:raw_pvh_high
@raw_pvh_low_ok
83 CF 01
@raw_pvh_high
; Same1MiB..2MiB usable interval required by complete BIOS boot.
81 3E 00 00 10 00 77 rel8:raw_pvh_next
85 D2 75 rel8:raw_pvh_high_ok
3D 00 00 20 00 72 rel8:raw_pvh_next
@raw_pvh_high_ok
83 CF 02
@raw_pvh_next
83 C6 18 4D 75 rel8:raw_pvh_map
83 FF 03 0F 85 rel32:raw_pvh_fail
; Metadata is now consumed: installing GDT/stack cannot corrupt unread records.
0F 01 15 abs32:raw_pvh_gdtr
EA abs32:raw_pvh_segments 08 00
@raw_pvh_segments
66 B8 10 00 8E D8 8E C0 8E D0 BC 00 7C 00 00
; Validate the actual preloaded executable, never trust a claimed hash result.
BE 00 00 10 00 B9 value32:kernel_size B8 C5 9D 1C 81
@raw_pvh_hash
0F B6 16 31 D0 69 C0 93 01 00 01 46 E2 rel8:raw_pvh_hash
3D value32:kernel_hash 0F 85 rel32:raw_pvh_fail
E8 rel32:raw_pvh_pci
; Shared literal entry retains CPUID checks, page mappings and terminal faults.
EA abs32:raw_start32 08 00
@raw_pvh_fail
FA
@raw_pvh_halted
F4 EB rel8:raw_pvh_halted
.align 8
@raw_pvh_gdt
00 00 00 00 00 00 00 00
FF FF 00 00 00 9A CF 00
FF FF 00 00 00 92 CF 00
FF FF 00 00 00 9A AF 00
@raw_pvh_gdtr
1F 00 abs32:raw_pvh_gdt
@raw_pvh_end
