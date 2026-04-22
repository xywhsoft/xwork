# Run API

> Status: English draft, pending review.

Run API models a single agent execution lifecycle: options, state, events, steps, approval pause/resume, summary, sync execution, async execution, cancellation, and recovery.

## Related Declarations

- `xwork_run`
- `xwork_run_options`
- `xwork_run_create()`
- `xwork_run_start()`
- `xwork_run_execute()`
- `xwork_run_execute_async()`
- `xwork_run_get_summary()`

## Minimal Call

```c
const char *psWorkspaces[] = { "main" };
xwork_run_options tRun;
xwork_run *pRun = NULL;

xwork_run_options_init(&tRun);
tRun.sRunId = "run-1";
tRun.sInstruction = "Inspect the workspace.";
tRun.psWorkspaceIds = psWorkspaces;
tRun.iWorkspaceCount = 1;

xwork_run_create(pRuntime, &tRun, &pRun);
xwork_run_start(pRun);
xwork_run_complete(pRun);
```

## Ownership

Runs are owned by the runtime unless explicitly destroyed earlier. Run strings and workspace IDs are copied. Callback pointers and user data are borrowed.

## Recovery Boundary

Serializable run state, events, checkpoints, artifacts, pending tools, and approval decisions can be persisted. Live process, terminal, thread, and callback state cannot be recovered.

## Related Docs

- [Orchestrator API](api-orchestrator.en.md)
- [Persistence API](api-persistence.en.md)
