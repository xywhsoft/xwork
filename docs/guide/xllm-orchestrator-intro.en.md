# xllm Orchestration and Tool Loop

> Status: English draft, pending review.

xwork uses `xllm` for model turns and wraps those turns in a recoverable run lifecycle with tool calls, approval, artifacts, events, and cancellation.

## Flow

```text
xwork_run_execute
  model turn through xllm
  parse tool calls
  evaluate policy / approval
  execute tool or pause
  record events and artifacts
  repeat until final summary
```

## Minimal Options

```c
xwork_orchestrator_options tOptions;

xwork_orchestrator_options_init(&tOptions);
tOptions.eToolChoiceMode = XWORK_TOOL_CHOICE_AUTO;
tOptions.iMaxTurns = 8;

xwork_run_execute(pRun, &tOptions);
```

## xllm Runtime Modes

- Borrow an existing `xllm_runtime` through `xwork_runtime_options::pLlmRuntime`.
- Let xwork bootstrap one through `xwork_runtime_options::pLlmBootstrap`.

Do not provide both at the same time.

## Next

- [xllm Integration API](../api/api-xllm-integration.en.md)
- [Orchestrator API](../api/api-orchestrator.en.md)
- [Tool API](../api/api-tools.en.md)
