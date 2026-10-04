# oslab

A lightweight, fast operating system built from scratch, paired with an external debugging and automated testing harness that agents can operate through text and structured tools.

## Project constraints

- All code shipped in the OS is written for this project: boot code, kernel, drivers, runtime services and applications.
- No external guest drivers, guest agents, SSH server or networking stack is required for development access.
- Compilers, linkers, emulators, debuggers and host-side development libraries may be external. They do not become guest dependencies.
- Release builds remain minimal. Debug and test builds provide selectable diagnostics and tests.
- Performance claims require measurements; targets will be defined before implementation.

## Development architecture

An external controller owns QEMU, serial capture, QMP and debugger sessions. It can inspect and recover the machine even when the guest is hung or unable to print. The OS exposes early boot markers, serial diagnostics, exception reports and a small test protocol.

Local emulation supports development; a dedicated homelab VM provides deployment and accelerated integration testing. Architecture, boot method, language and initial hardware profile are still decisions to confirm.

See [the development plan](docs/development-plan.md) and [agent instructions](AGENTS.md).

## Status

Project brief and plan only. No bootloader, kernel, drivers, harness or passing VM tests exist yet.
