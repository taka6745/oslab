# Hand-encoded PVH adapter

`pvh.asm` is separately authored instruction bytes, ModR/M bytes, immediates
and label relocations. Comments retain readable instruction intent; the original
`boot/pvh.asm` remains the readable reference. No bytes are extracted from a
compiled kernel or third-party implementation. NASM supplies object format,
symbols, relocations and payload inclusion, not instruction selection here.

Provenance: [Intel SDM Volume 2](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
instruction encoding tables; [Xen PVH ABI](https://xenbits.xen.org/docs/unstable/misc/pvh.html)
and its public start-info format. Fixed addresses retain this project's existing
PVH memory layout: map count/records at 0x5000/0x5010, low GDT 0x8000,
page tables 0x90000..0x95fff, stack 0x7c00, kernel 0x100000,
adapter 0x200000. They describe setup, never expected network answers.

Interface: NASM `-f elf32 -g -F dwarf`, defines `OSLAB_PRODUCTION`,
`KERNEL_PRELOADED`, `KERNEL_BYTES`, `KERNEL_HASH`, and quoted `KERNEL_FILE`.
Link with `ld.lld -m elf_i386 -nostdlib -T boot/pvh.ld`. The Xen note and
`pvh_entry` symbol match the reference; `machine_long_entry` marks 64-bit mode.
The payload is the actual hybrid kernel: selected hand-encoded runtime and HTTP
functions alongside retained readable C drivers, networking and other services.
It is not an entirely hand-encoded OS.

Retained invariants: loader pointer/magic/version bounds, map count and source
bounds/overlap, 64-bit record addition overflow, executing kernel FNV hash,
CPUID availability and long-mode support, full 4 GiB identity mapping, relocated
GDT and fail-stop. Count validation uses `(count-1)<=63` with unsigned comparison;
zero and all counts above 64 fail. Memory-map copies use MOV immediates: stack writes are unsafe until all
source records are consumed. Later constant loads use balanced push/pop. Long-mode ESP assignment zero-extends to RSP. `bt edx,29`
tests the documented long-mode bit. Instruction bytes alone cannot improve ISA
speed over equally selected mnemonics.

Task goal: a small separately hand-encoded mode-entry adapter; prerequisites are
source-verified kernel and pinned NASM/LLD. Acceptance requires independent
32/64-bit disassembly, malformed metadata/hash/CPU failure tests, real HTTP/wire
checks and matched timing against the reference, followed by exact-image homelab
validation. Manual builds of all debug/production and embedded/preloaded variants
passed NASM warnings-as-errors and linked. Corrected production text is 552 bytes
(embedded payload) or 536 bytes (preloaded), versus 560 bytes of reference
embedded production text. These counts exclude Xen note, kernel and ELF metadata.
Debug ELF containers can be larger because of authored source line tables.
The full raw HTTP host differential gate passed with protected pages and
200,000 seeded mutations; deliberate defects were rejected. The current hybrid
build has an 18,326-byte kernel, 14,010-byte packed payload and 15,872-byte disk
image; the matched readable PVH build has a 21,398-byte kernel and 17,408-byte
disk image. The embedded loader passed eleven boundary checks, including three valid
source/stack overlaps, and exact-image homelab wire/stress acceptance. Initial
stack constant loads corrupted a valid source map and were rejected; external
evidence retains that failure. Preload is compile-checked only. The complete
OS does not fit hundreds of bits.
