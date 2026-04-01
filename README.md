# xwork

`xwork` is the shared orchestration/runtime layer above `xllm`.

It exists to serve both:

- `xcode`
- `xclaw`

## Positioning

Layering:

- `xrt`
- `xllm`
- `xwork`
- `xcode` / `xclaw`

`xllm` solves model/runtime concerns:

- provider abstraction
- request/response normalization
- streaming
- session
- memory

`xwork` solves workflow/runtime concerns:

- workspace model
- tool registry
- approval and autonomy policy
- run/step state machine
- checkpoint and resume
- shared host services
- orchestration around `xllm`

## Current Status

This repository is past the initial bootstrap stage and now has a working
minimum shared runtime baseline.

What exists now:

- design baseline: [DESIGN.md](/D:/git/xwork/DESIGN.md)
- development plan: [DEVELOPMENT_SPEC.md](/D:/git/xwork/DEVELOPMENT_SPEC.md)
- public API draft: [xwork.h](/D:/git/xwork/xwork.h)
- aggregate implementation entry: [xwork.c](/D:/git/xwork/xwork.c)
- runtime / workspace / tool registry / run lifecycle
- event / approval request / checkpoint / artifact object model
- `xllm`-backed orchestrator loop with tool execution and approval pause/resume
- minimal local host helper for filesystem/process/vcs host services
- built-in host tool defs for `filesystem.read_text` / `filesystem.write_text` / `process.exec` / `vcs.status`
- builtin host tool execution now auto-synthesizes output/command artifacts for read/write/process/vcs flows
- local `filesystem.write_text` host contract now supports `mode=append`
- local `filesystem.write_text` host contract now supports `mode=create`
- local `filesystem.write_text` host contract now supports request-level `create_dirs:true`
- local `filesystem.read_text` host contract now supports request-level `offset_bytes`
- local `filesystem.read_text` host contract now returns explicit `file_size_bytes` / `remaining_bytes` / `eof` / `next_offset_bytes`
- local filesystem `read_text` / `write_text` failure paths now preserve structured result payloads for not-found / already-exists / parent-missing cases
- local `process.exec` host contract now supports request-level `cwd` override
- local `process.exec` host contract now supports request-level `max_output_bytes` truncation
- local `process.exec` host contract now supports request-level `env:["KEY=VALUE"]`
- local `process.exec` host contract now supports request-level `stdin_text`
- local `process.exec` host contract now supports request-level `allow_nonzero_exit:true`
- local `process.exec` failure paths now preserve structured result payloads for invalid request / non-zero exit cases
- local `process.exec` stdin_text is bounded by configured `iMaxProcessInputBytes`
- local `process.exec` env list is bounded by configured `iMaxProcessEnvEntries`
- typed artifact emit helpers for patch / report / command / output
- workspace memory attach and tool/artifact memory ingest hooks
- in-memory and file-backed checkpoint / snapshot persistence
- persisted run / event / checkpoint / artifact query surface
- built-in `xcode` / `xclaw` profiles
- smoke tests under [tests](/D:/git/xwork/tests)

The implementation is still intentionally minimal. It is a reusable baseline,
not yet a production runtime.

## Repository Layout

```text
xwork/
  DESIGN.md
  DEVELOPMENT_SPEC.md
  README.md
  xwork.h
  xwork.c
  include/
  src/
    xwork_core/
    xwork_workspace/
    xwork_tools/
    xwork_orchestrator/
    xwork_policy/
    xwork_persistence/
    xwork_artifacts/
    xwork_host/
    xwork_profiles/
  examples/
  tests/
  docs/
```

## Public API Draft

The current API draft now covers the minimum reusable workflow runtime surface:

- `xwork_runtime`
- `xwork_workspace`
- `xwork_run`
- `xwork_tool_def`
- `xwork_event`
- `xwork_approval_request`
- `xwork_checkpoint`
- `xwork_artifact`

The draft currently exposes:

- runtime creation/destruction
- workspace registration and lookup
- tool registration and lookup
- run creation and lifecycle state transitions
- orchestrator execution on top of `xllm`
- approval pause/resume and checkpoint load/recover
- artifact emission
- typed patch/report/command/output artifact helpers
- in-memory and file-backed persistence/query helpers
- built-in profile application helpers
- built-in host tool lookup/registration helpers
- local host service bootstrap helpers

This is enough to anchor a minimum end-to-end runtime loop before more
production-grade host integration is added.

## Minimal Example

```c
#include "xwork.h"

int main(void)
{
    xwork_runtime *pRuntime = NULL;
    xwork_runtime_options tRuntime;
    xwork_workspace_options tWorkspace;
    xwork_run_options tRun;
    xwork_workspace *pWorkspace = NULL;
    xwork_run *pRun = NULL;

    xwork_runtime_options_init(&tRuntime);
    xwork_workspace_options_init(&tWorkspace);
    xwork_run_options_init(&tRun);

    tWorkspace.sWorkspaceId = "main";
    tWorkspace.sRootPath = "D:/git/project";

    tRun.sRunId = "run-1";
    tRun.sInstruction = "Inspect the repo and propose a fix.";

    if ( xwork_runtime_create(&tRuntime, &pRuntime) != XWORK_OK ) {
        return 1;
    }
    if ( xwork_runtime_add_workspace(pRuntime, &tWorkspace, &pWorkspace) != XWORK_OK ) {
        xwork_runtime_destroy(pRuntime);
        return 2;
    }
    if ( xwork_run_create(pRuntime, &tRun, &pRun) != XWORK_OK ) {
        xwork_runtime_destroy(pRuntime);
        return 3;
    }
    if ( xwork_run_start(pRun) != XWORK_OK ) {
        xwork_runtime_destroy(pRuntime);
        return 4;
    }
    xwork_run_complete(pRun);
    xwork_runtime_destroy(pRuntime);
    return 0;
}
```

## Near-Term Milestones

1. Expand the built-in host tool surface and harden request/response contracts.
2. Tighten persistence format/versioning and durable query coverage.
3. Expand real-provider `xllm` smoke coverage beyond the offline stub baseline.
4. Add richer artifact semantics around patch/report/command outputs.
5. Add streaming/interruption-aware orchestration paths on top of the current loop.

## Smoke Tests

Primary smoke coverage currently lives in:

- [tests/xwork_orchestrator_smoke.c](/D:/git/xwork/tests/xwork_orchestrator_smoke.c)
- [tests/xwork_orchestrator_provider_smoke.c](/D:/git/xwork/tests/xwork_orchestrator_provider_smoke.c)

Typical compile/run commands:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -c xwork.c
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_orchestrator_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_orchestrator_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_orchestrator_provider_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_orchestrator_provider_smoke.exe -lws2_32 -liphlpapi
tests\xwork_orchestrator_smoke.exe
tests\xwork_orchestrator_provider_smoke.exe
```

The provider smoke now runs offline by default with a local OpenAI-compatible
HTTP stub, and can optionally hit a real provider when the relevant
environment variables are set.

## Non-Goals For This Stage

- distributed scheduling
- remote control plane
- rich editor integration
- production-grade persistence
- full autonomous planner

## Next Files To Read

- [DESIGN.md](/D:/git/xwork/DESIGN.md)
- [DEVELOPMENT_SPEC.md](/D:/git/xwork/DEVELOPMENT_SPEC.md)
- [xwork.h](/D:/git/xwork/xwork.h)
