# Raw IRQ and idle runtime

T021: real PIT/NIC interrupts wake an idle guest while deadlines use actual HPET
time. Requires one CPU, long mode, project GDT selector 0x18, loaded IDT at
0x1a0000, initialized NIC/HPET and guest code without red zone or SIMD.
Exact function addresses come from the external build manifest.

`irq_init` validates NIC IRQ 3..15, changes IDT vectors 32..47 only, remaps
cascaded PICs to 32/40 and programs PIT mode 3 with divisor 11932: nominal
99.9985 Hz, approximately 10 ms timer polling granularity. It unmasks PIT,
cascade and NIC, enables NIC RX/link/error causes and returns 1/0 with IF clear.
Other gates retain the terminal fault handler.

`raw_irq` saves all 15 non-stack GPRs, clears DF and aligns the stack for a SysV
call that acknowledges the real NIC ICR. It sends slave/master EOI, restores all
registers and returns with IRETQ. `idle_wait` checks RX ownership with IF clear.
It keeps polling for 2 ms after the last RX using cached actual `net_now`, then
uses adjacent STI/HLT/CLI to close the receive-versus-sleep race. Pending RX
returns immediately. Every return has IF clear; timer wakeups permit deadlines
to progress without packets. No synthesized clock or interrupt count is used.

Locally tested reciprocal image: all 256 gates, actual PIT vector 32, repeated
NIC cause/acknowledgement, 15 GPR/RSP preservation and guest clock after deferred
interrupts passed. osenv/local/t021/release-gate/irq.json records 2.48% of one
host CPU over a 2.016-second idle interval. Earlier 1000 Hz images measured
3.97..4.47%, and pure polling measured 100.58%; these observations are not a
matched physical performance claim. Both NIC wire gates and exact-image homelab
acceptance subsequently passed; see [release measurements](README.md).

Bytes were authored directly from the [Intel SDM](https://cdrdv2-public.intel.com/774492/325383-sdm-vol-2abcd.pdf),
[Intel PIC command descriptions](https://www.intel.com/content/dam/www/public/us/en/documents/datasheets/core-m-processor-platform-i-o-datasheet.pdf)
and [Intel 8254 timer descriptions](https://edc.intel.com/content/www/de/de/design/platforms/core-processor-series-3-datasheet-volume-1-of-2/001/8254-timers/).
