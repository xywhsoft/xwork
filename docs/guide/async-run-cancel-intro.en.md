# Async Execution and Cancellation

> Status: English draft, pending review.

Async execution lets the host run an agent in the background, wait with a timeout, cancel cooperatively, and then inspect the final summary.

## Flow

```text
xwork_run_execute_async
  wait
  cancel if needed
  join / get summary
  destroy async handle
```

## Minimal Shape

```c
xwork_run_async_handle *pHandle = NULL;

xwork_run_execute_async(pRun, &tOptions, &pHandle);
xwork_run_async_wait(pHandle, 1000);
xwork_run_async_cancel(pHandle);
xwork_run_async_destroy(pHandle);
```

Cancellation is cooperative. Tool executors and host services must check the cancel context.

## Next

- [Run API](../api/api-run.en.md)
- [Orchestrator API](../api/api-orchestrator.en.md)
