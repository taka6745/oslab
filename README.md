# oslab

Project-authored x86-64 OS with BIOS disk boot and a static HTTP server.
All guest code, drivers and headers live here; no external guest code or libraries are linked.
[INTEGRITY.md](INTEGRITY.md) and [AGENTS.md](AGENTS.md) are mandatory.

## Build and test

LLVM/LLD 23.1.2 and NASM 3.02; Python 3.12+ for the separate
[osenv](https://github.com/taka6745/osenv) controller. Build outputs, symbols,
VMs, logs, captures and private configuration belong there.

```sh
make prod                  # ../osenv/build/oslab-prod
make debug                 # ../osenv/build/oslab-debug
make web-debug PROFILE=1   # compact server with diagnostics/profiling
make pvh-prod              # optional pc-i440fx/qboot route, separate output
make machine-prod MACHINE_HTTP=1 # separate hand-encoded hybrid variant
make machine-pvh-prod MACHINE_HTTP=1
make machine-host-test     # raw-byte oracles, guard pages and branch limits
cd ../osenv
python3 -m osenv project-build --project ../oslab
python3 -m osenv project-test --project ../oslab
# Optional real Internet check:
python3 -m osenv project-test --project ../oslab --internet-host example.com
```

Production boots, obtains DHCP and serves TCP80. It has no guest UART, debug
commands, counters, outbound DNS/HTTP client, ICMP service or SSH. Debug retains
serial `OSL1` diagnostics, fault/hang tests and optional profiling. Host QMP/GDB
inspection needs no guest network. `DEBUG=0` alone is not a production build.

## Implemented and limited

Two-stage integrity-checked disk boot, long-mode paging, E820 page allocation,
exceptions, PIC/PIT, validated HPET time, PCI and Intel 82540EM/82574 legacy
DMA/receive interrupts. Compact web builds poll briefly after recent RX activity, then sleep atomically.
The optional osenv `--nic-model e1000e` selects the emulated 82574; it boots
faster locally, with comparable serving throughput. Optional
[PVH boot](boot/pvh.md) reduces measured reset-release to verified HTTP from
59.2 to 21.5 ms; complete BIOS disk boot remains mandatory. Authored
Ethernet/ARP/IPv4/UDP/DHCP/TCP; debug also provides ICMP and DNS A/CNAME/HTTP.
Device and network configuration comes from actual hardware and packets.

The 1,366-byte HTML/CSS page plus headers fits one 1,460-byte TCP payload.
One connection at a time: GET `/`, 404/405/400 errors, bounded 768-byte headers,
Content-Length, retransmission, MSS/window limits and a 10-second deadline.
No keep-alive, TLS, request bodies or concurrent clients. DHCP expiry stops
serving until restart or debug renewal. Out-of-order data requires retransmission.
No IPv6, DNS-over-TCP, jumbo frames, other NIC models, processes, filesystem, kernel
disk I/O, SMP or hardware acceleration.

Production uses LTO/-O3, a 16 KiB stack and eight RX/TX descriptors with two
shared TX buffers. The packed disk image is 16,384 bytes (unpacked layout: 22,016).
[Boot packing](boot/packing.md) changes disk size only; expanded code is identical.
[Performance](kernel/performance.md) records actual timing, traffic and test
results; [TASKS.md](TASKS.md) retains implementation history and provenance.

T019 local integrated, loader-boundary and production wire gates passed.
Local checks exercise disk boot, malformed images, memory exhaustion, faults,
hangs/recovery, absent devices, DMA errors, packet/HTTP sanitizers and deliberate
mutants. External clients and packet reconstruction verify actual responses.
These checks do not establish physical throughput or cache residency. Exact
production and optional PVH images passed dedicated homelab QEMU wire checks;
production also passed 1,000 response/boundary/timeout tests. [Pi 4 bring-up](arch/aarch64/README.md) works in emulation;
**Pi Ethernet and serving remain unimplemented.**

Hand-encoded variants retain the readable sources. [Machine code](kernel/machine/README.md)
covers entry, interrupt stubs/table, checksums, memory operations and complete HTTP
parsing; [PVH adapter](boot/machine/README.md) is separately encoded too. Drivers,
TCP/IP, memory management and response data remain project-authored C. These
optional builds use explicit opcode bytes and checked symbolic relocations,
with the same safety checks; they do not represent a complete OS in hundreds of bits.
