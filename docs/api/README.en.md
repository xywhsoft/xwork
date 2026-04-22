# xwork API Reference Index

> API docs are organized by usage model, not by declaration order in `xwork.h`. This index follows the `xllm` API reference standard.

## Writing Standard

Every public function declared with `XWORK_API` in `xwork.h` must have an independent `### function_name` section in exactly one API page.

Each function section must include:

- **Purpose:** what the function does and when to use it.
- **Prototype:** the exact declaration from `xwork.h`.
- **Parameters:** direction, nullability, lifetime, ownership, unit, range, and defaults.
- **Return Value:** all common `xwork_status` values or returned pointer/value semantics.
- **Ownership:** who owns returned objects, buffers, strings, handles, and reset/destroy obligations.
- **Notes:** call ordering, thread boundary, recovery boundary, side effects, and compatibility constraints.
- **Example:** minimal compilable C snippet when practical.
- **Related APIs:** neighboring init/reset/destroy/query functions.

Pages may additionally document constants, macros, enums, structs, opaque types, module lifecycle, common errors, thread boundaries, and recovery boundaries.

An API page is considered complete only when every function assigned to that page is covered by an independent section and the documented prototypes match `xwork.h`.

## Public API Scope

| Module | Main Objects / Capabilities |
| --- | --- |
| Common types | Status codes, versions, naming, and init/reset rules. |
| Runtime | `xwork_runtime`, runtime options, profiles, and `xllm` bootstrap. |
| Workspace | `xwork_workspace`, workspace roots, and workspace memory sync. |
| Tools | `xwork_tool_def`, tool registry, executors, and host-service bridge. |
| Run lifecycle | `xwork_run`, state, step, event, summary, sync/async execution. |
| Orchestrator | `xllm` model turn, tool loop, stream events, cancel, approval pause/resume. |
| Policy / Approval | `xwork_approval_request`, autonomy, filesystem/process/network policies. |
| Artifacts | `xwork_artifact`, summaries, typed metadata, reports, patches, and outputs. |
| Persistence | `xwork_persistence_backend`, snapshots, checkpoints, event/artifact queries. |
| Profiles | `xcode` and `xclaw` profile defaults and boundaries. |
| Multi-Agent | `xwork_agent_pool`, `xwork_task_graph`, dependencies, handoff, recovery. |
| Remote Worker | `xwork_control_plane`, workers, remote tasks, leases, chunks. |
| Replay | `xwork_replay_engine`, entries, manifest, filesystem refs, divergence. |
| xllm integration | Borrowed/owned `xllm_runtime`, sessions, memory, and cancel tokens. |
| Local host | Local filesystem/process/terminal boundaries built on host services and xrt. |

## API Pages

- [API Page Template](API_PAGE_TEMPLATE.en.md)
- [Common Types](types.en.md)
- [Runtime API](api-runtime.en.md)
- [Workspace API](api-workspace.en.md)
- [Tool API](api-tools.en.md)
- [Run API](api-run.en.md)
- [Orchestrator API](api-orchestrator.en.md)
- [Policy / Approval API](api-policy-approval.en.md)
- [Artifact API](api-artifacts.en.md)
- [Persistence API](api-persistence.en.md)
- [Host Tools API](api-host-tools.en.md)
- [Profiles API](api-profiles.en.md)
- [Multi-Agent API](api-multi-agent.en.md)
- [Remote Worker API](api-remote-worker.en.md)
- [Replay API](api-replay.en.md)
- [xllm Integration API](api-xllm-integration.en.md)
- [Local Host API](api-local-host.en.md)

## Reading Rules

1. Read the public contract at the top of [`xwork.h`](../../xwork.h) first.
2. Use `*_init()` before filling option/result structs.
3. Use matching `*_reset()` for result structs that own deep-copied data.
4. Use matching `*_destroy()` for opaque owned objects.
5. Treat getter-returned `const char *` and `const xwork_tool_def *` as borrowed unless documented otherwise.

Error semantics are defined by `xwork.h` and [Common Types](types.en.md). Module pages only describe common module-specific causes.
