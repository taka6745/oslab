# Complete hand-encoded server

Every guest instruction is an authored opcode byte. External osenv places bytes
and fixed-width fields, writes symbol containers and optionally packs data; no
guest compiler, assembler, linker or imported executable/header bytes. Readable
sources remain separate; fixed hardware and unsupported features are in README.

T024 packed BIOS disk: **8,704 bytes /69,632 bits**, SHA256
`fffe2ffc03072c8e9a09d72a396eebc79af4b077e55971010d97096c920dc21f`.
Decoded kernel8,888 bytes; optional direct ELF10,088 bytes. One bounded deferred
SYN prevents a new peer's handshake being lost while the previous peer closes.
The active connection, packet validation, replay parser and deadlines remain
intact. Local/homelab gates and30,000 homelab requests passed; measured direct
CPU release→full HTTP12.025ms,3,837RPS, median client242.882µs/run p99377.476µs.
These fresh results are not a matched speedup over the historical measurements.
DMA-copy, checksum-cache and empty-poll experiments established no serving gain;
they are excluded. Extended NIC checksum offload and static scatter/gather were
slower in paired trials and also excluded. Separately scoped KVM host/guest PMU
records guide further work; they do not establish physical CPU or cache behavior.
Evidence and rejected candidates remain in osenv/local/t024.

Historical T023 packed BIOS disk: **8,192 bytes /65,536 bits**, SHA256
`2c226df829c01614d63bc9bed5ec1edd1d85a98574ae9f5f8792352d4d3b097d`.
Decoded kernel8,568 bytes;512-byte BIOS,187-byte adapter,7,433-byte optimal stream
and60-byte sector fill. Website1,366 bytes plus94 HTTP header bytes still fits
one1,460-byte TCP payload. [Packed interface](packed.md) defines the codec.
Independent source reconstruction and suffix-cost recomputation prove the
attained minimum **for this kernel/codec/adapter/sector layout**. A globally
shortest equivalent OS is unproved. Storage fell five stream bytes from T022,
with three fewer checksum instructions per32-byte block and the same bounds,
loads, negative-zero/carry semantics, tails and ABI.

Historical T023 [literal direct entry](pvh.md): kernel plus382+600-byte adapters in a
9,768-byte ELF, SHA256
`07e285d7364098aa23571f540f108eeee43fc00187ac0061ca2f0c3c08f54218`.
It bypasses disk firmware on a fresh VM; complete BIOS acceptance stays mandatory.

| Scope | Verified RPS | CPU release → full HTTP | Client median | Median run p99 |
| --- | ---: | ---: | ---: | ---: |
| Packed BIOS, local TCG | 6,776 | 54.023ms | 136.813µs | 213.250µs |
| Direct entry, local TCG | 6,519 | 14.998ms | 137.625µs | 215.500µs |
| Direct entry, homelab KVM | 4,043 | 12.820ms | 233.493µs | 363.783µs |

Local: QEMU11.1.2, oneCPU, e1000e, minimal devices, no NIC ROM, five10,000-response
runs per variant. The direct-entry matched BIOS measured6,679RPS and53.084ms.
Cold entry was about70.6% faster; throughput remains inconclusive (paired95%
interval0.911..1.024), with a retained slow first trial. No serving gain is
claimed and direct entry stays optional. Homelab QEMU10.1.2 KVM used three5,000-
response runs per route; its matched BIOS was3,984RPS and20.316ms. Different
machines/accelerators cannot establish an acceleration speedup. Captured18µs
median service uses QEMU virtual time, separate from checked client/QMP clocks.
Neither physical power-on, CPU/idle cycles nor cache residency was measured.

Prepared powered resume:3.038ms median; paused same-process snapshot restoration:
9.960ms, including RAM verification/control/client costs, five samples each.
Arbitrary external clock/lease/peer recovery after suspend remains unimplemented.
Separate one-vCPU VM capacity probes peaked near6,256 aggregateRPS with twoVMs;
four did not improve it. These are separate processes, not guest SMP/affinity.

Exact local and dedicated homelab image gates passed boot/fault/hang recovery,
guarded primitives/clocks, real DMA/IRQ,61 protocol boundaries, both NIC wire
paths,13 decoder cases and19 direct metadata/CPU/PCI boundaries. A bypassed magic
check and checksum/decoder defects were correctly rejected. Homelab stress passed
1,000 complete responses plus boundaries/deadlines on each boot route.
Five TCP frames require combined client ACK/GET/FIN, with both FINs acknowledged;
ordinary clients use eight. Keep-alive, TLS, guest SMP and Pi networking remain
unimplemented. Production exposes no SSH or guest debug service.

Hot-page layout hurt serving and was rejected; header-only clearing and a BAR
software-TLB-alias candidate established no benefit and were excluded. A local
storage interruption remains failed. Lossless sparse capture storage now verifies
unchanged bytes/hashes before replacement. Generated proofs, seeds, source/tool/
firmware manifests, captures, failures, per-request samples and graphs stay in
external osenv/local/t023. T022 remains historical evidence in local/t022.
