# Agent instructions

## Scope and architecture

Follow the constraints in README.md. Guest code must be authored for this project. External host-side tooling is permitted. Do not introduce external guest drivers or runtime dependencies.

Keep guest code small and reusable. Keep the build/debug/test controller outside the guest. Separate release, debug and test builds; do not burden release execution with unconditional tracing.

## Work and verification

Each task must define prerequisites, owned files, interfaces, acceptance tests, negative tests and required evidence. Use isolated run directories, immutable input images and per-run overlays. Do not share writable VM images across concurrent tasks.

Capture boot output before execution begins. Bound every build, VM and debugger operation. Preserve logs and machine state before resetting failed runs when possible.

Pin toolchain, firmware and machine configuration. Use fixed seeds and fixtures for repeatable tests. Treat TCG, accelerated execution and SMP as separate validation lanes.

Do not claim completion from compilation or a guest PASS line alone. Verify expected output, host exit state, absence of unexpected resets and integrated boot behavior. Clearly distinguish planned, implemented, locally verified and deployment verified.

## Public repository

Never commit credentials, private host addresses, authentication URLs, local machine inventory or unreviewed memory dumps. Keep host-specific configuration and run artifacts ignored. New repository publication does not authorize later deployment or modification of existing homelab services.
