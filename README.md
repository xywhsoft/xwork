# xwork

`xwork` is the shared orchestration/runtime layer above `xllm`.

It exists to serve both:

- `xcode`
- `xclaw`

## Positioning

Layering:

- `xrt`
- `xllm`
- `xwork`
- `xcode` / `xclaw`

`xllm` solves model/runtime concerns:

- provider abstraction
- request/response normalization
- streaming
- session
- memory

`xwork` solves workflow/runtime concerns:

- workspace model
- tool registry
- approval and autonomy policy
- run/step state machine
- checkpoint and resume
- shared host services
- orchestration around `xllm`

## Current Status

This repository is past the initial bootstrap stage and now has a working
minimum shared runtime baseline.

What exists now:

- design baseline: [DESIGN.md](/D:/git/xwork/DESIGN.md)
- development plan: [DEVELOPMENT_SPEC.md](/D:/git/xwork/DEVELOPMENT_SPEC.md)
- agent runtime progress tracking: [AGENT_RUNTIME_TRACKING_SPEC.md](/D:/git/xwork/AGENT_RUNTIME_TRACKING_SPEC.md)
- public API draft: [xwork.h](/D:/git/xwork/xwork.h)
- aggregate implementation entry: [xwork.c](/D:/git/xwork/xwork.c)
- runtime / workspace / tool registry / run lifecycle
- event / approval request / checkpoint / artifact object model
- `xllm`-backed orchestrator loop with tool execution and approval pause/resume
- model turn execution can now pass stream preference, forward normalized model events, honor xllm cancel tokens, and cooperatively interrupt before/after major run phases
- host service invocation now has a per-call context for run/cancel/interrupt metadata, and local `process.exec` can cooperatively cancel before spawn or while polling a running subprocess
- custom tool executors can now opt into `pfnToolExecEx` with a cancellation context and `xwork_tool_exec_context_should_cancel()` helper
- runs can now execute through a minimal async handle (`xwork_run_execute_async` / `xwork_run_async_*`) with wait, timed wait, status, cancel, and destroy APIs; async cancel feeds the same cooperative cancel token path and is covered against mock tools, unfinished-handle destroy, and local `process.exec`
- async run API docs now state the shallow-copy/lifetime contract for run/options/callback user data and the wait-timeout status semantics
- run execution now has a per-run execution guard, so concurrent `xwork_run_execute` entries on the same run fail with `XWORK_ERROR_INVALID_STATE`
- provider smoke now exercises the async run handle on its main provider execution path while keeping local stub coverage for request/response normalization and an offline model-call failure path
- minimal local host helper for filesystem/process/vcs host services
- built-in host tool defs for `filesystem.read_text` / `filesystem.write_text` / `process.exec` / `process.start_terminal` / `process.terminal_read` / `process.terminal_write` / `process.terminal_resize` / `process.terminal_stop` / `vcs.status`
- builtin host tool execution now auto-synthesizes output/command artifacts for read/write/process/vcs flows
- builtin terminal host tool execution now auto-synthesizes session command/output artifacts for start/write/stop flows
- local `filesystem.write_text` host contract now supports `mode=append`
- local `filesystem.write_text` host contract now supports `mode=create`
- local `filesystem.write_text` host contract now supports request-level `create_dirs:true`
- local `filesystem.read_text` host contract now supports request-level `offset_bytes`
- local `filesystem.read_text` host contract now returns explicit `file_size_bytes` / `remaining_bytes` / `eof` / `next_offset_bytes`
- local filesystem `read_text` / `write_text` failure paths now preserve structured result payloads for not-found / already-exists / parent-missing cases
- local `process.exec` host contract now supports request-level `cwd` override
- local `process.exec` host contract now supports request-level `max_output_bytes` truncation
- local `process.exec` host contract now supports request-level `env:["KEY=VALUE"]`
- local `process.exec` host contract now supports request-level `stdin_text`
- local `process.exec` host path now runs on `xrt subprocess` instead of shell `popen`
- local `process.exec` host contract now supports request-level `timeout_ms`
- local `process.exec` host contract now supports request-level `timeout_stop` (`interrupt` / `terminate` / `kill` / `kill_tree`)
- local `process.exec` host contract now supports request-level `allow_nonzero_exit:true`
- local `process.exec` host contract now supports request-level `merge_stderr:false`
- local `process.exec` host contract now supports request-level `include_events:true`
- local `process.exec` host contract now supports request-level `use_terminal:true` with optional `terminal_cols` / `terminal_rows`
- local `process.exec` result now returns explicit `stdout` / `stderr` plus per-stream truncation flags
- local `process.exec` result can now return ordered `xrt subprocess` events with stream/kind/text/exit metadata
- local `process.exec` terminal mode now reports `use_terminal`, negotiated terminal size, explicit `terminal_output_captured`, and ordered lifecycle events; captured terminal text remains platform-dependent
- local host now supports interactive terminal sessions via `process.start_terminal` / `process.terminal_read` / `process.terminal_write` / `process.terminal_resize` / `process.terminal_stop`
- local `process.terminal_resize` now reports explicit `resize_applied` so terminal resize can degrade gracefully instead of failing the whole session
- local terminal session state results now return explicit `output_text` / `output_bytes`, plus `event_end_seq` / `has_more_events` / `event_stream_done` alongside ordered events
- local `process.terminal_write` now supports `include_state:true` with optional `after_seq` / `max_events`, and also `write_eof:true` to close terminal stdin explicitly; a write can return the post-write incremental terminal state in one round trip
- local terminal session state results now also carry stable metadata `session_index` / `stdin_closed`, so callers can track session lifetime without inferring it from write responses
- local host now supports `process.list_terminals` with request-level `session_name` / `running` / `done` / `after_session_index` / `limit` filters, plus `session_index_asc` pagination metadata (`has_more_sessions` / `next_after_session_index`), and terminal sessions can carry a stable `session_name`, so callers can rediscover and page through active interactive sessions without caching everything out-of-band
- builtin `process.list_terminals` now also emits a JSON output artifact (`terminal-sessions://active`) from orchestrator runs, so terminal inventory can be persisted alongside other run artifacts
- builtin `process.terminal_resize` now also emits a JSON output artifact, so negotiated terminal geometry changes can be persisted alongside the rest of a terminal session
- builtin `process.terminal_write` now also emits a JSON output artifact when terminal state is returned, so post-write terminal state windows persist alongside the write command itself
- builtin `process.terminal_read` now also emits a JSON output artifact, so incremental terminal state windows can enter the same run artifact/persistence pipeline as terminal session transcripts
- builtin `process.terminal_stop` now also emits a JSON output artifact, so final terminal session stop state persists alongside the transcript/output artifact
- when interactive terminal support is available, file persistence smoke also verifies that terminal session JSON artifacts (`resize/write/read/stop`) can be listed and loaded back, and can now be found directly by artifact name from both file persistence and runtime persistence APIs
- persisted artifacts now also have a lightweight summary-list surface (`id/kind/output_class/output_role/name/mime/storage_ref/summary/sequence`, plus content and patch stats when present), so callers can inspect terminal JSON artifact chains before loading full artifact bodies
- persisted artifact summaries now also support a minimal metadata query surface (`kind` / `output_class` / `output_role` / `name_prefix` / `mime` / `storage_ref` / `exit_code` / `sequence`), so callers can filter terminal JSON chains without loading full artifacts
- artifact summary query also supports exact `artifact_name`, so callers can resolve a single persisted terminal JSON artifact without falling back to full artifact loads
- persisted artifact summary query now also supports `after_sequence + limit`, and summary lists return `has_more/next_after_sequence`, so terminal artifact chains can be paged without ad hoc callers-side cursors
- artifact summary query also supports `mime_prefix` / `storage_ref_prefix`, so callers can slice a shared terminal session artifact chain by transport metadata without full artifact loads
- output/report artifacts now carry typed output metadata (`output_class` plus `output_role`) through runtime objects, summaries, snapshots, persistence, and workspace memory ingest; builtin filesystem and terminal artifact synthesis fills file-content/file-change/terminal-state/terminal-inventory classes
- report artifacts now also carry report-specific typed metadata (`report_class` plus `report_subject_ref`) through runtime objects, summaries, snapshots, persistence, query filters, and workspace memory ingest
- artifact summary query smoke now also covers `exit_code` and min/max `sequence` filters against a persisted command artifact
- builtin `process.exec` artifact synthesis now preserves stderr instead of dropping it from command artifacts
- command artifacts can now carry structured command I/O stats (`stdout_byte_count` / `stderr_byte_count` / stdout/stderr truncation flags) through runtime objects, summaries, snapshots, persistence, and workspace memory ingest
- local `process.exec` failure paths now preserve structured result payloads for invalid request / timeout / cancelled / non-zero exit cases, including requested stop policy and observed stop reason
- local `process.exec` stdin_text is bounded by configured `iMaxProcessInputBytes`
- local `process.exec` env list is bounded by configured `iMaxProcessEnvEntries`
- typed artifact emit helpers for patch / report / command / output
- artifacts with `content_text` now carry computed content stats (`content_byte_count` / `content_line_count`) through runtime objects, summaries, snapshots, persistence, and workspace memory ingest
- patch artifacts now carry computed patch stats (`file_count` / `hunk_count` / `added_line_count` / `deleted_line_count`) through runtime objects, summaries, snapshots, persistence, and workspace memory ingest
- workspace memory attach and tool/artifact memory ingest hooks
- workspace memory can now be synced directly from a workspace root through `xllm_memory_sync_workspace`, with a small `xwork_workspace_memory_sync_summary` result surface
- workspace memory can also sync one changed file through `xllm_memory_sync_file`, with a compact change-kind summary for incremental update paths
- in-memory and file-backed checkpoint / snapshot persistence
- persisted run / event / checkpoint / artifact query surface
- built-in `xcode` / `xclaw` profiles
- smoke tests under [tests](/D:/git/xwork/tests)

The implementation is still intentionally minimal. It is a reusable baseline,
not yet a production runtime.

## Repository Layout

```text
xwork/
  DESIGN.md
  DEVELOPMENT_SPEC.md
  README.md
  xwork.h
  xwork.c
  include/
  src/
    xwork_core/
    xwork_workspace/
    xwork_tools/
    xwork_orchestrator/
    xwork_policy/
    xwork_persistence/
    xwork_artifacts/
    xwork_host/
    xwork_profiles/
  examples/
  tests/
  docs/
```

## Public API Draft

The current API draft now covers the minimum reusable workflow runtime surface:

- `xwork_runtime`
- `xwork_workspace`
- `xwork_run`
- `xwork_tool_def`
- `xwork_event`
- `xwork_approval_request`
- `xwork_checkpoint`
- `xwork_artifact`

The draft currently exposes:

- runtime creation/destruction
- workspace registration and lookup
- tool registration and lookup
- run creation and lifecycle state transitions
- orchestrator execution on top of `xllm`
- approval pause/resume and checkpoint load/recover
- artifact emission
- typed patch/report/command/output artifact helpers
- in-memory and file-backed persistence/query helpers
- built-in profile application helpers
- built-in host tool lookup/registration helpers
- local host service bootstrap helpers

This is enough to anchor a minimum end-to-end runtime loop before more
production-grade host integration is added.

## Minimal Example

```c
#include "xwork.h"

int main(void)
{
    xwork_runtime *pRuntime = NULL;
    xwork_runtime_options tRuntime;
    xwork_workspace_options tWorkspace;
    xwork_run_options tRun;
    xwork_workspace *pWorkspace = NULL;
    xwork_run *pRun = NULL;

    xwork_runtime_options_init(&tRuntime);
    xwork_workspace_options_init(&tWorkspace);
    xwork_run_options_init(&tRun);

    tWorkspace.sWorkspaceId = "main";
    tWorkspace.sRootPath = "D:/git/project";

    tRun.sRunId = "run-1";
    tRun.sInstruction = "Inspect the repo and propose a fix.";

    if ( xwork_runtime_create(&tRuntime, &pRuntime) != XWORK_OK ) {
        return 1;
    }
    if ( xwork_runtime_add_workspace(pRuntime, &tWorkspace, &pWorkspace) != XWORK_OK ) {
        xwork_runtime_destroy(pRuntime);
        return 2;
    }
    if ( xwork_run_create(pRuntime, &tRun, &pRun) != XWORK_OK ) {
        xwork_runtime_destroy(pRuntime);
        return 3;
    }
    if ( xwork_run_start(pRun) != XWORK_OK ) {
        xwork_runtime_destroy(pRuntime);
        return 4;
    }
    xwork_run_complete(pRun);
    xwork_runtime_destroy(pRuntime);
    return 0;
}
```

## Near-Term Milestones

1. Expand the built-in host tool surface and harden request/response contracts.
2. Tighten persistence format/versioning and durable query coverage.
3. Expand real-provider `xllm` smoke coverage beyond the offline stub baseline.
4. Harden async run execution semantics around handle lifetime, concurrent observation, and cancellation coverage.
5. Expand real-provider `xllm` smoke coverage and provider error-path matrix beyond the offline HTTP failure baseline.

## Smoke Tests

Primary smoke coverage currently lives in:

- [tests/xwork_orchestrator_smoke.c](/D:/git/xwork/tests/xwork_orchestrator_smoke.c)
- [tests/xwork_orchestrator_provider_smoke.c](/D:/git/xwork/tests/xwork_orchestrator_provider_smoke.c)

Typical compile/run commands:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -c xwork.c
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_orchestrator_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_orchestrator_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_orchestrator_provider_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_orchestrator_provider_smoke.exe -lws2_32 -liphlpapi
tests\xwork_orchestrator_smoke.exe
tests\xwork_orchestrator_provider_smoke.exe
```

The provider smoke now runs offline by default with a local OpenAI-compatible
HTTP stub, and can optionally hit a real provider when the relevant
environment variables are set.

## Non-Goals For This Stage

- distributed scheduling
- remote control plane
- rich editor integration
- production-grade persistence
- full autonomous planner

## Next Files To Read

- [DESIGN.md](/D:/git/xwork/DESIGN.md)
- [DEVELOPMENT_SPEC.md](/D:/git/xwork/DEVELOPMENT_SPEC.md)
- [xwork.h](/D:/git/xwork/xwork.h)
