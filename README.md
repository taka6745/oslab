# oslab

A minimal, extremely fast OS built from scratch for a homelab VM, developed autonomously through text-based tools.

- Author all guest boot code, kernel, drivers and runtime for this project. No external guest drivers or services. External compilers and host-side development tools/libraries are permitted.
- Keep release code small; compile optional diagnostics and tests into separate builds.
- The agent must build, boot, observe, debug, test and recover the VM without screenshots or a functioning guest network.
- Measure boot time, image size, memory and workload performance before making speed claims.

[AGENTS.md](AGENTS.md) defines the working contract. Current status: specification only; no OS or harness implemented.
