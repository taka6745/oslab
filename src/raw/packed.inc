; Optional raw packed-disk adapter; ISA bytes authored from Intel SDM.
; Format authored in boot/pack.c and boot/stage2.asm: literal1..128 or
; match3..130 with little-endian backward distance1..65535, overlap allowed.
; Existing BIOS chain verifies stored payload FNV before entering this stub.
; Writer requires nonempty bounded blob/decoded image, blob<=64KiB,
; source blob wholly inside verified payload, relocated end<=0x90000.
.section text
.org 100000
@packed_entry32
FC ; independently clear direction before relocation
BE value32:blob_source
BF 00 00 08 00
B9 value32:relocated_bytes
F3 A4
EA abs32:packed_decode 08 00
@packed_stub_end
.section text
.org 80000
@packed_decode
FC
BE value32:input_start
BB value32:input_end
BF 00 00 10 00
BD value32:decoded_kernel_end
@packed_next
39 DE
74 rel8:packed_decoded
77 rel8:packed_fail
0F B6 0E 46
F6 C1 80
75 rel8:packed_match
; Literal: tag+1 bytes, input and output remaining-space checks.
41
89 D8 29 F0 39 C1
77 rel8:packed_fail
89 E8 29 F8 39 C1
77 rel8:packed_fail
F3 A4
EB rel8:packed_next
@packed_match
83 E1 7F 83 C1 03
89 E8 29 F8 39 C1
77 rel8:packed_fail
89 D8 29 F0 83 F8 02
72 rel8:packed_fail
0F B7 16 83 C6 02
85 D2
74 rel8:packed_fail
89 F8 2D 00 00 10 00 39 C2
77 rel8:packed_fail
; Save compressed cursor, copy forward from already emitted bytes, restore.
56 89 FE 29 D6 F3 A4 5E
EB rel8:packed_next
@packed_fail
FA F4 EB FC
@packed_decoded
39 EF
75 rel8:packed_fail
; Original kernel FNV checks decoder output before any kernel instruction.
BE 00 00 10 00
B9 value32:decoded_kernel_size
B8 C5 9D 1C 81
@packed_hash
0F B6 16 31 D0 69 C0 93 01 00 01 46 E2 rel8:packed_hash
3D value32:decoded_kernel_hash
75 rel8:packed_fail
EA 00 00 10 00 08 00
@packed_decoder_end
