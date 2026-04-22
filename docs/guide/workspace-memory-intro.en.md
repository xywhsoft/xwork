# Workspace Memory

> Status: English draft, pending review.

Workspace memory connects `xwork_workspace` to `xllm_memory`, allowing host products to sync workspace context into model memory.

## Ownership

`xwork_workspace_options::pMemory` is borrowed. The caller owns the `xllm_memory` object and must keep it alive longer than the workspace.

## Flow

```text
create xllm_memory
create xwork workspace with borrowed memory
sync workspace root or selected files
model turn reads memory context
```

## Recovery

Run snapshots do not recreate memory objects. The host must recreate or load compatible memory before recovering runs that reference it.

## Next

- [Workspace API](../api/api-workspace.en.md)
- [xllm Integration API](../api/api-xllm-integration.en.md)
