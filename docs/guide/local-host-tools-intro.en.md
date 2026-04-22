# Local Host Tools

> Status: English draft, pending review.

Local host tools expose filesystem, process, terminal, VCS, and editor operations through xwork host services.

## Flow

```text
register built-in tool
configure local host services
model/tool requests host action
policy/approval gate
host service executes
artifact/event recorded
```

## Minimal Registration

```c
xwork_runtime_register_builtin_tool(pRuntime, XWORK_TOOL_FILESYSTEM_READ_TEXT);
xwork_runtime_register_builtin_tool(pRuntime, XWORK_TOOL_PROCESS_EXEC);
```

## Boundary

The host product must enforce workspace roots, approvals, cancellation, timeout, output size limits, and secret redaction.

## Next

- [Host Tools API](../api/api-host-tools.en.md)
- [Local Host API](../api/api-local-host.en.md)
