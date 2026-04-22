# claw Autonomous Agent Example

> Status: English draft, pending review.

This example maps to [`examples/claw_autonomous_agent.c`](../../examples/claw_autonomous_agent.c). It demonstrates a command-line autonomous agent using the `xclaw` profile.

## What It Demonstrates

- Autonomous `process.exec`.
- Command output artifacts.
- File persistence.
- Run recovery from persisted state.

## Key APIs

- `xwork_profile_get_builtin(XWORK_PROFILE_XCLAW, ...)`
- `xwork_profile_apply_run_options()`
- `xwork_file_persistence_configure_backend()`
- `xwork_runtime_register_builtin_tool()`
- `xwork_run_execute()`
- `xwork_runtime_recover_run_from_persistence()`

## Recovery Boundary

Persisted run state can be recovered. Live process handles and terminal sessions are not recovered.
