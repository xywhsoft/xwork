# Workspace API

> Status: English draft, pending review.

Workspace API models project roots, workspace IDs, memory integration, and the boundary where filesystem policies are enforced.

## Related Declarations

- `xwork_workspace`
- `xwork_workspace_options`
- `xwork_runtime_add_workspace()`
- `xwork_runtime_find_workspace()`
- `xwork_workspace_memory_sync_*`

## Minimal Call

```c
xwork_workspace_options tOptions;
xwork_workspace *pWorkspace = NULL;

xwork_workspace_options_init(&tOptions);
tOptions.sWorkspaceId = "main";
tOptions.sRootPath = ".";

xwork_runtime_add_workspace(pRuntime, &tOptions, &pWorkspace);
```

## Ownership

Workspaces are owned by the runtime. Workspace ID and root path are copied. `pMemory` is borrowed and must outlive the workspace.

## Boundary

Workspace roots define the allowed filesystem scope. Host products should still enforce policy and approval before writes, deletes, process execution, and terminal operations.

## Related Docs

- [Runtime API](api-runtime.en.md)
- [Local Host API](api-local-host.en.md)
- [Workspace memory guide](../guide/workspace-memory-intro.en.md)
