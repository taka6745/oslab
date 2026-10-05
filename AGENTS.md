# Agent contract

## Work autonomously

Read README.md. Make routine implementation decisions, record them briefly, and continue without asking the user to choose tools or approve each step. Start with x86-64, BIOS, C plus assembly, LLVM/LLD, NASM, QEMU with the separately maintained osenv controller unless evidence requires a change. These are initial defaults, not portability requirements.

Build the external harness first, then the complete disk boot chain, exceptions/memory, interrupts/drivers, storage and user execution. Add SMP after the single-CPU path works. Do not invent application requirements. For each task record its goal, interface, prerequisites and acceptance tests in one short task entry; update it with results. Split independent work behind stable interfaces and isolated images/runs.

Use local QEMU software emulation for development and a dedicated homelab VM for final validation. Discover authenticated access and capacity; keep host configuration private. Automate setup and diagnose recoverable failures. Ask only for unavailable credentials, unresolved product scope, or destructive changes outside this project's resources. If remote access is blocked, continue local work and report the exact blocker. Preserve unrelated VMs and dirty work.

## Required external tool interface

Maintain the CLI in the external osenv repository, never vendor it here. Require
one CLI for setup/doctor, build, run, logs, test, debug, capture, stop/recover and deploy. Add MCP only when a caller requires it; reuse the CLI implementation. All operations must be noninteractive, bounded and return useful exit codes plus structured results. Long operations return IDs; logs support cursors. Isolate sockets, overlays and artifacts per run; one controller owns each VM.

Capture output before boot. Use early boot markers, serial diagnostics and a small versioned command/result protocol for guest tests; no external guest agent. Drive VM state through QMP and CPU/memory/breakpoints through GDB's machine interface. Support every boot-stage CPU mode and matching ELF symbols/load addresses. On panic, reset or timeout preserve raw logs, registers, memory and disassembly where available before recovery. Preserve evidence even when capture fails; never wait indefinitely.

Save source/build/image hashes, symbols, tool versions, firmware/machine configuration, test input/seed and verdict per run. Dumps and diagnostics must be readable through the CLI; implement the project's panic format decoder as needed. Failures must be reproducible from saved inputs with one command.

## Tests and completion

Pin tooling and machine configuration. Deterministic tests use one CPU, clean disks, fixed inputs/seeds/time and no uncontrolled network. Validate record/replay support before enabling it. Keep accelerated/SMP stress separate; assert invariants across repeated schedules.

Test portable logic on the host with sanitizers and seeded fuzzing; test hardware behavior in the booted guest. Cover malformed/truncated boot images, expected faults, memory exhaustion/overflow/boundaries, absent devices, interrupt/DMA errors, corrupt storage and hangs. Save minimised failing inputs as regressions. Keep a full custom disk-boot test even if faster direct-loading tests exist.

Check outputs, completion markers, expected exit state and unexpected resets/panics externally. A successful compile or guest PASS line alone is not completion. Prove representative tests catch deliberate defects. Gate changes on relevant tests and integrated boot. Verify the exact release image on the homelab before claiming deployment complete.

The external harness fixture gate is established. The next OS goal is the complete
project-authored disk boot chain. Use real OS images for OS acceptance; fixtures
remain in osenv and may never substitute for OS implementation.

Never commit secrets, private host configuration, generated images or memory dumps. Keep documentation short and current; distinguish implemented, locally tested and homelab verified. Keep all shipped guest code project-authored.

## Mandatory source integrity

Read and obey [INTEGRITY.md](INTEGRITY.md) before editing. These requirements
are release-blocking. No exceptions may be inferred from a green test suite.
