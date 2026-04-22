# First xwork Program

> Status: English draft, pending review.

This guide shows the smallest useful xwork lifecycle: create a runtime, add a workspace, create a run, start it, complete it, and destroy the runtime.

## Complete Example

Source: [`examples/first_xwork_program.c`](../../examples/first_xwork_program.c).

Build:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\first_xwork_program.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\first_xwork_program.exe -lws2_32 -liphlpapi
```

## Object Flow

```text
xwork_runtime
  owns xwork_workspace
  owns xwork_run
```

## Minimal Lifecycle

```c
const char *psWorkspaces[] = { "main" };
xwork_runtime_options tRuntime;
xwork_workspace_options tWorkspace;
xwork_run_options tRun;
xwork_runtime *pRuntime = NULL;
xwork_workspace *pWorkspace = NULL;
xwork_run *pRun = NULL;

xwork_runtime_options_init(&tRuntime);
xwork_workspace_options_init(&tWorkspace);
xwork_run_options_init(&tRun);

tWorkspace.sWorkspaceId = "main";
tWorkspace.sRootPath = ".";

tRun.sRunId = "run-1";
tRun.sInstruction = "Inspect the workspace and summarize the task.";
tRun.psWorkspaceIds = psWorkspaces;
tRun.iWorkspaceCount = 1;

xwork_runtime_create(&tRuntime, &pRuntime);
xwork_runtime_add_workspace(pRuntime, &tWorkspace, &pWorkspace);
xwork_run_create(pRuntime, &tRun, &pRun);
xwork_run_start(pRun);
xwork_run_complete(pRun);
xwork_runtime_destroy(pRuntime);
```

## Next

- [Runtime API](../api/api-runtime.en.md)
- [Workspace API](../api/api-workspace.en.md)
- [Run API](../api/api-run.en.md)
