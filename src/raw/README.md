# Complete hand-encoded server

Every guest instruction is a literal authored opcode byte. External osenv places
bytes and fixed-width fields, writes symbols and optionally packs data; no guest
compiler, assembler, linker or imported executable code is used. Readable sources
remain separate. The fixed setup and unsupported features are in the root README.

T022 disk: **8,192 bytes / 65,536 bits**, SHA256
`afef5c6cb0699f307bad28bc618c10432aca194afc2d3daa2b151c94b868437e`.
Decoded kernel: 8,568 bytes. Storage: 512-byte BIOS sector, 187-byte hand-encoded
adapter, 7,438-byte compressed stream and 55 bytes of sector fill. The unchanged
website is 1,366 bytes plus 94 HTTP header bytes.

The external encoder checks every legal literal/back-reference length with suffix
dynamic programming. Independent accounting reconstructs every disk byte and
recomputes its certificate. **65,536 bits is the attained minimum for this exact
kernel, codec, adapter and BIOS sector layout.** A global shortest equivalent OS
is unproved. Other kernels, encodings, codecs and loaders can change that bound.
[Packed interface](packed.md) records the actual decoder contract.

Matched one-CPU QEMU 11.1.2 TCG, e1000e, minimal devices, BIOS disk boot,
five runs of 10,000 verified responses per variant:

| Measurement | T021 raw baseline | T022 packed |
| --- | ---: | ---: |
| Aggregate requests/second | 6,475 | 6,681 |
| Median CPU release to complete first HTTP reply | 53.910 ms | 53.543 ms |
| Median client request completion | 138.418 µs | 137.292 µs |
| Median per-run client p99 | — | 211.209 µs |
| Median captured request to response | 21 µs | 19 µs |

Separate five-run debugger probes: median CPU release to decoded 64-bit kernel
entry 48.605 ms; network loop 50.238 ms. Median controller launch to first reply
was 523.398 ms. CPU release begins from QEMU's cold reset state; physical power-on
was not measured. Client/QMP timing uses checked host clocks; packet intervals use
QEMU virtual time and are never subtracted from host timestamps. These samples
establish neither physical cycles/cache residency nor Raspberry Pi performance.

A smaller instruction candidate lost throughput and was rejected. The accepted
variant retains original HTTP and TCP hot instructions, shorter proven branches,
cold-init addressing reductions and a smaller fully initialized RAM arena. Bounds,
checksums, DMA ownership, clock validation, deadlines and terminal faults remain.

Local and dedicated homelab exact-image gates passed actual disk boot, guarded
primitives/clock, DMA/IRQ, 61 protocol boundaries and both NIC wire paths. Thirteen
actual decoder cases check full kernel expansion, overlap, truncated/invalid
streams, hashes, output guards and halts. The homelab also passed 1,000 verified
response/boundary/timeout checks. Five frames require client ACK/request/FIN
combined; ordinary clients still use eight. No guest SSH or debug service is added.

Generated proofs, per-request samples, captures, rejected candidates and graphs
remain outside this checkout in osenv `local/t022`; exact homelab evidence is in
`local/t021-homelab/74037ed4-188c-49c0-ad1c-e1cb693ad259`. T021 measurements remain
historical external evidence. This is x86 VM validation; Pi networking remains
unimplemented.
