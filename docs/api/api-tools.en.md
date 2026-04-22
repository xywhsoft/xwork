# Tool API

> Status: English draft, pending review.

Tool API defines model-callable tools, tool registry entries, executors, execution context, host-service bridge, and built-in host tools.

## Related Declarations

- `xwork_tool_def`
- `xwork_tool_call`
- `xwork_tool_result`
- `xwork_tool_exec_fn`
- `xwork_tool_exec_ex_fn`
- `xwork_tool_exec_context`
- `xwork_runtime_register_tool()`
- `xwork_runtime_register_builtin_tool()`

## Minimal Call

```c
xwork_tool_def tDef;

xwork_tool_def_init(&tDef);
tDef.sToolId = "example.echo";
tDef.sDisplayName = "Echo";
tDef.eKind = XWORK_TOOL_VIRTUAL;
tDef.eSideEffect = XWORK_TOOL_SIDE_EFFECT_NONE;

xwork_runtime_register_tool(pRuntime, &tDef);
```

## Ownership

The runtime copies public tool metadata during registration. Executor callbacks and user data are borrowed and must outlive runtime use.

## Cancellation

Use `xwork_tool_exec_context_should_cancel()` inside long-running executors and host-service adapters.

## Related Docs

- [Host Tools API](api-host-tools.en.md)
- [Policy / Approval API](api-policy-approval.en.md)
