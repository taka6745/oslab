# Tasks

T001 — Goal: keep this repository exclusively for OS development. Interface:
external osenv CLI; mandatory AGENTS.md and INTEGRITY.md. Prerequisites: separate
osenv checkout and source integrity audit. Acceptance: no harness submodule,
wrapper, fixtures or generated evidence tracked or left in this working tree;
external audit with --os-only passes and is enforced by CI using a pinned
external harness revision. Result: harness removed; historical evidence
preserved privately outside this checkout. OS code was absent at this separation milestone.

T002 — Goal: implement a complete project-authored disk boot chain. Interface:
BIOS disk image and matching ELF symbols for external osenv inspection.
Prerequisites: x86-64 toolchain, source integrity rules and external harness.
Acceptance: real full disk boot with externally verified markers, load addresses,
failure diagnostics and exact-image homelab validation. Result: implemented; diagnostic image fully disk-boots locally and on the
homelab, with matching stage1/stage2/kernel ELF symbols and fault/hang recovery.

T003 — Goal: enforce the OS-only repository boundary. Interface: CI tracked-file
allowlist and OS-focused agent contract. Prerequisites: harness already external.
Acceptance: inventory contains only OS contracts/CI; policy rejects controller,
fixture, library, runtime-evidence and private-config paths while accepting OS
source/build paths; external source audit passes. Result: local boundary checks
and source audit pass; boundary remains enforced as guest source is added.
CI result is recorded on the commit.

T004 — Goal: real Internet connectivity through project-authored guest code.
Interface: PCI e1000 DMA driver, Ethernet/ARP/IPv4/UDP/DHCP/DNS/TCP/HTTP and serial
commands. Prerequisites: disk boot, exceptions, physical allocator, PIT timer.
Acceptance: clean/no-NIC boot; host parser sanitizer/fuzz tests; malformed frames
rejected; DHCP-derived configuration; DNS and TCP response from an external host
with packet evidence; exact-image homelab verification; measured size/boot/request
latency without extrapolating TCG to hardware. Provenance: authored from Intel
8254x register/descriptor specification and IETF protocol specifications; no driver
or stack source imported. Result: 17 actual-OS cases passed locally and on the homelab, including five
DNS/TCP/HTTP requests with lengths/hashes independently verified from pcap. Host
ASan/UBSan regressions and 120,000 seeded inputs pass; disabling checksum validation
is rejected. The external harness's 18 checks still pass. Diagnostic image hash:
688b278f67be25c3129251ed92bfba1f002a607d569a1a165161672e585d17d8.
Measured locally under single-CPU TCG: 29,696-byte image, 24,598-byte kernel,
0.520-second median launch-to-ready (three runs), HTTP transfer times
31/34/29/28/29 ms. These are emulation/Internet observations, not hardware throughput.
DEBUG=0 separately compiles out deliberate fault/hang/self-test commands; compile
and local rejection of its fault command checked, not homelab release-verified.
