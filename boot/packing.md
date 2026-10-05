# Boot packing

Authored here: host-only `pack.c` encodes; `stage2.asm` decodes in protected
mode. No host library enters the guest. Stage one verifies stage two, which
supplies encoded/decoded lengths and the original FNV-1a hash. FNV detects
accidental corruption, not malicious modification.

- Tag 0..127: copy tag+1 following literal bytes (1..128).
- Tag 128..255: copy (tag&127)+3 bytes (3..130) from the following
  little-endian 16-bit backward distance (1..65535, at most bytes emitted).
  Forward copying supports overlap.

The host accepts 1..524,288 bytes and finds minimum cost over every legal
literal/match length; optimality applies only to this format. The build rejects
packing that does not shrink. The decoder bounds all reads/copies, requires the
exact expanded length and verifies its hash before executing the kernel.
Malformed input halts silently in production or reports failure in debug.

Production defaults to packing; `make prod BOOT_COMPRESS=0` selects raw.
Debug defaults to raw. Both load identical kernel code at 1 MiB, with identical
ELF addresses and runtime RAM use; there is no runtime decompression.

After `make prod`, run from osenv:
`python3 -m osenv.packed_test --build build/oslab-prod`. External evidence
includes expanded-byte comparison, sanitizer roundtrips, minimum-size oracles,
malformed disk boots and guard/CPU/RAM captures. Results: T012/T013 in TASKS.md.
