# First xwork Program

> Status: English draft, pending review.

This example maps to [`examples/first_xwork_program.c`](../../examples/first_xwork_program.c). It validates the minimal runtime/workspace/run lifecycle.

## What It Demonstrates

- Create `xwork_runtime`.
- Add one workspace.
- Create one run.
- Start and complete the run.
- Destroy the runtime.

## Build

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\first_xwork_program.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\first_xwork_program.exe -lws2_32 -liphlpapi
```

Exit code `0` means the lifecycle completed.

## Key APIs

- `xwork_runtime_create()`
- `xwork_runtime_add_workspace()`
- `xwork_run_create()`
- `xwork_run_start()`
- `xwork_run_complete()`
- `xwork_runtime_destroy()`
