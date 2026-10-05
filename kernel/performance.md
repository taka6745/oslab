# Serving measurements

Local QEMU TCG results, 2026-10-05. One qemu64 CPU, 64 MiB RAM,
pc-i440fx-9.2, isolated NAT, fixed RTC and real-time execution (icount off).
Toolchain: LLVM/LLD 23.1.2, NASM 3.02, QEMU 11.1.2. Final production image:
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
