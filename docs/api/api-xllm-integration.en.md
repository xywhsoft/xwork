# xllm Integration API

> Status: English draft, pending review.

xwork integrates with `xllm_runtime`, profiles, sessions, workspace memory, streaming events, and cancel tokens. xwork does not implement provider adapters; it orchestrates `xllm` model capabilities inside recoverable and approvable runs.

## Related Declarations

- `xwork_xllm_profile_options`
- `xwork_xllm_transport_options`
- `xwork_xllm_bootstrap_options`
- `xwork_runtime_options::pLlmRuntime`
- `xwork_runtime_options::pLlmBootstrap`
- `xwork_workspace_options::pMemory`
- `xwork_model_event`
- `xwork_model_event_fn`

## Minimal Call

```c
xwork_runtime_options tRuntimeOptions;
xwork_xllm_bootstrap_options tBootstrap;

xwork_runtime_options_init(&tRuntimeOptions);
xwork_xllm_bootstrap_options_init(&tBootstrap);

tRuntimeOptions.pLlmBootstrap = &tBootstrap;
xwork_runtime_create(&tRuntimeOptions, &pRuntime);
```

## Modes

| Mode | Meaning |
| --- | --- |
| Borrow existing runtime | The caller creates `xllm_runtime` and passes it through `pLlmRuntime`. |
| Bootstrap runtime | The caller passes `pLlmBootstrap`; xwork creates and owns the `xllm_runtime`. |

`pLlmRuntime` and `pLlmBootstrap` cannot be used together.

## Related Docs

- [Runtime API](api-runtime.en.md)
- [Workspace API](api-workspace.en.md)
- [Orchestrator API](api-orchestrator.en.md)
