# oslab

A minimal, extremely fast OS built from scratch for a homelab VM, developed autonomously through text-based tools.

- Author all guest boot code, kernel, drivers and runtime for this project. No external guest drivers or services. External compilers and host-side development tools/libraries are permitted.
- Keep release code small; compile optional diagnostics and tests into separate builds.
- The agent must build, boot, observe, debug, test and recover the VM without screenshots or a functioning guest network.
- Measure boot time, image size, memory and workload performance before making speed claims.

[AGENTS.md](AGENTS.md) defines the working contract. The external harness is
implemented and tested; guest code remains explicitly labelled fixtures. The
OS boot chain, kernel and drivers are next.

[TOOLING.md](TOOLING.md) records verified development tools and the readiness check.

The reusable harness is [osenv](https://github.com/taka6745/osenv), pinned as a
submodule. Run `git submodule update --init --recursive`, then `./dev doctor`
and `./dev test`. The latter builds, boots, checks assertions, diagnoses a real
fault, captures hangs/resets and recovers. Use `./dev --help` for all controls;
see [the debugger guide](tools/osenv/DEBUGGING.md).

Debugging includes register/memory reads and writes, 16/32/64-bit disassembly,
symbols, additional ELF load addresses, stepping, persistent breakpoints and
watchpoints, device inspection, traces, writable diagnostic logs, isolated NIC
controls/pcap and retained disk overlays. Kernel subsystem inspection will be
added with the corresponding OS services; networking is off in the normal gate.
