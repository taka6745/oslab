# Literal direct entry

The optional PHYS32/Xen version1 entry loads the same authored kernel at1MiB,
metadata/GDT adapter at0x110000 and PCI adapter at0x111000. External osenv places
literal bytes into disjoint ELF32 segments and note18; no compiler, assembler,
linker, imported instruction/header bytes or generated instruction selection.
The separate full BIOS disk chain remains mandatory.

Prerequisites: oneCPU i440fx/PIIX3, exactly one root function0 Intel82540EM/82574,
actual128KiB BAR0, declared low RAM through0x96000 and usable1–2MiB, bounded
metadata below4MiB and unoccupied0xc0000000..0xe0000000 aperture. The adapter
checks metadata counts/reserved fields/overflow, usable intervals, actual kernel
FNV, CPUID/MSR support, BAR probe/assignment and interrupt routing. It assigns
aligned0xc0000000, verifies reads and INTA swizzle/PIRQ/ELCR IRQ11, then enters
shared CPU/device initialization. Unsupported boards/configurations halt.

Adapter sizes:382+600 bytes. The9,768-byte direct ELF contains the8,568-byte
kernel plus loader metadata; packed BIOS storage stays8,192 bytes. Acceptance
requires19 actual metadata/CPU/PCI cases, a rejected magic-check bypass mutant,
both NIC wire paths and complete responses, matched timing and exact homelab.
Evidence and final artifact identity are in the raw README and external
osenv/local/t023. Moving BAR to0xc0ba0000 to test a QEMU software-TLB alias did
not establish a gain and was rejected. No physical cache benefit is claimed.

This is cold direct VM entry, not installation into a CPU. Power-up/RESET
invalidates caches/TLBs; INIT differs but does not preserve a running execution
environment. Powered resume and snapshot restoration have separate lifecycle
scopes; arbitrary peer/DMA/clock/lease recovery after suspension is unimplemented.
Physical firmware installation requires identified board/DRAM/chipset/NIC
bring-up and a recoverable flash procedure.

Provenance: project-authored literal encodings from Intel SDM and documented
[Xen ABI](https://xenbits.xen.org/docs/unstable/misc/pvh.html),
[start-info interface](https://github.com/xen-project/xen/blob/master/xen/include/public/arch-x86/hvm/start_info.h),
[QEMU loader](https://www.qemu.org/docs/master/system/generic-loader.html),
[monitor snapshots](https://www.qemu.org/docs/master/system/monitor.html) and
[Intel SDM](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html).
Firmware is hashed external infrastructure, not our guest implementation.
