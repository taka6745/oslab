# oslab

Project-authored x86-64 BIOS OS with a static HTTP server. All guest code,
drivers and headers live here; no external guest code or libraries are linked.
[INTEGRITY.md](INTEGRITY.md) and [AGENTS.md](AGENTS.md) are mandatory.

## Build and test

LLVM/LLD 23.1.2 and NASM 3.02; Python 3.12+ for the separate
[osenv](https://github.com/taka6745/osenv) controller. Build outputs, symbols,
VMs, logs, captures and private configuration belong there.

```sh
make prod                  # ../osenv/build/oslab-prod
make debug                 # ../osenv/build/oslab-debug
make web-debug PROFILE=1   # compact server with diagnostics/profiling
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
exceptions, PIC/PIT, PCI and Intel 82540EM DMA/receive interrupts. Compact
web builds poll briefly before sleeping to catch short packet bursts. Authored
Ethernet/ARP/IPv4/UDP/DHCP/TCP; debug also provides ICMP and DNS A/CNAME/HTTP.
Device and network configuration comes from actual hardware and packets.

The 1,366-byte HTML/CSS page plus headers fits one 1,460-byte TCP payload.
One connection at a time: GET `/`, 404/405/400 errors, bounded 768-byte headers,
Content-Length, retransmission, MSS/window limits and a 10-second deadline.
No keep-alive, TLS, request bodies or concurrent clients. DHCP expiry stops
serving until restart or debug renewal. Out-of-order data requires retransmission.
No IPv6, DNS-over-TCP, jumbo frames, other NICs, processes, filesystem, kernel
disk I/O, SMP or hardware acceleration.

Production uses LTO/-Oz, a 16 KiB stack and eight RX/TX descriptors with two
shared TX buffers. The packed disk image is 12,800 bytes (raw: 14,336).
[Boot packing](boot/packing.md) changes disk size only; expanded code is identical.
[Performance](kernel/performance.md) records actual timing, traffic and test
results; [TASKS.md](TASKS.md) retains implementation history and provenance.

Local checks exercise disk boot, malformed images, memory exhaustion, faults,
hangs/recovery, absent devices, DMA errors, packet/HTTP sanitizers and deliberate
mutants. External clients and packet reconstruction verify actual responses.
These checks do not establish physical throughput or cache residency. Current
homelab SSH timed out. [Pi 4 bring-up](arch/aarch64/README.md) works in emulation;
**Pi Ethernet and serving remain unimplemented.**
