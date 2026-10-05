# Complete hand-encoded server

Every guest instruction in this variant is a literal authored opcode byte.
The separate osenv writer places bytes, checks fixed-width relocations and emits
symbols; it selects no instructions and invokes no guest compiler, assembler or
linker. Readable C/assembly and the earlier hybrid remain separate references.

The fixed x86-64 BIOS setup and unsupported features are listed in the root README.
Boot integrity, actual hardware discovery, packet checksums/bounds, DMA ownership,
timeouts and explicit failure halts remain mandatory. There is no alternate C
execution path. External QMP/GDB provides inspection without guest networking.

T021 image SHA256:
`0c59ee67c4bcbe26b603322a6df70e9f8b320ce2e245cfe2c1f47f4a3d246967`.
Kernel 9,023 bytes; complete disk 9,728 bytes versus readable 16,384 (40.6% smaller).
The unchanged website body is 1,366 bytes; HTTP headers are 94 bytes.

Matched local one-CPU QEMU TCG/NAT, three runs of 10,000 responses per variant:

| Measurement | Readable | Hand encoded |
| --- | ---: | ---: |
| Median BIOS release RPC to verified HTTP | 61.48 ms | 53.86 ms |
| Aggregate responses/second | 6,169 | 6,641 |
| Median captured request to response | 19 µs | 21 µs |
| Guest dispatches, separate init/1,000-request profile | 11,450,241 | 7,883,857 |

Boot includes control RPC and DHCP observation; controller launch is separately
about 0.5 seconds. Captured service latency is slightly slower despite improved
aggregate throughput. Earlier matched throughput varied, so the 7.6% result is
this sample, not a universal speedup. Dispatch counts reconcile with independent
callbacks; they do not measure physical cycles or cache residency. Isolated
two-second QEMU idle sampling measured 2.48% of one host CPU, not idle cycles.

Five TCP frames pass with client ACK/GET/FIN combined; ordinary socket clients
still use eight. Both NIC wire gates pass loss, zero-window, MSS, sequence wrap,
FIN and malformed-packet checks. Direct guest tests cover 186 primitives,
286 clock/zero-fill checks, 61 network boundaries, DMA/IRQ errors, guarded memory,
deliberate defects, corrupt/truncated boot, absent NIC, faults and hang recovery.
The exact image and symbols passed dedicated homelab QEMU integrated checks and
1,000 response/boundary/timeout checks. This is VM verification, not Pi Ethernet
or physical throughput validation.

Host compression round trips reduced the disk to 6,201 bytes with gzip or 5,816
with LZMA. Neither is shipped: an authored decoder and its boot/safety cost have
not been measured. Generated artifacts, rejected candidates, packet captures,
graphs and full measurements remain in external osenv `local/t021`,
`local/t021-instructions` and `local/t021-homelab`.
