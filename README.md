# oslab

A project-authored x86-64 BIOS operating system for a dedicated homelab VM.
All guest boot code, kernel code, drivers and network protocols are authored here.
No external driver, networking library, guest service or vendored code is linked.

Implemented: two-stage disk boot with integrity checks, E820-derived physical-page
allocation/freeing, long-mode paging, CPU exception diagnostics, PIC/PIT and serial,
PCI discovery and an Intel 82540EM DMA-ring driver. The IPv4 client stack implements
Ethernet, ARP, ICMP echo replies, UDP, DHCP, DNS A/CNAME parsing and a bounded TCP/HTTP
client with retries, sequence/checksum validation and orderly connection close.
NIC configuration, MAC addresses, DHCP values and DNS answers come from the device
and network, not baked-in responses or environment-specific addresses.

The OS supports one CPU and one client connection at a time. IPv6, TLS/HTTPS,
DNS-over-TCP, jumbo frames, other NIC models, processes, filesystems, kernel disk
I/O, SMP and hardware acceleration are not implemented. Out-of-order TCP data is
acknowledged at the current receive position for peer retransmission, not buffered.
This is a bootable kernel with networking, not a complete general-purpose OS.

Build definitions belong here. VM controllers, sockets, disks, logs, packet
captures and private host configuration belong in the separate
[osenv](https://github.com/taka6745/osenv) checkout. `make` defaults to writing its
image and all three symbol files there. LLVM/LLD 23.1.2 and NASM 3.02 are pinned by
the external build controller; QEMU/GDB are host tools, not shipped guest code.
The host-only parser test uses the compiler's sanitizer/runtime infrastructure;
none of that runtime is linked into the kernel.

From the external osenv checkout, with Python 3.12+:

```sh
python3 -m osenv project-build --project ../oslab
python3 -m osenv project-test --project ../oslab
# Explicit opt-in live Internet verification, kept separate from the default gate:
python3 -m osenv project-test --project ../oslab --internet-host example.com
python3 -m osenv project-deploy --project ../oslab --config local/deploy.json --internet-host example.com
```

Deploy returns an ID for status/logs/wait. Each run retains the exact image, source
and tool hashes, ELF files, serial output, commands and external verdict. Manual
runs accept `--image ... --symbols ... --mode long64 --memory 64 --network internet`.
Networking uses the emulator's outbound NAT transport; the guest itself performs
NIC DMA and every packet/protocol operation. The default gate has no Internet.

The default build is diagnostic (`DEBUG=1`), including test/fault commands.
`make DEBUG=0 OUT=../osenv/build/oslab-release` builds a separate image with
self-test and deliberate fault/hang commands compiled out. That variant must
receive its own exact-image acceptance before any release claim.

Serial commands in the diagnostic build: `selftest`, `dhcp`, `resolve HOST`, `http HOST /PATH`, `stats`,
`fault`, `pagefault`, `hang`, `exit`. Commands are bounded and emit `OSL1` versioned records.
`exit` reports the preceding command result through debug-exit. Fault/hang commands
exercise actual exception/halt paths; external inspection remains available even
when guest services fail. Panic records include vector, error, RIP, RSP and CR2.

Tests cover complete disk boot, real memory exhaustion and boundary/free checks,
corrupt stage/kernel payloads, truncated disks, #UD/#PF fault capture, hang capture and
recovery, missing NIC, link down and absent DNS. The exact guest parser source is
checked on the host with ASan/UBSan, regressions, 100,000 random and 20,000 structured
seeded inputs; an intentionally disabled checksum check must fail those tests.
Live acceptance compares received HTTP lengths/hashes against independently
reassembled TCP packet captures. Performance results are stored with each run;
TCG boot/request timings are not physical-hardware throughput claims.

[AGENTS.md](AGENTS.md), [INTEGRITY.md](INTEGRITY.md) and [TASKS.md](TASKS.md) define
scope, mandatory source rules and verification status. CI enforces the OS-only
file boundary and the deterministic full disk-boot gate.

Specifications consulted (no source code imported):
[Intel 8254x device manual](https://www.intel.com/content/dam/doc/manual/pci-pci-x-family-gbe-controllers-software-dev-manual.pdf),
[DHCP](https://www.rfc-editor.org/rfc/rfc2131),
[DNS](https://www.rfc-editor.org/rfc/rfc1035),
[TCP](https://www.rfc-editor.org/rfc/rfc9293).
