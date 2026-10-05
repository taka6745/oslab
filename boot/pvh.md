# Optional PVH entry

`make pvh-prod` builds separate `../osenv/build/oslab-pvh-prod` artifacts;
`make pvh-debug` builds `../osenv/build/oslab-pvh-debug`. Both retain the full
BIOS disk image and authored-source dependency checks. BIOS disk boot remains
the mandatory integrated acceptance path.

The project-authored ELF32 entry follows the
[Xen x86/HVM direct boot ABI](https://xenbits.xenproject.org/docs/4.10-testing/misc/pvh.html):
a physical entry note, protected-mode CPU, and EBX pointing at versioned loader
metadata. It bounds the actual map pointers/count, checks memory-range overflow,
converts records for the existing allocator, validates kernel integrity and CPU
features, builds the same long-mode identity map, and enters the actual kernel.
A GDT copied to reserved `0x8000` survives kernel BSS clearing and keeps hidden
segment descriptors usable during later IRQs. Firmware is recorded external
emulator infrastructure, not project-authored guest code.

This optional board is minimal QEMU `pc-i440fx-9.2`, one CPU, 64 MiB, headless,
with Intel 82540EM/82574 legacy-interface NIC and external qboot. qboot supplies
actual memory metadata but leaves NIC MMIO BARs unassigned. The PVH-enabled
driver probes actual BAR size with decoding disabled, checks the declared
`0xc0000000..0xe0000000` aperture against all loader records, rejects competing
assigned memory BARs/bridges, and verifies assignment. It configures the actual
PIIX3 PCI interrupt route and level-triggered PIC input; LAPIC/x2APIC are disabled
for this PIC-based board. This is a bounded board contract, not a general PCI
resource allocator or support for arbitrary firmware/hardware.

`make pvh-preload-prod` separately builds `../osenv/build/oslab-pvh-preload`. Its
`pvh-inputs.json` declares `preload: true` and the source-verified kernel hash.
The external controller loads that exact raw kernel at `0x100000` through
[QEMU's generic loader](https://www.qemu.org/docs/master/system/generic-loader.html)
without changing the CPU entry point. The small adapter still checks the actual
loaded kernel hash; it only omits embedded payload/copy. Normal PVH defaults to
an embedded payload. Both use external qboot and a fresh reset CPU, rather than
an OS snapshot or warm service resume.

Matched local measurements reduced reset-release to full verified HTTP from
59.2 to 21.5 ms; whole controller launch remained about half a second. Preload
showed no convincing additional gain. See [performance](../kernel/performance.md)
for timing boundaries and retained evidence. Both variants passed nine actual malformed/map/kernel boundary cases and
production wire gates; PVH debug passed the two-delivery NIC IRQ gate. The separate full BIOS and packed-decoder gates passed.
Less than 5 ms and physical/homelab validation remain unachieved.
