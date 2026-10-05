# oslab

A project-authored x86-64 operating system for a dedicated homelab VM.
Initial target: BIOS, C and assembly, LLVM/LLD and NASM.

This repository contains only OS source, OS build definitions and OS development
contracts. The boot chain, kernel and drivers are not implemented yet. No harness
fixture is shipped here and no OS boot or performance claim is currently made.

The external development harness lives in [osenv](https://github.com/taka6745/osenv).
Keep its checkout, sockets, VM disks, captures and private configuration outside
this repository. Run its fixture gate from that checkout. Once an OS image exists,
use its manual-image interface with this project's image and ELF; fixture PASS
must never stand in for OS acceptance. OS build definitions will belong here. CI enforces the OS-only file boundary
defined in AGENTS.md and rejects unrelated tracked files.

[AGENTS.md](AGENTS.md) defines development and real boot acceptance requirements.
[INTEGRITY.md](INTEGRITY.md) forbids stubs, imported code/dependencies and canned
implementations. Audit this repository using the external harness:

```sh
python3 -m osenv audit --os-only --repo /absolute/path/to/oslab
```

Run that command from the external osenv checkout. An audit checks source policy;
it does not assert that an OS exists or boots.
