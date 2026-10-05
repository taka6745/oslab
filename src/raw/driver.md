# Raw NIC and clock runtime

T021: project-authored x86-64 instruction bytes implement root-bus PCI discovery,
eight legacy RX/TX descriptors and actual HPET time. Requires one CPU, DF clear,
identity-mapped guest storage, uncached PCI/HPET MMIO and BIOS-assigned BARs.
Code addresses and exact image/source hashes come from the external build manifest.
SysV functions preserve callee-saved registers; no compiled or assembled guest
implementation is copied into this variant.

`clock_init` validates HPET revision, a 64-bit counter and exactly 10000000 fs
(10 ns) period at the supported PC address 0xfed00000. Other periods fail
explicitly. It resets/enables the counter without legacy replacement.
`clock_ms` reads that real counter and computes floor(counter/100000) using
counter>>5 and a proven unsigned reciprocal division by 3125; call only after
successful initialization. Network polling caches an actual sample in `net_now`.

`nic_init` scans 32 root-bus function-zero slots for Intel 82540EM (100e:8086) or
82574 (10d3:8086), validates a memory BAR in 0xc0000000..0xffffffff, enables bus
mastering, resets with a 100 ms deadline, reads the actual MAC, configures DMA
rings and waits up to 3 seconds for link. Returns 1/0; unsupported buses/functions
and absent devices fail. Failed initialization leaves the shared MMIO pointer null.

`nic_send(rdi=frame,rsi=bytes)` accepts 14..1514 bytes only with a completed,
error-free descriptor. It copies bounded whole qwords plus remaining bytes,
zero-pads to 60 bytes, publishes EOP/IFCS/RS after a store fence and rings TDT.
Return 1 means queued; external delivery requires wire verification. Eight
independent buffers prevent reuse before DD.

`nic_receive()` returns rax=buffer and edx=length, or both zero. Each call examines
at most eight descriptors. DMA errors, missing EOP and invalid lengths are
released; accepted frames remain caller-owned until `nic_release`. Additional
receive calls return zero while a frame is outstanding; repeated release is
harmless. `nic_pending` reads real DD. `nic_interrupt` acknowledges ICR;
`nic_irq_enable` enables RX/link/error causes after IRQ setup.

Shared storage: MAC 0x180100, MMIO 0x180108, RX head 0x180110, TX tail 0x180114,
outstanding 0x180118, IRQ 0x18011c, HPET period 0x180120 and last RX milliseconds
0x180128. RX/TX rings occupy 0x181000/0x181080 (128 bytes each); eight 2048-byte
RX buffers occupy 0x182000..0x185fff and TX buffers 0x186000..0x189fff.

Locally tested on the reciprocal image: direct NIC ownership/error/boundary,
60-byte padding, guarded qword copy and exact eight-descriptor work limits passed;
clock boundary/seeded vectors and deliberate defects were checked externally.
Evidence is in osenv/local/t021/release-gate/driver.json and
clock-primitives.json. Both NIC wire gates and exact-image homelab acceptance
subsequently passed; see [release measurements](README.md).

Provenance: this project's kernel/e1000.c and kernel/arch.c document its hardware
interface; instruction bytes are newly authored from the
[Intel SDM](https://cdrdv2-public.intel.com/774492/325383-sdm-vol-2abcd.pdf) and
[Intel 82574 register/descriptor documentation](https://www.intel.com/content/www/us/en/collections/products/ethernet/gigabit-controllers/82574-controllers.html).
