# Agent contract

## Work autonomously

Read README.md. Make routine implementation decisions, record them briefly, and continue without asking the user to choose tools or approve each step. Start with x86-64, BIOS, C plus assembly, LLVM/LLD, NASM, QEMU with the separately maintained osenv controller unless evidence requires a change. These are initial defaults, not portability requirements.

Build the external harness first, then the complete disk boot chain, exceptions/memory, interrupts/drivers, storage and user execution. Add SMP after the single-CPU path works. Do not invent application requirements. For each task record its goal, interface, prerequisites and acceptance tests in one short task entry; update it with results. Split independent work behind stable interfaces and isolated images/runs.

Use local QEMU software emulation for development and a dedicated homelab VM for final validation. Discover authenticated access and capacity; keep host configuration private. Automate setup and diagnose recoverable failures. Ask only for unavailable credentials, unresolved product scope, or destructive changes outside this project's resources. If remote access is blocked, continue local work and report the exact blocker. Preserve unrelated VMs and dirty work.

## Repository boundary

Keep only project-authored guest code, OS-specific tests, build definitions and
these development contracts here. No host VM controller, harness fixtures,
submodules, packaging dependencies, deployment scripts or private host settings.
The external osenv checkout owns VM processes, sockets, logs, captures, recovery
and deployment. Keep generated output outside this checkout where practical;
never track it. Do not restore the removed wrapper or harness as a convenience.

CI permits root development contracts, Makefile/linker definitions, the source
integrity workflow and C/assembly/linker sources or documentation under boot/,
kernel/, arch/, include/, src/ and tests/. Extend that boundary only for a concrete
OS requirement and document its purpose. OS tests must exercise actual OS code;
external harness fixtures belong in osenv.

## OS diagnostic interface

Expose early boot markers, serial diagnostics and a versioned command/result
protocol in optional debug builds. Keep all guest implementations project-authored;
no external guest agent. Provide matching ELF symbols and load addresses for every
boot-stage CPU mode. Describe the actual panic format so the external harness can
decode it. The OS must be observable without a working guest network.

Use the separate osenv CLI to build/observe/debug/test and preserve evidence before
recovery. Save source/build/image hashes, symbols, tool versions, firmware/machine
configuration, inputs/seeds and external verdicts outside this repository.
Failures must be reproducible from saved inputs. Missing harness functionality
must be implemented in osenv, never embedded here or faked by guest success text.

## Tests and completion

Pin tooling and machine configuration. Deterministic tests use one CPU, clean disks, fixed inputs/seeds/time and no uncontrolled network. Validate record/replay support before enabling it. Keep accelerated/SMP stress separate; assert invariants across repeated schedules.

Test portable logic on the host with sanitizers and seeded fuzzing; test hardware behavior in the booted guest. Cover malformed/truncated boot images, expected faults, memory exhaustion/overflow/boundaries, absent devices, interrupt/DMA errors, corrupt storage and hangs. Save minimised failing inputs as regressions. Keep a full custom disk-boot test even if faster direct-loading tests exist.

Check outputs, completion markers, expected exit state and unexpected resets/panics externally. A successful compile or guest PASS line alone is not completion. Prove representative tests catch deliberate defects. Gate changes on relevant tests and integrated boot. Verify the exact release image on the homelab before claiming deployment complete.

Use real OS images for OS acceptance; fixtures remain in osenv and may never
substitute for OS implementation. Current status is recorded in TASKS.md.

Never commit secrets, private host configuration, generated images or memory dumps. Keep documentation short and current; distinguish implemented, locally tested and homelab verified. Keep all shipped guest code project-authored.

## Mandatory source integrity

Read and obey [INTEGRITY.md](INTEGRITY.md) before editing. These requirements
are release-blocking. No exceptions may be inferred from a green test suite.
