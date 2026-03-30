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

This repository is at the initial bootstrap stage.

What exists now:

- design baseline: [DESIGN.md](/D:/git/xwork/DESIGN.md)
- development plan: [DEVELOPMENT_SPEC.md](/D:/git/xwork/DEVELOPMENT_SPEC.md)
- public API draft: [xwork.h](/D:/git/xwork/xwork.h)
- aggregate implementation entry: [xwork.c](/D:/git/xwork/xwork.c)
- internal module skeleton under [src](/D:/git/xwork/src)

The current implementation is intentionally minimal. It is not yet a production runtime.

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

The current API draft focuses on the minimum shared object model:

- `xwork_runtime`
- `xwork_workspace`
- `xwork_run`
- `xwork_tool_def`

The draft currently exposes:

- runtime creation/destruction
- workspace registration and lookup
- tool registration and lookup
- run creation and lifecycle state transitions

This is enough to anchor the initial object model before the full orchestrator is implemented.

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

1. Stabilize the core object model and event model.
2. Add host service abstractions for filesystem/process/vcs.
3. Add the first orchestrator loop on top of `xllm`.
4. Add checkpoint persistence and run resume.
5. Add product profiles for `xcode` and `xclaw`.

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
