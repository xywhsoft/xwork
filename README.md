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
- model event cancellation priority is documented: interrupt/cancel token checks run before the user event callback, and a callback returning `false` cancels the model turn and propagates through `XWORK_ERROR_CANCELLED`
- host service invocation now has a per-call context for run/cancel/interrupt metadata, and local `process.exec` can cooperatively cancel before spawn or while polling a running subprocess
- custom tool executors can now opt into `pfnToolExecEx` with a cancellation context and `xwork_tool_exec_context_should_cancel()` helper
- runs can now execute through a minimal async handle (`xwork_run_execute_async` / `xwork_run_async_*`) with wait, timed wait, status, cancel, and destroy APIs; async cancel feeds the same cooperative cancel token path and is covered against mock tools, unfinished-handle destroy, and local `process.exec`
- async run API docs now state the shallow-copy/lifetime contract for run/options/callback user data and the wait-timeout status semantics
- run execution now has a per-run execution guard, so concurrent `xwork_run_execute` entries on the same run fail with `XWORK_ERROR_INVALID_STATE`
- in-process multi-agent task graph baseline: `xwork_agent_pool`, agent roles, task nodes/dependencies, child-run mapping, max-concurrency fan-out/fan-in scheduling, failure propagation policies, cooperative cancel, scheduler pause/resume, per-agent retry, handoff request/result tracking with artifact refs, memory context refs, shared workspace refs, read-only/shared-writable policy flags, handoff audit events, handoff snapshot persistence/recovery, agent result and aggregate report artifacts using `xwork.report.v1`, child-run event audit entries, parent/agent/task run index filters, file-persisted agent pool and graph snapshots, snapshot-to-graph import, and persistence recovery for pending/completed graph state
- remote worker/control plane baseline: `xwork_control_plane`, worker registry/heartbeat/lease state, assignment queue, claim/complete/fail/cancel APIs, HTTP decoded-message transport marker, task policy/approval and network policy gates, secret redaction, capability matching plus control-plane capability allowlists, local worker `process.exec`, remote terminal start/list/stop, and filesystem host-tool execution with workspace-root and destructive-command policy enforcement, defined in-process worker-auth and single-tenant/project boundaries, result-attached artifact/diagnostics summary refs, artifact upload messages with blob refs/content hashes/chunk metadata/payload bytes, plane-owned blob chunk query/recovery, stdout/stderr output chunk upload/query/recovery, stale lease detection, orphaned assignment marking, worker/task query APIs, file-persisted control plane snapshots, and recovery that orphans in-flight assignments while keeping queued work resumable
- deterministic replay cassette baseline: `xwork_replay_engine`, record/load/replay entry APIs, typed filesystem snapshot/ref API, runtime host-service record/replay integration, replay event log schema for model stream/terminal/event sequence comparison, checkpoint seek, manifest/result/divergence summaries, strict and audit modes, side-effect blocking, cancel, first-divergence query, divergence report artifact emission, stable `fnv1a64` text hash, and file-persisted replay manifest/entry/result/raw-payload query and recovery
- provider smoke now exercises the async run handle on its main provider execution path while keeping local stub coverage for request/response normalization and an offline model-call failure path
- minimal local host helper for filesystem/process/vcs host services
- built-in host tool defs for `filesystem.read_text` / `filesystem.write_text` / `filesystem.list` / `filesystem.stat` / `filesystem.glob` / `filesystem.mkdir` / `filesystem.move` / `filesystem.delete` / `filesystem.apply_patch` / `process.exec` / `process.start_terminal` / `process.terminal_read` / `process.terminal_write` / `process.terminal_resize` / `process.terminal_stop` / `vcs.status` / `vcs.diff` / `vcs.log` / `vcs.branch`
- builtin host tool execution now auto-synthesizes output/command artifacts for read/write/process/vcs flows, including git status/diff/log/branch command artifacts
- builtin `process.exec` artifact synthesis now also emits a `process.diagnostics.json` report artifact for stderr, non-zero exit, and build/test-like commands using schema `xwork.diagnostics.v1`
- builtin terminal host tool execution now auto-synthesizes session command/output artifacts for start/write/stop flows
- local host now supports `filesystem.list` with request-level `path` / `recursive` / `include_hidden` / `limit`, and returns structured JSON entries plus pagination metadata
- local host now supports `filesystem.stat`, returning `exists`, path, file type, size, and mtime metadata as structured JSON
- local host now supports `filesystem.glob` with request-level `path` / `pattern` / `recursive` / `include_hidden` / `limit`, and returns bounded structured JSON matches
- local host now supports `filesystem.mkdir` with `recursive`, `exist_ok`, and `dry_run`
- local host now supports `filesystem.move` with `target_path`, `overwrite`, `recursive`, `create_dirs`, and `dry_run`
- local host now supports `filesystem.delete` with file/directory deletion, recursive directory deletion, and `dry_run`
- local host now supports `filesystem.apply_patch` as a single-file exact text replacement patch with `old_text`, `new_text`, `dry_run`, and structured conflict errors
- local host filesystem tools can now enforce a filesystem root plus allow/deny path prefixes, and denied paths return structured `path_denied` results with audit-friendly reasons
- local host process tools can now enforce command allow/deny patterns and optionally reject commands classified as destructive before spawning a subprocess
- public host services now include an explicit `XWORK_HOST_NETWORK` slot, and policy evaluation supports network host allow/deny patterns plus deny-by-default behavior for network side effects
- local host now supports read-only VCS tools: `vcs.status`, `vcs.diff` with working-tree/staged modes, `vcs.log` with bounded `limit`, and `vcs.branch` with current branch plus dirty state
- builtin `filesystem.list` / `filesystem.stat` / `filesystem.glob` / `filesystem.mkdir` / `filesystem.move` / `filesystem.delete` now also emit JSON output artifacts from orchestrator runs
- builtin `filesystem.apply_patch` now emits a patch artifact from orchestrator runs, including structured `xwork.patch_apply_result.v1` and `xwork.patch_file_summary.v1` JSON metadata
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
- JSON report artifacts can use schema `xwork.report.v1` with stable `report_kind`, `status`, `subject_ref`, `title`, `summary`, `body_markdown`, and `items` fields for plan/progress/final report flows shared by AI IDE and claw integrations
- planner support is intentionally a boundary in v1: callers can persist plan reports, pass planner context or plan JSON into orchestrator turns, and force `auto/none/required/named` tool choice, while the autonomous planner itself remains outside xwork
- diagnostics report artifacts use `XWORK_ARTIFACT_REPORT_DIAGNOSTICS` and carry minimal severity/source/location/message records derived from process output
- terminal session output artifacts now identify their JSON contracts with `xwork.terminal_state.v1` for session state windows and `xwork.terminal_inventory.v1` for terminal inventory results
- artifact summary query smoke now also covers `exit_code` and min/max `sequence` filters against a persisted command artifact
- builtin `process.exec` artifact synthesis now preserves stderr instead of dropping it from command artifacts
- command artifacts can now carry structured command I/O stats (`stdout_byte_count` / `stderr_byte_count` / stdout/stderr truncation flags) through runtime objects, summaries, snapshots, persistence, and workspace memory ingest
- local `process.exec` failure paths now preserve structured result payloads for invalid request / timeout / cancelled / non-zero exit cases, including requested stop policy and observed stop reason
- local `process.exec` stdin_text is bounded by configured `iMaxProcessInputBytes`
- local `process.exec` env list is bounded by configured `iMaxProcessEnvEntries`
- typed artifact emit helpers for patch / report / command / output
- artifacts with `content_text` now carry computed content stats (`content_byte_count` / `content_line_count`) through runtime objects, summaries, snapshots, persistence, and workspace memory ingest
- patch artifacts now carry computed patch stats (`file_count` / `hunk_count` / `added_line_count` / `deleted_line_count`) plus apply-result and per-file summary JSON through runtime objects, summaries, snapshots, persistence, and workspace memory ingest
- workspace memory attach and tool/artifact memory ingest hooks
- workspace memory can now be synced directly from a workspace root through `xllm_memory_sync_workspace`, with a small `xwork_workspace_memory_sync_summary` result surface
- workspace memory can also sync one changed file through `xllm_memory_sync_file`, with a compact change-kind summary for incremental update paths
- in-memory and file-backed checkpoint / snapshot persistence
- persisted run / event / checkpoint / artifact query surface
- built-in `xcode` / `xclaw` profiles with distinct defaults: `xcode` is semi-auto, low-risk auto-approval only, no default workspace memory, planner boundary off, and network-deny-by-default; `xclaw` is autonomous, high-risk auto-approval capable, workspace-memory enabled, planner boundary enabled, and also network-deny-by-default unless callers configure an allowlist
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

Recovery is snapshot-based. `xwork_runtime_recover_run_from_persistence()`
loads the latest persisted run snapshot, so a pending tool with an approved
approval decision can be resumed and re-executed from stored arguments. Live OS
process handles and interactive terminal sessions are not rehydrated after a
process restart; their persisted artifacts are durable audit/output records, and
host integrations must rediscover or restart live sessions explicitly.

Error mapping is intentionally narrow: invalid caller input returns
`XWORK_ERROR_INVALID_ARGUMENT`, invalid lifecycle usage returns
`XWORK_ERROR_INVALID_STATE`, unsupported capabilities or newer persistence
formats return `XWORK_ERROR_UNSUPPORTED`, missing durable objects return
`XWORK_ERROR_NOT_FOUND`, duplicate ids return `XWORK_ERROR_ALREADY_EXISTS`,
cooperative cancellation returns `XWORK_ERROR_CANCELLED`, and provider, host, or
persistence I/O failures that are outside caller control return
`XWORK_ERROR_EXTERNAL_FAILURE`. `xwork_version()` and `xwork_status_cstr()`
expose stable strings for logging and diagnostics.

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

## Runnable Examples

Product-oriented examples are available under [examples](/D:/git/xwork/examples):

- [examples/ai_ide_agent.c](/D:/git/xwork/examples/ai_ide_agent.c) uses the `xcode` profile, local filesystem host service, a mock `xllm` model turn, dry-run patch/report artifacts, and approval pause/resume.
- [examples/claw_autonomous_agent.c](/D:/git/xwork/examples/claw_autonomous_agent.c) uses the `xclaw` profile, `process.exec`, command/report artifacts, file persistence, and run recovery.
- [examples/multi_agent_claw.c](/D:/git/xwork/examples/multi_agent_claw.c) uses the `xclaw` profile with the P3 in-process multi-agent scheduler, child run report artifacts, agent/task graph persistence, recovery, and parent/agent/task run-index query.
- [examples/remote_worker_agent.c](/D:/git/xwork/examples/remote_worker_agent.c) uses the P3 remote worker/control plane, local `process.exec` worker execution, HTTP decoded-message transport marker, control-plane snapshot persistence, recovery orphaning, remote result/artifact summaries, artifact blob chunk recovery, and queued task continuation.
- [examples/replay_agent_run.c](/D:/git/xwork/examples/replay_agent_run.c) uses the P3 deterministic replay cassette, filesystem snapshot/ref records, checkpoint seek, strict replay, audit divergence, and replay report artifact emission.

Typical build/run commands:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\ai_ide_agent.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\ai_ide_agent.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\claw_autonomous_agent.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\claw_autonomous_agent.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\multi_agent_claw.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\multi_agent_claw.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\remote_worker_agent.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\remote_worker_agent.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\replay_agent_run.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\replay_agent_run.exe -lws2_32 -liphlpapi
examples\ai_ide_agent.exe
examples\claw_autonomous_agent.exe
examples\multi_agent_claw.exe
examples\remote_worker_agent.exe
examples\replay_agent_run.exe
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
- [tests/xwork_core_smoke.c](/D:/git/xwork/tests/xwork_core_smoke.c)
- [tests/xwork_host_smoke.c](/D:/git/xwork/tests/xwork_host_smoke.c)
- [tests/xwork_persistence_smoke.c](/D:/git/xwork/tests/xwork_persistence_smoke.c)
- [tests/xwork_profile_smoke.c](/D:/git/xwork/tests/xwork_profile_smoke.c)
- [tests/xwork_orchestrator_provider_smoke.c](/D:/git/xwork/tests/xwork_orchestrator_provider_smoke.c)
- [tests/xwork_stress_smoke.c](/D:/git/xwork/tests/xwork_stress_smoke.c)
- [tests/xwork_multi_agent_smoke.c](/D:/git/xwork/tests/xwork_multi_agent_smoke.c)
- [tests/xwork_remote_worker_smoke.c](/D:/git/xwork/tests/xwork_remote_worker_smoke.c)
- [tests/xwork_replay_smoke.c](/D:/git/xwork/tests/xwork_replay_smoke.c)

See [tests/README.md](/D:/git/xwork/tests/README.md) for the test groups,
optional real-provider environment variables, and the CI target mapping.

Packaging and compatibility notes are tracked in
[docs/PACKAGING.md](/D:/git/xwork/docs/PACKAGING.md),
[docs/COMPATIBILITY.md](/D:/git/xwork/docs/COMPATIBILITY.md), and
[CHANGELOG.md](/D:/git/xwork/CHANGELOG.md).
Builtin host tool contracts and examples are tracked in
[docs/HOST_TOOL_CONTRACTS.md](/D:/git/xwork/docs/HOST_TOOL_CONTRACTS.md) and
[docs/HOST_TOOL_EXAMPLES.md](/D:/git/xwork/docs/HOST_TOOL_EXAMPLES.md).
P3 full-capability tracking is recorded in
[P3_FUTURE_BOUNDARY_TRACKING_SPEC.md](/D:/git/xwork/P3_FUTURE_BOUNDARY_TRACKING_SPEC.md).
Remote worker/control-plane ownership, thread-safety, shutdown, transport,
wire JSON schema, and recovery boundaries are documented in
[docs/REMOTE_WORKER.md](/D:/git/xwork/docs/REMOTE_WORKER.md).
Deterministic replay entry/event contracts are documented in
[docs/REPLAY.md](/D:/git/xwork/docs/REPLAY.md).

Typical compile/run commands:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -c xwork.c
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_core_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_core_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_host_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_host_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_orchestrator_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_orchestrator_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_persistence_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_persistence_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_profile_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_profile_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_orchestrator_provider_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_orchestrator_provider_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_stress_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_stress_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_multi_agent_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_multi_agent_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_remote_worker_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_remote_worker_smoke.exe -lws2_32 -liphlpapi
gcc -std=c11 -Wall -Wextra -pedantic -Ilib\sqlite tests\xwork_replay_smoke.c lib\sqlite\sqlite3.c -o tests\xwork_replay_smoke.exe -lws2_32 -liphlpapi
tests\xwork_core_smoke.exe
tests\xwork_host_smoke.exe
tests\xwork_orchestrator_smoke.exe
tests\xwork_persistence_smoke.exe
tests\xwork_profile_smoke.exe
tests\xwork_orchestrator_provider_smoke.exe
tests\xwork_stress_smoke.exe
tests\xwork_multi_agent_smoke.exe
tests\xwork_remote_worker_smoke.exe
tests\xwork_replay_smoke.exe
```

The provider smoke now runs offline by default with local OpenAI-compatible,
Anthropic, and Ollama stubs. It can optionally hit a real provider when the
relevant environment variables are set, and `XWORK_PROVIDER_SMOKE_EXPECT_ERROR=1`
turns that real-provider run into an expected error-path smoke for bad
credentials, bad model ids, or a controlled error endpoint.
The real-provider runbook and drift log template are tracked in
[docs/PROVIDER_SMOKE.md](/D:/git/xwork/docs/PROVIDER_SMOKE.md).

## Non-Goals For This Stage

- production remote transport / cloud control plane
- rich editor integration
- production-grade persistence
- full autonomous planner

## Next Files To Read

- [DESIGN.md](/D:/git/xwork/DESIGN.md)
- [DEVELOPMENT_SPEC.md](/D:/git/xwork/DEVELOPMENT_SPEC.md)
- [xwork.h](/D:/git/xwork/xwork.h)
