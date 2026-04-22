# AI IDE Agent Example

> Status: English draft, pending review.

This example maps to [`examples/ai_ide_agent.c`](../../examples/ai_ide_agent.c). It demonstrates an AI IDE style flow using the `xcode` profile.

## What It Demonstrates

- Workspace-scoped file read.
- Dry-run patch.
- Approval pause/resume.
- Final report artifacts.

## Key APIs

- `xwork_profile_get_builtin(XWORK_PROFILE_XCODE, ...)`
- `xwork_profile_apply_run_options()`
- `xwork_runtime_register_builtin_tool()`
- `xwork_run_execute()`
- `xwork_runtime_get_last_approval_request()`
- `xwork_run_resume_with_approval()`
- `xwork_runtime_query_artifact_summaries()`

## Extension Points

- Connect approval requests to IDE UI.
- Render patch artifacts in an editor diff view.
- Persist run state for IDE restart recovery.
