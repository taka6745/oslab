# Development plan

## Execution lanes

1. Host-native tests for portable allocator, parser, queue and filesystem logic, with sanitizers and seeded fuzzing.
2. Local QEMU TCG for complete boot, single-CPU repeatable tests and instruction-level debugging.
3. A dedicated remote development environment for the same pinned TCG tests and larger campaigns.
4. A dedicated deployment VM for accelerated execution, SMP stress and release measurements.

An ARM64 development host can cross-compile and emulate x86. Hardware acceleration is architecture-dependent. The same image must be validated in each intended execution lane; emulator and accelerated execution are not interchangeable correctness proofs.

## Tools and interfaces

Candidate tools: LLVM/LLD, NASM for an x86 BIOS path, one build graph (Make or CMake/Ninja), QEMU/qemu-img, QMP, GDB/MI, Python/pytest and hosted fuzzing tools. QEMU EDU is a candidate virtual device for a project-written PCI/MMIO/IRQ/DMA driver.

Build a CLI first, with a thin MCP wrapper over identical operations: doctor, build, run, logs, test, debugger inspection, capture, replay, stop and deploy. Return run/operation IDs, status, exit codes, log cursors and artifact references. Long operations are asynchronous and bounded. One controller owns each VM.

The guest test protocol uses a serial transport and supports boot-time tests before an interactive command interface exists. Preserve raw output alongside parsed events. External inspection must work without guest networking or a guest agent.

## Evidence and determinism

Preserve stage-specific ELF symbols, linker maps, image hashes, source revision, build flags, machine and firmware manifests, serial output, QMP events and test inputs. On hangs, pause and inspect registers, memory and disassembly before reset. A new OS requires its own versioned panic/dump interpretation; existing Linux dump tools do not automatically understand it.

Pin machine version, CPU features, firmware and toolchain. Start deterministic tests with one CPU, fixed RAM, fresh disk overlays, fixed RTC/input/seed and no live network. Enable instruction counting and record/replay only for validated supported configurations. KVM/SMP stress uses repeated schedules and invariants. Verify reproducible builds rather than assuming cross-host byte identity.

Tests must distinguish expected guest failures from infrastructure failures. Require protocol completion, intended assertions, appropriate process exit and absence of unexpected resets. Verify representative tests detect deliberately introduced defects. Save minimised fuzz failures as regression fixtures.

## Milestones and acceptance

| Milestone | Required evidence |
|---|---|
| External harness | Broken image and hung guest produce usable diagnostics and bounded recovery without guest networking |
| Boot chain | Custom boot code reaches loader and kernel; symbols resolve at each stage |
| Exceptions and memory | Fault reports identify expected instructions; allocation/mapping invariants hold under exhaustion and boundary cases |
| Interrupts and drivers | Project-written serial/timer/PCI drivers pass IRQ and DMA tests plus absent-device and timeout cases |
| Storage and user execution | Fixture reads/writes, malformed images and protection boundaries behave as specified |
| SMP and acceleration | Repeated progress/isolation tests with captured failure state |
| Deployment | Exact tested artifact boots on the dedicated target VM; text observation and external recovery verified |

Preserve full disk-boot tests even if direct kernel loading is later used for faster subsystem tests. Do not replace project boot-code authorship with an external bootloader.

## First task

Confirm CPU architecture, BIOS versus UEFI, language and initial device set. Establish pinned tooling and a minimal harness fixture. Prove early output capture, debugger inspection, timeout capture and clean reruns before starting open-ended OS features.

## Primary references

- [Clang cross-compilation](https://clang.llvm.org/docs/CrossCompilation.html)
- [QEMU GDB interface](https://www.qemu.org/docs/master/system/gdb.html)
- [QMP reference](https://www.qemu.org/docs/master/interop/qemu-qmp-ref.html)
- [QEMU record/replay](https://www.qemu.org/docs/master/system/replay.html)
- [GDB machine interface](https://www.sourceware.org/gdb/current/onlinedocs/gdb.html/GDB_002fMI.html)
- [QEMU EDU device](https://www.qemu.org/docs/master/specs/edu.html)
- [QTest device-model tests](https://www.qemu.org/docs/master/devel/testing/qtest.html)
- [LLVM libFuzzer](https://llvm.org/docs/LibFuzzer.html)

Master documentation may describe unreleased capabilities. Check the pinned installed release before enabling features.
