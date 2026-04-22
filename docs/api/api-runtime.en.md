# Runtime API

> Status: English draft, pending review.

Runtime API owns the top-level `xwork_runtime`, runtime options, profiles, tool registry, workspaces, runs, host services, persistence backend, replay engine, and `xllm` integration boundary.

## Related Declarations

- `xwork_runtime`
- `xwork_runtime_options`
- `xwork_runtime_create()`
- `xwork_runtime_destroy()`
- `xwork_runtime_register_tool()`
- `xwork_runtime_add_workspace()`
- `xwork_profile_apply_runtime_options()`

## Minimal Call

```c
xwork_runtime_options tOptions;
xwork_runtime *pRuntime = NULL;

xwork_runtime_options_init(&tOptions);
xwork_runtime_create(&tOptions, &pRuntime);
xwork_runtime_destroy(pRuntime);
```

## Ownership

`xwork_runtime_create()` returns an owned runtime. Destroying the runtime also destroys attached workspaces, registered tools, and runs that are still owned by the runtime.

`pLlmRuntime`, `pReplayEngine`, callback user data, host services, and persistence backend user data are borrowed. `pLlmBootstrap` lets xwork create and own the `xllm_runtime`.

## Errors

Typical errors are `XWORK_ERROR_INVALID_ARGUMENT`, `XWORK_ERROR_NO_MEMORY`, `XWORK_ERROR_ALREADY_EXISTS`, and `XWORK_ERROR_EXTERNAL_FAILURE`.

## Related Docs

- [xllm Integration API](api-xllm-integration.en.md)
- [Workspace API](api-workspace.en.md)
- [Tool API](api-tools.en.md)
