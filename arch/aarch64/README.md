# Pi 4 bring-up status

Project-authored AArch64 entry, secondary-core parking, EL2-to-EL1 handoff,
PL011 UART, generic timer and cache-geometry reads. Shared HTTP selection and
formatting code runs here; the network stack separately cross-compiles for ARM64.
No GENET driver, Ethernet service, MMU/cache enablement or physical-board
verification is implemented yet. This target explicitly reports that gap and
must not be advertised as a Raspberry Pi web-server image.

Build outside the guest checkout:

```
make pi4-bringup OUT=../osenv/build/pi4-bringup
```

The output `kernel8.img` expects Pi 4 firmware to load it at `0x80000` in
AArch64 EL1/EL2 with MMU off. For a board bring-up boot configuration, use
`arm_64bit=1`, `kernel=kernel8.img`, `kernel_address=0x80000`, `enable_uart=1`,
`init_uart_clock=48000000`, `dtoverlay=disable-bt` and `arm_peri_high=0`.
Firmware remains platform infrastructure; none is imported into this repository.
UART is GPIO14/15, 115200 8N1. Sending `?` reads the actual bring-up status.
Other Pi models are not supported by these fixed BCM2711 peripheral addresses.

The external controller repeats this gate with
`python3 -m osenv pi4-test --project ../oslab` and saves source/image/tool hashes,
symbols and serial output. QEMU 11.1.2 `raspi4b` boot was checked externally: EL1 marker, advancing physical
counter, cache registers (48 KiB L1I / 32 KiB L1D / 1 MiB L2) and UART query.
These are emulated-register results, not physical speed or cache-residency proof.

Specifications consulted, with no source imported:
[BCM2711 peripherals](https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf),
[Pi boot settings](https://www.raspberrypi.com/documentation/computers/config_txt.html),
[Cortex-A72 manual](https://documentation-service.arm.com/static/60368ce38f952d2e4134dc2e).
