# Serving measurements

Local QEMU TCG results, 2026-10-05. One qemu64 CPU, 64 MiB RAM,
pc-i440fx-9.2, isolated NAT, fixed RTC and real-time execution (icount off).
Toolchain: LLVM/LLD 23.1.2, NASM 3.02, QEMU 11.1.2. T013 production image:
`51dac8649eee65fcdc0c913cd62fac3db2f033b373b8b8307da998fb30ce23fd`.
External osenv retains hashes, complete socket samples, captures and verdicts.

| Measurement | Previous image, 2,000 requests | Final image, 10,000 requests |
| --- | ---: | ---: |
| Launch through first HTTP response | 1.535 s | 1.534 s |
| Requests/s | 223 | 681 |
| Median connection through full response | 4.773 ms | 1.655 ms |
| p99 connection through full response | 5.449 ms | 2.991 ms |
| Median first byte after request | 3.274 ms | 1.516 ms |
| HTML body goodput | 2.44 Mbit/s | 7.44 Mbit/s |
| Median captured frames per connection | 11 | 8 |
| Median captured Ethernet bytes per connection | 2,123 | 1,959 |
| HTTP headers / unchanged body | 78 / 1,366 bytes | 94 / 1,366 bytes |
| Boot image | 12,288 bytes | 12,800 bytes |

Final results combine two sequential seeded runs of 5,000 requests (seeds 24301
and 24302), which individually measured 659 and 704 requests/s. Throughput is
total requests divided by total load time; latency aggregates actual samples.
The single-connection small-page workload measures emulator/NAT behavior, not
physical NIC capacity, Raspberry Pi performance or a universal speed limit.
Captured bytes exclude FCS, preamble and inter-frame gaps. Boot includes host VM
startup, BIOS, link negotiation, DHCP and the first response.

The driver now wakes the CPU on real receive IRQs. ITR=64 bounds the interrupt
rate at about 61,035/s; PIT deadlines remain active. Idle checks DMA readiness
with interrupts masked before STI/HLT. The response piggybacks its request ACK;
final data carries FIN only when the receiver has sequence-space room. A closed
window postpones FIN, and loss of its ACK retransmits both payload and FIN.

Optional header whitespace and the optional 200 reason text were removed, while
the required status-line separator remains. Content-Length derives from the
actual body at compile time, allowing clients to detect truncation. Charset,
content type, connection closure, checksums, parser bounds, deadlines and error
responses remain. The complete response is exactly one 1,460-byte TCP payload.
Protocol references: [HTTP framing](https://www.rfc-editor.org/rfc/rfc9112.html),
[TCP ACK and FIN](https://www.rfc-editor.org/rfc/rfc9293.html),
[Intel controller registers](https://www.intel.com/content/dam/doc/manual/pci-pci-x-family-gbe-controllers-software-dev-manual.pdf).
No reference implementation was imported.

A packet scatter/copy-removal experiment increased the image and consistently
worsened latency, so it was reverted. Disabling the emulator NIC option ROM did
not improve boot and remains opt-in. Compression saves 1,536 disk bytes against
the same 14,336-byte raw image; expanded code is identical. Neither experiment
established faster boot. Performance and safety take priority over size.

Separate debug profiling measured 2,005,450,000 of 2,008,225,000 elapsed TSC ticks
across HLT during idle, and 2,352,549,000 of 2,874,628,000 during load. These
intervals include interrupt-handler time and are not retired instructions or
physical idle cycles. Production contains no profiling instrumentation.

Validation includes real production/debug packet peers, lost ACK/FIN ACK,
zero-window and exact-window reopening, sequence wrap, malformed requests,
timeout recovery, silent non-HTTP ports and production UART. A standard HTTP
client accepted the full response and rejected a proxy-truncated actual body.
The new window test rejected the preceding faulty image. Two GDB-triggered
hardware interrupt deliveries verified handler entry, cause clearing and EOI.
Integrated boot/fault/recovery, host sanitizers and packed-image guards passed.

Reproduce from the sibling osenv checkout after `make prod` and `make debug`:

```sh
python3 -m osenv.web_stress --image build/oslab-prod/oslab.img --symbols build/oslab-prod/kernel.elf --production --requests 5000 --seed 24301
python3 -m osenv.http_interop --image build/oslab-prod/oslab.img --symbols build/oslab-prod/kernel.elf
python3 -m osenv.irq_test --image build/oslab-debug/oslab.img --symbols build/oslab-debug/kernel.elf
```

Physical cache misses, link throughput, board boot and exact-image homelab
validation remain unverified: configured SSH timed out. Emulated Pi bring-up
passed, but native Pi Ethernet and serving are still unimplemented.

## Hardware/compiler search

T015 compares actual disk images twice in opposite orders, 2,000 seeded requests
per run, retaining boundaries, malformed traffic and timeout recovery. First set:

| Candidate | Requests/s, two runs | Image bytes | Decision |
| --- | --- | ---: | --- |
| Current baseline | 618 / 656 | 12,800 | Keep |
| ITR=0 | 591 / 658 | 12,800 | No repeatable gain; removes IRQ rate cap |
| ITR=256 | 506 / 524 | 12,800 | Slower throughput |
| -O2 | 617 / 574 | 15,872 | No throughput gain |
| -O3 | 635 / 596 | 16,896 | No throughput gain |
| REP MOVSB copies | 660 / 620 | 12,800 | No consistent gain |

References: [Intel packet tuning](https://www.intel.com/content/dam/doc/application-note/8255x-8254x-ethernet-controllers-small-packet-traffic-performance-appl-note.pdf),
[Intel CPU optimization](https://www.intel.com/content/dam/doc/manual/64-ia-32-architectures-optimization-manual.pdf).
Our candidates are authored here; external experiments and evidence stay in osenv.

Cache prefetch/non-temporal stores need physical PMU/cache validation; ordinary
[TCG does not model guest caches](https://www.qemu.org/docs/master/devel/multi-thread-tcg.html).
MMIO remains uncacheable and DMA ordering/fences stay intact. Huge-page mappings,
DMA alignment and batched RX doorbells already exist. CPU-specific SIMD needs
feature detection, state initialization and interrupt preservation before use.
Checksum offload needs authored descriptor handling and independent packet checks;
software checksums remain enabled. No unsupported offload flag is treated as work.

Per-core serving requires AP startup, APIC routing, independent stacks/connection
state and bounded queue handoff; share immutable page/code, isolate mutable data
by cache line. The current driver has one receive ring and no RSS distribution.
Merely setting QEMU to multiple CPUs cannot implement these requirements.
Wi-Fi/bonding needs authored device drivers, per-interface addressing, link-failure
handling and network support. [Ordinary bonding](https://docs.kernel.org/networking/bonding.html)
generally distributes flows rather than doubling one connection's bandwidth.
Neither SMP serving nor bonding is currently implemented.

Second matched set (two runs each): baseline 629/614 requests/s, ring32 606/639,
checksum unroll 642/641, bounded polling 1,257/1,182. Keep the existing ring and
checksum loop; retain only the 32-iteration PAUSE/descriptor check before sleep
in compact web builds. Interrupts are not masked for this polling window; the
original masked DMA check plus atomic STI/HLT remains after it.
Combined throughput: baseline 621 vs polling 1,218 requests/s; median latency
1.678 ms vs 0.341/0.353 ms; polling p99 2.812/2.837 ms. Responses and captured
wire cost are unchanged. Debug idle halt fraction: baseline 99.84%, polling
98.87%, including ISR time, not physical power/cycles. This is a bounded
latency/idle tradeoff measured under TCG, not proof of hardware superiority.
Release image remains 12,800 bytes, SHA256
`c7dd4825473bdbcd471283db1af722b25780342efc87a67e58d2f9e5e7e1e80c`.


## Boot and minimum exchange (T016)

The final image supports the 82574 legacy interface with the same authored
driver. Four matched 2,000-request runs, alternating device order:

| NIC model | Full launch to first response | Requests/s | Default |
| --- | --- | --- | --- |
| 82540EM (`e1000`) | 1,527 / 1,523 ms | 1,210 / 1,192 | Yes |
| 82574 (`e1000e`) | 520 / 530 ms | 914 / 900 | No |

Use `--nic-model e1000e` with osenv web stress or web peer tests. Guest image
remains 12,800 bytes, SHA256
`fa8913dfef42b4f3e97dae14f10dcbcfc9600e9f331a9741999e31cc17ff6791`.
This is an emulator hardware choice, not physical board speed. Original NIC
throughput is retained by default. No emulator timers or guest checks were bypassed.

Three QMP RESUME/STOP probes on the preceding image measured 68.61, 68.63 and
69.59 ms from reset resume through BIOS/disk/kernel/NIC initialization to the
first DHCP-send breakpoint. This excludes host setup, debugger attachment and
DHCP; it must not replace the full launch-to-serving measurement. Captures show
DISCOVER/OFFER followed by a one-second delay before REQUEST/ACK on e1000.
[QEMU's e1000 model](https://raw.githubusercontent.com/qemu/qemu/master/hw/net/e1000.c)
gates RX for one second after RCTL initialization. The 82574 path avoids that
emulator behavior, using the compatible interface documented in the
[Intel 82574 datasheet](https://device.report/m/d8f635e77f61284e5f89e0d91a2ddc48564a3154f7b1de885fec0d589f1a6695).

Independent captures prove a graceful five-frame exchange: SYN, SYN/ACK,
ACK+GET+FIN, response+ACK+FIN, ACK. Production/debug packet peers passed.
Normal socket/NAT clients still send eight frames; the server cannot force their
ACK/request/FIN coalescing. Loss, window boundaries and required acknowledgements
remain intact. Both NICs passed wire acceptance; 82574 IRQ cause/EOI rearmed
twice. Integrated boot/fault/recovery, sanitizers and exact-image codec tests passed.

## Instruction-driven tuning (T017)

Pinned one-CPU TCG/NAT production experiments verified 100,000 responses,
plus a final 10,000-request run. Two seeded runs per candidate; instrumentation
was separate. Polling budgets 0/8/16/32/64/128/256/512 delivered combined
563/551/584/1256/1542/1634/4743/4878 requests/s. Always polling 256/512 consumed
5.70/10.39% of the debug idle TSC interval outside halt. Gate polling on an
actual RX batch within two timer milliseconds: 256 checks delivered 4668
requests/s with only 0.21% outside halt. Retain 256; 512 adds little throughput.
The original masked descriptor check and atomic STI/HLT remain.

With that activity gate, matched 2×5000-request copy experiments measured byte
loop / REP MOVSB / REP MOVSQ at 4742/5101/6118 requests/s. Retain word copies
plus byte tails; boot establishes the x86 direction-flag ABI. Host sanitizer
and guard-page tests execute the actual x86 implementation (Rosetta on this
Mac), cover zero length, alignments, tails and page edges, and reject a deliberate
rounded-up word-count defect. No alternative implementation substitutes for it.
Checksum iterations of 2/8/16 bytes measured 6248/6407/6298 requests/s; retain
8-byte unrolling with the existing tail and carry fold. An independent byte
oracle covers alignments and 5000 seeded lengths through 65535 bytes and rejects
a dropped-word defect. Existing packet/HTTP sanitizer fuzzing remains enabled.

Final exact production image: 12,800 bytes, SHA256
`728d4e2074bce8d62794c92d7bf60de74f85a3326ec3681878a56724b4bc3237`.
10,000 requests: 6474 requests/s, median 143 µs, p99 231 µs, cold launch to
first response 1511 ms. The page, headers and wire exchange are unchanged.
These changes improve serving; they do not remove the emulator NIC boot delay.

An authored external QEMU plugin counts actual instruction dispatches by ELF
symbol and cross-checks inline totals against a separate callback count. It adds
no guest code or production diagnostics. Earlier 32-check/byte-copy debug runs
attributed 71% of serving dispatches to memcpy, motivating the second copy test.
Final debug dispatches/request fell 53402→12692; idle dispatches in two seconds
fell 662060→189143, with 0.18% of the interval outside halt.
Counts include debug commands and faulting dispatches; LTO-inlined work is
attributed to the containing symbol. This is not physical retired instructions,
cycle accuracy, cache misses, power or physical network throughput. Graphs,
per-request data, candidate sources, hashes and captures remain outside oslab.

References: [Intel optimization manual](https://cdrdv2-public.intel.com/821612/248966-Optimization-Reference-Manual-V1-050.pdf)
for PAUSE/polling tradeoffs; [QEMU plugin API](https://www.qemu.org/docs/master/devel/tcg-plugins.html)
for dispatch counters; [instruction-counting limits](https://qemu-project.gitlab.io/qemu/devel/tcg-icount.html).
PAUSE latency varies by hardware, so physical retuning remains necessary.


## Controlled boot investigation (T018)

The new external perf_bench CLI prepares forwarding while paused, then measures
reset-resume-call to verified HTTP separately from full controller launch. With
82574 and its unused network boot ROM disabled, the unchanged T017 image took
72–86 ms, median 73.4 ms; full launch was about 550 ms. Legacy NIC reset-to-HTTP
still took 1079–1091 ms. No timer, DHCP or disk-integrity checks were bypassed.
QMP/GDB probes: reset to kernel_main 61.6 ms; reset to first dhcp_send 64.4 ms.
These are separate boots, not synchronized component intervals. Firmware/disk
accounts for most startup time. HTTP timing includes control RPC and 10 ms
DHCP capture observation resolution; milestone timing uses actual QMP events.

Two matched final serving runs on 82574 gave ~6423 requests/s, median 146 µs,
p99 225/232 µs. An immutable-response checksum cache slowed both NICs and grew
the image by 512 bytes; 512 burst checks failed repeat comparisons. Both were
removed. The release bytes/hash stay unchanged. The external CLI alternates
baseline/candidate runs with matching seeds and rejects failed aggregates.

[Cloudflare's isolate startup](https://blog.cloudflare.com/eliminating-cold-starts-2-shard-and-conquer/)
occurs inside an already-running service and may be hidden behind TLS. This
full BIOS/DHCP startup is a broader boundary; no matched Cloudflare win is claimed.
Physical board measurements remain unverified.

Headless machine follow-up: explicit external minimal-devices mode removes unused
default devices/VGA. Same complete BIOS/disk/DHCP path and exact guest image:
three native runs, 15,000 verified responses; reset-to-HTTP 58.7–60.9 ms,
full controller launch 524–545 ms, 6369 requests/s, median 147–149 µs,
p99 210–242 µs. Real loss/window/wrap/checksum and five-frame wire gates passed.
Machine configuration is recorded and preserved by reproduce/recover. External
18 controller gates, 22 unit tests and source audits passed. Physical timing
and a matched Cloudflare comparison remain unverified.


## Optional PVH boot and elapsed time (T019)

Current default production uses LTO/-O3: packed disk 16,384 bytes, kernel
20,310 bytes, packed payload 14,815 bytes; the sector-rounded unpacked layout
would be 22,016 bytes. Default image SHA256:
`51035f4194671ad445e798c8f3668dad6d7dccd73493a3ee0cbaaf4e374da5e8`.
Earlier sizes/results above describe their respective historical images.

The optional authored [PVH32 adapter](../boot/pvh.md) uses external qboot
firmware and real loader memory-map metadata. Its descriptor table is copied to
reserved low memory before kernel BSS clearing. Guarded PCI BAR allocation and
PIIX3 level-triggered IRQ routing support the declared minimal i440fx board.
PIT IRQs now cache elapsed time from a validated 64-bit HPET counter; delayed
interrupt delivery no longer subtracts elapsed time from deadlines. Absence or
unsupported HPET falls back to PIT ticks. No deadline or integrity check was
removed.

Final matched one-CPU TCG/NAT runs use headless pc-i440fx-9.2 and e1000e,
three seeds and 10,000 verified responses per run. Both routes in each pair use
the same PVH-enabled disk image SHA256
`2657a899166cdf9771e4c8771bce78cfe7d09b1b43a10f8a5c528e630e17a32f`:

| Pair | BIOS disk | qboot/PVH | qboot/PVH preload |
| --- | ---: | ---: | ---: |
| BIOS vs PVH: median reset-release call to full HTTP | 59.23 ms | 21.52 ms | — |
| BIOS vs PVH: requests/s | 6,573 | 6,465 | — |
| PVH vs preload: median reset-release call to full HTTP | — | 20.26 ms | 20.05 ms |
| PVH vs preload: requests/s | — | 6,406 | 6,397 |

PVH removes about 37.7 ms from the first pair's median, with throughput within
1.7%. Whole controller launch to response still spans 466–518 ms for PVH and
501–556 ms for BIOS. Serving socket medians span 140–146 µs in that pair;
captured request-frame to response-frame medians are 20–21 µs. These are
different timing boundaries. Reset-release includes control RPC and DHCP
observation polling, followed by externally verified real HTTP. Every run starts
a fresh reset guest; it does not restore an HTTP-ready OS snapshot.

Preloading the recorded raw kernel at 1 MiB skips adapter payload copying while
still hashing actual kernel memory. The second pair gives no convincing extra
boot or serving improvement: ranges overlap and throughput differs by 0.2%.
It remains an optional experiment. Neither route achieved less than 5 ms.

External osenv retains `local/t019/final-pvh-vs-bios/summary.json`,
`local/t019/final-preload-vs-pvh/summary.json`, individual captures, source/build
provenance and verdicts. Full BIOS, packed decoder, both NIC production wire
gates and nine actual boundary cases per PVH variant passed; source audits passed.
These local results establish neither physical boot/cycle/cache performance nor
a matched Cloudflare startup comparison; exact-image homelab validation remains
unverified.

Contiguous TCP frame construction removes two payload copies. A separate three-run
O3/Oz comparison measured captured service medians 19/23 µs and aggregate
6525/6354 requests/s, retaining O3 despite its larger image. Host sanitizer
tests include 100,000 seeded HPET arithmetic cases against an independent
128-bit oracle. Forced lost cached ticks recovered with zero observed lag;
disabling HPET sampling deliberately lagged 535 ms and was rejected.

Exact default production image follow-up (three 10,000-response runs):
6671 requests/s, reset-release to HTTP median 59.77 ms, warm medians
136–140 µs, p99 216–239 µs and captured service median 19 µs.
This is a separate validation, not a matched PVH comparison.
