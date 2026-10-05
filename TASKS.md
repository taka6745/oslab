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
