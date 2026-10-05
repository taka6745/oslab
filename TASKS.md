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

T005 — Goal: prevent source from outside this repository entering the OS image.
Interface: repository-authored ABI types, -nostdinc, resolved C/NASM dependency
check and saved source-inputs.json. Prerequisites: existing freestanding build.
Acceptance: guest inputs all repository-owned; deliberate external header/include
rejected; no undefined kernel symbols or external libraries; host sanitizer tests,
actual OS boot gate and exact-image homelab verification. Result: source authored
here; 21 repository-owned source/build inputs recorded; actual external-header
negative build rejected; no undefined kernel symbols; sanitizer regressions and
120,000 seeded inputs passed; source audit and all 17 local OS checks passed.
Image SHA256 remains 688b278f67be25c3129251ed92bfba1f002a607d569a1a165161672e585d17d8,
identical to the previously homelab-verified image. Updated source gate has not
yet run in hosted CI.

T006 — Goal: minimal static HTTP serving and measured cache/boot budget.
Interface: TCP80 GET /, bounded 768-byte request, serial serve command, make web.
Prerequisites: real DHCP/ARP/IPv4/TCP and NIC DMA. Provenance: authored here from
HTTP/TCP interfaces; no code or libraries imported. Acceptance: real external
HTTP client, fragmented requests, status routing, aborted-request recovery,
retransmission and DMA ownership, exact-image boot gates, footprint/timing evidence.
Result: 39 real requests passed on the compact release profile, including split
headers, 404/405/400 and recovery. Host HTTP and seeded packet sanitizer tests passed.
Integrated diagnostic gate passed 17 tests, including live DNS/HTTP. Actual external Ethernet peer verified
128-byte MSS, dropped-ACK retransmission, zero-window reopening, out-of-order
request recovery, sequence wrap, corrupt TCP rejection and ARP contention; external exit code 33, no panic/reset.
HTTP regressions and 30,000 seeded malformed inputs passed; wrong-status mutant
was rejected. Disabling the guest TCP checksum check was also rejected, with
a complete CPU/RAM failure capture preserved externally. Compact image 19456 bytes; text 12,433 bytes, rodata 1,984,
data 22, BSS 25,760 (includes 16 KiB stack and 4 KiB allocator maps).
DMA allocation adds 28 KiB (20 KiB buffers plus 8 KiB descriptor pages).
Full data allocation exceeds Pi 4 L1D; runtime plus boot page tables fits an
L2 capacity budget, not verified cache residency. Median cold launch-to-serving
0.574 seconds over three isolated-DHCP TCG runs. Release image SHA256
0e2d4a6542fa09d45fffdc06862ffa5e98566704212e0d42c00d3f2b2e13555a.
Physical serving, cache misses, exact-image homelab release and hosted CI are
not verified. Provisional Pi target needs model confirmation and hardware access.

T007 — Goal: Raspberry Pi support, provisional Pi 4 wired target pending model/access.
Interface: authored ARM64 firmware entry, PL011 serial, timer/cache registers and
shared portable HTTP code. Prerequisites: board model, native Ethernet driver,
DMA/cache coherency, physical serial/network access. Acceptance: real boot and
external page fetch on that board; physical boot-to-serve and PMU cache measurements.
Result: Pi 4 QEMU bring-up verified EL1, advancing timer, UART query and cache geometry;
ARM64 network stack cross-compiled. Native Ethernet, MMU/cache enablement and physical
verification remain unimplemented. Bring-up reports unsupported networking explicitly.
An existing configured Pi SSH endpoint was discovered but refused the connection;
model confirmation and a usable physical test path remain pending.

T008 — Goal: stress actual web serving, boundary recovery and measured idle work.
Interface: optional PROFILE=1 serial perf/perf-reset; external seeded socket stress.
Prerequisites: real disk image, DHCP/TCP, pinned TCG and external packet peer.
Provenance: counters and DMA layout authored here; host load generation in osenv.
Acceptance: exact response checks, 768-byte boundary, malformed requests, recovery,
loss/reordering peer gate, sanitizer/integrated boot; compare footprints and idle/load.
Result: 5,000 release requests plus 1,000 profiling requests passed with exact
response comparisons, seeded fragmentation, 767/768 acceptance and 769 reset,
six malformed-header regressions and slow-client timeout recovery. Final wire
loss/reordering/checksum/window/wrap gate passed; all 16 integrated OS checks,
150,000 sanitizer inputs, eight harness tests and source audits passed locally.
Stress found a real back-to-back FIN/SYN batch race; release the completed slot
inside receive. The previous image fails this regression; CPU/RAM captured.
Compact DMA rings now share aligned storage: 24 KiB total DMA, saving 4 KiB.
Release text 12,481 bytes; image 19,456 bytes. Single-CPU real-time TCG release:
225.85 requests/s, median 4.65 ms, p99 5.32 ms (5,000 samples). Profiling run:
99.73% idle / 94.11% loaded ticks across halt-through-interrupt-return; normal
release compiles counters out. No RX errors/TX-full; external exit 33, no panic.
Release image SHA256 26fab01194adb3d016368307b6de21e23f1f2fb0718b07dad89aa9f8b9b8fded.
TSC ticks include interrupt return overhead; virtual-clock acceptance is separate
from real-time throughput. Retired instructions, physical cycles/cache misses,
stack high-water, concurrent serving, Pi Ethernet and exact-image homelab release
are not verified. These measurements do not establish an exhaustive optimum.

T009 — Goal: improve the static page and shrink the image without losing features.
Interface: responsive embedded HTML/CSS, two-sector stage two, optional LTO.
Prerequisites: existing disk chain and single-connection HTTP server.
Provenance: page and boot layout authored here; no external guest assets/code.
Acceptance: one-MSS response, desktop/mobile browser review, real disk and corrupt
image checks, peer loss/boundary checks, sanitizer and before/after stress timings.
Result: authored 1,366-byte responsive HTML/CSS page; full HTTP response 1,444
bytes, within one normal TCP payload. Desktop and 375-pixel mobile browser review
passed with no overflow; section navigation works. No external assets or code.
Stage two shrank from 4,096 to 1,024 bytes; full-payload checksums and retries
remain. Harness derives kernel disk offset from actual stage-two size.
Final web image 16,384 bytes versus 19,456 (15.79% smaller); text 11,777,
rodata 2,773, data 22, BSS 25,760 and DMA 24 KiB. Full diagnostic image 28,160
versus 33,792 (16.67% smaller), with all commands and outbound networking retained.
Compared -Os without LTO (17 KiB), -Os/LTO (16.5 KiB), -Oz/LTO (16 KiB).
Selected -Oz/LTO for web; -O2/LTO enlarged the diagnostic image, so it keeps LTO off.
Two 5,000-request new-page runs passed at 225.07 and 225.73 requests/s, versus
baseline retest 227.41. Final median 4.67 ms, p99 5.42 ms; throughput within 1%
of baseline in these local TCG runs, not a physical performance guarantee.
Final wire loss/reorder/window/wrap/checksum/ARP gate, 150,000 sanitizer inputs,
eight harness tests, source audits and all 17 integrated OS checks passed,
including actual outbound Internet and corrupt/truncated disk boot. No undefined
guest symbols. Web SHA256 ae4441445c87b19eb7c95b4c0c1b2a91f9e9898bdb73dae88dd6010aa4d55d83.
Diagnostic SHA256 332c8cb9ba2df3c264482f7afc528870fde878a090d9e847e6d1689a1037d163.
Physical/Pi serving, exact-image homelab release and hosted CI remain unverified.

T010 — Goal: verify remaining compression opportunities on exact release images.
Interface: external compression/round-trip report by image, kernel, ELF section
and page; no imported codec or guest runtime dependency.
Prerequisites: saved web and diagnostic image hashes, symbols and host codecs.
Acceptance: confirm exact hashes/sizes, byte-identical decompression for each
algorithm; distinguish archive size, bootable-layout budget and runtime costs.
Result: verified the exact T009 web/diagnostic image SHA256 values. External
host codecs tested gzip, bzip2, XZ/LZMA, Zstandard and LZ4 at nine settings across
16 actual inputs: images, kernels, boot stages, ELF text/rodata/data and linked
HTML/HTTP bytes. All 144 byte-identical round trips, repeat-encoding comparisons
and truncated-stream rejection checks passed. Reports and compressed artifacts
remain in osenv/local/compression-check; no guest codec/code imported.
Web archive: raw 16,384; gzip 10,723; XZ/16-KiB dictionary 9,944; LZ4-9 12,929.
Diagnostic archive: raw 28,160; gzip 16,716; XZ/16-KiB 15,344; LZ4-9 20,081.
Web kernel: raw 14,678; gzip 10,028; XZ/16-KiB 9,276; LZ4-9 12,167.
HTML: raw 1,366; gzip 847. Tiny 22-byte data section grows under every codec.
Compression confirms redundancy remains chiefly in code and read-only data.
Keeping the current 1,536 raw boot bytes plus XZ-compressed kernel gives 11,264
sector-rounded bytes before an authored decoder and changed boot metadata. This
is a storage budget, not a bootable image. Installed liblzma reports 82,552 bytes
for the small-dictionary kernel stream versus 67,175,032 for default -9e; guest
decoder footprint and speed are not implemented or measured. Expanded runtime
RAM/cache footprint is unchanged. HTTP gzip needs negotiation and an identity
fallback, which adds storage/code while both responses already fit one segment.
Original boot images remain byte-identical and unmodified. Source audits pass;
no compressed-boot or performance improvement is claimed.

T011 — Goal: strict separate production/debug images; production only boots/serves.
Interface: make prod/web, make debug and web-debug; production has no guest UART
command/log interface, fault/test commands, counters or outbound HTTP/DNS client.
Prerequisites: authored disk chain/network server and external-only acceptance.
Acceptance: production symbols/strings audit; serial-command injection ignored;
HTTP survives boundaries/loss and port probes; debug commands/boot gates still work.
Result: make prod/web writes oslab-prod; make debug writes oslab-debug, with
web-debug for compact instrumented serving. Production enforces DEBUG=0,
PROFILE=0, WEB_ONLY=1 and AUTOSERVE=1 at compile time; conflicting debug and
profiling configurations were rejected by real negative builds. Boot UART,
serial input/output, command parser, fault/self-test commands, profiling and NIC
statistics, outbound DNS/HTTP client and ICMP echo are absent from production.
Panic/boot failures fail-stop without guest diagnostics; checksums remain active.
No SSH/remote-administration guest service exists in either image. ELF symbols
and host QMP/GDB remain external infrastructure, absent from the raw disk image.
Production symbol/string audit passed: no console/debug symbols or OSL1 text,
boot UART routines or undefined symbols. Actual serial-command injection had no
effect; 5,000 exact-response requests, size boundaries, malformed headers and
slow-client recovery passed. Wire tests passed MSS/loss/window/reorder/wrap/bad
checksum cases; ports 22/23/443/2222/8080/12345/65535 were silent, and ICMP echo
was not exposed. Three actual corrupted/truncated production images halted at
expected boot-stage addresses with complete external CPU/RAM captures. Successful
production tests terminate the VM externally; there is no guest debug-exit command.
Production image 13,824 bytes; text 9,845, rodata 2,133, data 14, BSS 25,696.
Local real-time TCG: 226.36 requests/s, median 4.64 ms, p99 5.26 ms; launch to
first observed HTTP response 1.50 s, not a physical board timing claim.
Production SHA256 e526153e00259998af5f35b01e5991fa31cd4399a8c74d642a51f01504f5659c.
Exact debug/profile image passed all 17 integrated checks including outbound
Internet, plus perf/stats/exit; compact debug wire gate, 150,000 sanitizer inputs,
eight harness tests and both source audits passed. Debug image 28,672 bytes.
No physical/Pi production serving, exact-image homelab release or hosted CI claim.

T012 — Goal: further binary reduction with an authored boot-only codec.
Interface: BOOT_COMPRESS=1 encodes our kernel through boot/pack.c (host build
transform), stage2 bounds-checks/expands it, then checks the original kernel hash.
Prerequisites: current production boot chain; decoder fits stage-two allocation.
Acceptance: real compressed boots; loaded bytes identical to ELF/raw kernel;
malformed streams fail-stop, debug regression gate and production wire/stress.
Result: implemented and locally tested. Default prod compresses; default debug
does not. Image 13,824 -> 12,288 bytes (11.1% smaller); raw kernel 12,046 ->
10,638 encoded bytes. Minimum-cost encoding saved another 74 bytes over greedy
parsing (sector-rounded image size unchanged). Decoder fits the unchanged
1,024-byte stage two. All
12,046 expanded bytes matched the original at the actual booted kernel entry.
Fourteen ASan/UBSan encoder round trips covered 1..524,288 bytes and token/window
boundaries; empty/oversize inputs were rejected. Thirty-two independent
exhaustive short-input cases confirmed minimum encoding size for this format.
Five real malformed disk images
(zero/back-before-output offset, output overflow, truncated literal and decoded
hash mismatch) halted in the actual decoder failure path, preserved the output
guard, and had full CPU/RAM captures saved externally. No external codec code.
Exact default prod wire gate passed, including loss/window/ordering/wrap, silent
non-HTTP ports and UART. Compressed 5,000-request stress/boundaries/recovery
passed on the final image: 259.3 req/s, median 4.01 ms, p99 4.66 ms, first
HTTP 1.56 s (TCG). Initial raw and compressed comparisons timed out during
load; failure inputs, pcap and CPU/RAM remain retained. Captures showed
abandoned 100ms readiness clients retransmitting SYNs 6/18 seconds later and
taking the single connection slot during load. The external stress harness now
waits for an actual captured DHCP ACK and makes one readiness HTTP request
within its original deadline; load assertions/timeouts are unchanged. New
readiness parser tests cover ACK/offer, wrong ports and truncated live records.
With corrected readiness, raw 5,000-request comparison passed at 253.9 req/s
and first HTTP 1.54 s; compressed repeated 10,000 requests with a different
seed also passed, including boundaries and slow-client timeout recovery.
These NAT/TCG timings do not establish a physical speed improvement or
exhaustive reliability.
All 17 debug OS checks, packet/HTTP sanitizers, ten harness tests and source
audit passed. Encoded payload joins the external build hash manifest. No
physical/Pi serving, exact-image homelab verification or hosted CI claim.

T013 — Goal: measure boot, serving and captured network cost; reduce avoidable
transfer/latency without removing safety checks or regressing performance.
Interface: external per-phase socket samples/pcap accounting; production TCP80.
Prerequisites: strict prod/debug builds, actual packet peer and packed boot gate.
Acceptance: matched baseline/candidate runs, wire loss/window/close tests,
sanitizers, integrated debug boot and exact final production-image stress.
Result: locally verified final image 12,800 bytes, SHA256
51dac8649eee65fcdc0c913cd62fac3db2f033b373b8b8307da998fb30ce23fd.
Two seeded 5,000-request production runs: 681 requests/s combined, median
1.655 ms, p99 2.991 ms; launch-to-first-response 1.534 s, essentially unchanged.
Receive IRQ wakeups, ACK piggyback and window-safe data/FIN reduce median captured
frames 11 -> 8 and Ethernet bytes 2,123 -> 1,959 per connection. The unchanged
1,366-byte page has a 94-byte header with derived Content-Length, fitting one
1,460-byte payload. Image grew 512 bytes versus the previous release to retain
performance and safer framing. A copy-removal candidate was reverted after worse
latency; disabling the NIC ROM did not improve boot. Details: kernel/performance.md.
Passed: 150,000 sanitizer cases; 17 integrated debug checks; 18 harness gates;
13 harness unit tests; production/debug wire peers; two real IRQ deliveries;
14 codec roundtrips, 32 optimal-size oracles and five malformed disk boots.
Standard HTTP client rejected a deliberately truncated actual response. The
zero-window test rejected the preceding faulty image before the fix. Pi emulated
bring-up passed; Pi Ethernet is unimplemented. Configured homelab SSH timed out;
physical performance and exact-image homelab deployment remain unverified.


T014 — Goal: remove redundant documentation/comments without losing behavior.
Interface: concise README, unchanged guest instructions and development contracts.
Prerequisites: T013 release evidence. Acceptance: comments-only guest diff, identical rebuilt release image,
source audit and diff checks. Provenance: edited
project-authored text only. Result: README reduced to 55 lines; packing documentation
37 -> 26; redundant code comments shortened. Production rebuild remains exactly
SHA256 51dac8649eee65fcdc0c913cd62fac3db2f033b373b8b8307da998fb30ce23fd.
No executable code or safety checks removed. Source audit/diff checks passed.


T015 — Goal: measure compiler, interrupt moderation and copy-path candidates.
Interface: existing production TCP80 and externally saved real-image stress.
Prerequisites: T013 baseline; authored driver; pinned TCG configuration.
Acceptance: repeated matched measurements, unchanged response/wire correctness,
relevant actual-image gates for retained guest changes. Provenance: Intel AP-453
and optimization manual; no reference code imported. Result: 40,000 real requests
over 20 runs, boundaries and timeout recovery passed. Only bounded 32-iteration
pre-sleep polling retained in compact web builds: paired combined throughput
621 -> 1,218 requests/s, median 1.678 -> 0.341/0.353 ms, idle halt fraction
99.84% -> 98.87% in separate profiling. TCG-only, not physical cache/power data.
Higher ITR slowed throughput; -O2/-O3, REP copies, larger rings and checksum
unrolling did not justify retention. Current 12,800-byte image SHA256
c7dd4825473bdbcd471283db1af722b25780342efc87a67e58d2f9e5e7e1e80c.
Final standard-client/truncation, production/debug wire, integrated boot/fault/
recovery, 150,000 sanitizer cases and exact-image packed-boot checks passed.
Pushed starting OS/harness commits 3d2cb47/a577d08 both passed hosted CI.
SMP/per-core queues and bonding require new authored subsystems and hardware
validation; neither is implemented by merely selecting more emulator CPUs.


T016 — Goal: lower boot latency and verify minimum TCP exchange cost.
Interface: optional 82574 legacy NIC; external pcap/QMP phase measurements.
Prerequisites: T015 image, authored DMA/IRQ stack, real packet peer.
Acceptance: actual five-frame close; repeat cold boots/load, wire loss/window/
IRQ tests, sanitizers, packed-image and integrated gates. Provenance: RFC 9293,
Intel 82574 datasheet and QEMU timing documentation; no code imported.
Result: optional 82574 support shares our legacy driver; full cold launch-to-
response 520/530 ms vs 1527/1523 ms on 82540EM. Throughput 914/900 vs 1210/1192
requests/s, so 82540EM remains default. Image remains 12,800 bytes, SHA256
fa8913dfef42b4f3e97dae14f10dcbcfc9600e9f331a9741999e31cc17ff6791.
Five-frame capture verified in production/debug when the client combines
ACK+GET+FIN; ordinary clients still use eight. Three prior-image QMP probes
measured BIOS-to-first-DHCP-send 68.6–69.6 ms; this excludes host setup/DHCP.
The e1000 emulator gates receive for one second after RCTL initialization.
Final wire/IRQ, integrated and codec gates passed; external harness/unit checks
passed. Physical board timing and exact-image homelab remain unverified.

T017 — Goal: profile boot/idle/serving and retain measured bottleneck fixes.
Interface: bounded packet-burst polling; authored external instruction counters.
Prerequisites: T016 release, pinned single-CPU QEMU, exact ELF and packet captures.
Acceptance: seeded repeated production comparisons, independent instruction
counter agreement, idle/load graphs, source audit, sanitizers and real wire/
boot/fault/recovery gates. Provenance: Intel optimization manual and QEMU plugin
API; no imported guest code. Result: 110,000 production responses verified.
Retain recent-RX-gated 256 checks, REP word copies plus tails, and 8-byte checksum
unroll. Final 10,000 requests: 6474/s, median 143 µs, p99 231 µs, boot 1511 ms;
12,800-byte image SHA256
728d4e2074bce8d62794c92d7bf60de74f85a3326ec3681878a56724b4bc3237.
Debug serving dispatches/request fell 53402→12692; idle outside halt 0.18%.
Counter totals agree independently. Both NIC wire suites, integrated boot/fault/
recovery, decoder, sanitizer/oracle/guard-page and deliberate-defect checks passed.
Source audits, 18 external harness gates and 15 unit tests passed. Graphs and
raw data stay in osenv. Physical cycles/cache behavior and homelab remain unverified.

T018 — Goal: reduce boot-to-serving and validate serving candidates.
Interface: external controlled boot/HTTP benchmark and QMP/GDB milestone probe.
Prerequisites: T017 exact release, real DHCP/TCP, matching seeds and image hashes.
Acceptance: real repeated boots/load, packet boundaries, failed-breakpoint safety,
source integrity and external controller gates. Provenance: own experiments and
Cloudflare isolate-startup documentation; no guest code imported.
Result: retain T017 guest bytes, SHA256
728d4e2074bce8d62794c92d7bf60de74f85a3326ec3681878a56724b4bc3237.
82574 without unused PXE ROM: reset-to-first-HTTP 72–86 ms (median 73.4), full
controller launch about 550 ms. Kernel entry 61.6 ms; first DHCP send 64.4 ms
in separate QMP probes. Final matched runs ~6423 requests/s, median 146 µs,
p99 225/232 µs. Cached response checksums slowed both NICs and added 512 bytes;
512 polling checks failed repeats. Both rejected. All tools/evidence stay in
osenv; firmware dominates reset latency. No matched Cloudflare or physical-board
win is established. New harness tools and breakpoint-failure regression tested.

Headless machine follow-up: explicit external minimal-devices mode removes unused
default devices/VGA. Same complete BIOS/disk/DHCP path and exact guest image:
three native runs, 15,000 verified responses; reset-to-HTTP 58.7–60.9 ms,
full controller launch 524–545 ms, 6369 requests/s, median 147–149 µs,
p99 210–242 µs. Real loss/window/wrap/checksum and five-frame wire gates passed.
Machine configuration is recorded and preserved by reproduce/recover. External
18 controller gates, 22 unit tests and source audits passed. Physical timing
and a matched Cloudflare comparison remain unverified.

T019 — Goal: challenge boot/serving across loader, clock, CPU and packet paths.
Interface: optional authored PVH32 entry, guarded root-bus BAR setup, absolute
HPET time cached at PIT IRQ, contiguous TCP frames; external restore/clock tools.
Prerequisites: T018 release, published Xen PVH/Intel PCI/APIC/HPET/QEMU interfaces.
Acceptance: full BIOS boot remains mandatory; real alternative boot/DHCP/HTTP,
malformed loader/maps and clock arithmetic, lost-tick deliberate defect, packet
loss/window/wrap/checksum gates, matched repeated load and source audits.
Provenance: specifications only and reuse of our own stage2; no imported code.
Measured: final matched 3×10,000-response BIOS/PVH runs gave median reset-release
to verified HTTP 59.23/21.52 ms and 6573/6465 requests/s on one-CPU TCG/NAT.
Preload/PVH repeat gave 20.05/20.26 ms and 6397/6406 requests/s: no convincing
additional improvement; <5 ms remains unmet. Default LTO/-O3 packed disk is
16,384 bytes; actual kernel/payload 20,310/14,815 bytes (unpacked layout 22,016).
Low-memory PVH GDT survives kernel BSS clearing; PIT IRQ caches validated
absolute HPET time, preserving elapsed deadlines across deferred delivery.
Local full BIOS 16-case integrated gate, packed decoder, both NIC production wire
gates, nine actual boundary cases per PVH variant and source audits passed.
Physical/homelab and Cloudflare comparison remain unverified. Raw runs, hashes, rejected candidates and verdicts stay in osenv.
