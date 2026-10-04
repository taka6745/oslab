# Tooling readiness

Baseline verified on Apple Silicon macOS: Python 3.12.11, LLVM/LLD 23.1.2,
NASM 3.02, QEMU 11.1.2 and GDB 17.2. Use Homebrew's explicit tool paths;
Apple's default linker does not produce our guest ELF files. No shell profile
changes are needed. This is an observed baseline, not yet an enforced tool lock.

Install: `brew install llvm lld nasm qemu gdb`

Check: `python3 tools/check_toolchain.py`

The check returns JSON and exits nonzero on failure. It checks host ASan/UBSan,
x86-64 freestanding C compilation, ELF linking and binary conversion, NASM BIOS
sector assembly, QEMU TCG startup/cleanup, QMP handshake/status and GDB/MI remote
register access. Each subprocess and socket read is bounded; temporary outputs
are removed. GDB remote debugging works without native macOS process signing.

Local development will use BIOS, one x86 CPU and TCG without guest networking.
The controller enforces the build tool versions, pins `pc-i440fx-9.2`, retains
per-run manifests and isolates sockets, overlays and artifacts. Record/replay is
not enabled or validated yet.

Authenticated homelab access and capacity were checked read-only. Private
discovery output is under ignored `local/`. Dedicated controller-owned QEMU TCG
VMs verified the exact 16-bit boot/fault/hang/reset/recovery fixture and 32/64-bit
debug fixtures. Existing Proxmox VM definitions were not changed. This verifies
the harness fixtures, not a released OS.

## Task T001: development tools

- Goal: establish working tools before implementing the external harness.
- Interface: `python3 tools/check_toolchain.py`; JSON result and exit code.
- Prerequisites: Homebrew, Python 3 and the five formulae listed above.
- Acceptance: compile/link/assemble, host sanitizers, QEMU TCG, QMP and GDB/MI
  checks pass; authenticated remote access and capacity discovered privately.
- Result: all local checks passed; remote discovery succeeded. No OS boot or
  integrated fault/hang/recovery acceptance test exists yet.

## Task T002: external harness

- Goal: one automated build/boot/inspect/fault/hang/recovery command and reusable
  AI-friendly debugging controls.
- Interface: `./dev`; pinned public `osenv` submodule, JSON results, run IDs and
  log cursors. Guest fixture protocol `OSE1`; QMP and persistent GDB/MI.
- Prerequisites: T001 tooling, Python 3.12+, authenticated homelab host tools.
- Acceptance: external boot assertion and expected exit; intentional #UD,
  hang/reset capture, recovery, malformed inputs, deliberate defect rejection,
  16/32/64-bit debugger tests, isolated runs, partial-capture survival and
  dead-controller recovery; exact fixture images verified remotely.
- Result: 17 gate cases passed locally and in GitHub CI, including 5,000 seeded
  malformed protocol inputs; exact fixture hashes passed seven homelab checks.
  Generated evidence and private deployment configuration remain ignored.

Next: project-authored complete disk boot chain and kernel, replacing these
fixtures; add subsystem introspection to a separate debug build as services
appear. The harness's machine inspection works independently of guest services.
