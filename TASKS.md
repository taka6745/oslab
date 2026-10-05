# Tasks

T001 — Goal: keep this repository exclusively for OS development. Interface:
external osenv CLI; mandatory AGENTS.md and INTEGRITY.md. Prerequisites: separate
osenv checkout and source integrity audit. Acceptance: no harness submodule,
wrapper, fixtures or generated evidence tracked or left in this working tree;
external audit with --os-only passes and is enforced by CI using a pinned
external harness revision. Result: harness removed; historical evidence
preserved privately outside this checkout. OS boot chain not implemented yet.

T002 — Goal: implement a complete project-authored disk boot chain. Interface:
BIOS disk image and matching ELF symbols for external osenv inspection.
Prerequisites: x86-64 toolchain, source integrity rules and external harness.
Acceptance: real full disk boot with externally verified markers, load addresses,
failure diagnostics and exact-image homelab validation. Result: not started.

T003 — Goal: enforce the OS-only repository boundary. Interface: CI tracked-file
allowlist and OS-focused agent contract. Prerequisites: harness already external.
Acceptance: inventory contains only OS contracts/CI; policy rejects controller,
fixture, library, runtime-evidence and private-config paths while accepting OS
source/build paths; external source audit passes. Result: local boundary checks
and source audit pass; no guest code exists yet. CI result is recorded on the commit.
