# Migration

> Status: English draft, pending review.

This page describes how to migrate product-specific agent infrastructure toward xwork.

## From a Direct xllm Tool Loop

1. Keep `xllm` provider/session configuration as the model layer.
2. Create an `xwork_runtime`.
3. Add workspaces and profiles.
4. Register tool definitions in the xwork registry.
5. Move tool execution through xwork tool executors or host services.
6. Use `xwork_run_execute()` or `xwork_run_execute_async()` for the model/tool loop.
7. Record artifacts, checkpoints, and persistence through xwork APIs.

## From a Custom Tool System

Map existing tools to:

- `xwork_tool_def`
- `xwork_tool_exec_fn` or `xwork_tool_exec_ex_fn`
- `xwork_tool_call`
- `xwork_tool_result`
- policy/approval metadata
- artifact output

## From Product-Specific Persistence

Use the built-in file persistence for local smoke and development. For production DB/object storage, implement `xwork_persistence_backend`.

## Migration Rule

Move one capability at a time: runtime/workspace first, then tool registry, then policy/artifacts, then persistence/replay, then multi-agent or remote-worker orchestration.
