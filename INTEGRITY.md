# Source integrity rules

These are release-blocking requirements for every contributor and agent.

- Performance, correctness and safety take priority over image size. Retain
  bounds/checksum/integrity checks, honest HTTP framing, timeouts and recovery.
  Keep size changes only after relevant real-image tests and matched boot,
  latency/throughput comparisons. Record rejected candidates and measurement
  limits; a smaller binary alone is never acceptance.
- Implement every advertised behavior through the real execution path. No stubs,
  placeholder bodies, fabricated logs/registers, canned success, mock production
  adapters, TODO implementations or silently ignored unsupported operations.
  Unsupported work must fail explicitly and remain labelled unimplemented.
- Author all shipped source in this project. Do not copy snippets, vendor code,
  link external libraries or add third-party dependencies. Consult specifications
  and documentation, then implement from the documented interface. Record source
  provenance in the task entry. Do not import guest code from any other repository,
  including repositories we own. All guest source and included files must resolve
  inside this repository; only generated objects from those inputs may be linked.
- External executables (compiler, assembler, linker, debugger, emulator, Git/SSH)
  and the Python standard library are development infrastructure. They must not
  become guest code or dependencies of this repository. Firmware is recorded
  emulator infrastructure, not a project-authored boot chain.
- Guest compilation uses `-nostdinc` and repository-authored type definitions;
  no system, compiler, downloaded or harness headers may enter the guest build.
  The build records and validates compiler/assembler dependencies. Host-only
  sanitizer tests remain separate and contribute no code to the OS image.
  The project-authored `boot/pack.c` is a host-only OS build transform; its
  host C library performs file I/O and is never linked into guest code. The
  guest decoder is authored here in `boot/stage2.asm`. Record both inputs.
- Hardware addresses, format constants, setup-specific configuration and explicit
  test vectors are allowed when their meaning is documented. Do not hardcode
  expected answers, run IDs, timing thresholds or special input branches to make
  a real feature appear to work. Test-only deliberate defects must be isolated,
  labelled and excluded from release guest builds.
- Tests must inspect actual results independently and reject representative
  deliberate defects. Never disable a failing check, weaken an assertion, replace
  the tested path with a mock or change an expected result just to obtain green.
- Keep fixtures outside the OS repository. Fixture success proves the harness,
  never an OS feature. Do not claim an unimplemented kernel service is inspectable.
- Run the source audit and relevant execution tests before publishing. Review
  every changed function for completeness and provenance. The automated audit
  detects certain violations; it cannot prove originality or semantic correctness.
  A clean audit is not permission to bypass any rule above.
