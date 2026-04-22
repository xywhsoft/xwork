# xwork

`xwork` is a C agent workflow runtime library built on top of `xllm`.

Its goal is to extract reusable infrastructure for AI IDEs, autonomous command-line agents, remote workers, recoverable task flows, and deterministic replay. Product layers such as `xcode` and `xclaw` can build on top of it instead of re-implementing runtime, tool, approval, artifact, persistence, and orchestration primitives.

## Positioning

Default layering:

```text
xrt
xllm
xwork
xcode / xclaw
```

`xllm` owns model providers, sessions, memory, request/response handling, and streaming.

`xwork` orchestrates model calls around workspaces, tools, approval, task state, artifacts, persistence, remote execution, and replay.

## Core Capabilities

| Capability | Description |
| --- | --- |
| Runtime / Workspace | Runtime, workspace, profile, shared `xllm_runtime`, and workspace memory management. |
| Tool Registry | Model-callable tool definitions, executors, arguments, and cancellation context. |
| Orchestrator | Model turn + tool loop, tool calls, approval pause/resume, and final summary. |
| Policy / Approval | Policy and approval modeling for filesystem, process, network, terminal, and risky operations. |
| Host Tools | Built-in filesystem, process, terminal, VCS, and editor host tool contracts. |
| Artifacts | File content, patches, command output, terminal state, diagnostics, and structured reports. |
| Persistence | Run, event, checkpoint, artifact, agent graph, remote plane, and replay cassette storage. |
| Multi-Agent | In-process agent pool, task graph, dependency scheduling, handoff, and recovery boundary. |
| Remote Worker | Control plane, worker registry, lease, assignment, and artifact/output chunk protocol objects. |
| Deterministic Replay | Record/replay models, tools, checkpoints, filesystem refs, and divergence reports. |

## Current Boundaries

Usable today:

- Single-run runtime/workspace/tool/orchestrator loop.
- `xllm` model turn + tool loop, streaming events, approval pause/resume, and async cancellation.
- Local filesystem/process/terminal/VCS/editor host tool contracts.
- Artifacts, checkpoints, file persistence, and run/event/artifact queries.
- In-process multi-agent task graphs.
- In-process remote worker/control plane objects and decoded HTTP transport boundary.
- Deterministic replay cassette, filesystem snapshot/ref, and divergence report.

Still owned by the host product:

- UI, CLI, IDE panels, and human interaction.
- Real network server/client, worker auth, tenant/project isolation, and deployed control plane.
- Product-level autonomous planner policy.
- External database, object storage, or distributed multi-writer persistence backend.
- Long-term operations for model provider behavior drift.

## Documentation

- [Chinese documentation center](docs/README.md)
- [English documentation center](docs/README.en.md)
- [English API index](docs/api/README.en.md)
- [English guide index](docs/guide/README.en.md)
- [English examples index](docs/case/README.en.md)

## Quick Compile Check

Run from the repository root:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -c xwork.c
```

## Single Header

Generate the single-header distribution:

```powershell
.\build_single_head.bat
```

The generated file is `singlehead/xwork.h`. Define `XWORK_IMPLEMENTATION`
in exactly one translation unit before including it. The single header embeds
only xwork itself; provide `xrt`, `xllm`, and `sqlite` according to your build
layout.

Example build and run commands are documented in [examples/README.md](examples/README.md).

## Dependencies

| Dependency | Role |
| --- | --- |
| `xrt` | Base runtime, filesystem, process, terminal, and platform capabilities. |
| `xllm` | Providers, model requests/responses, sessions, memory, and streaming events. |
| `sqlite` | Local dependency used by examples and smoke tests. |
| Platform libraries | Windows examples usually link `ws2_32`, `iphlpapi`, and related system libraries. |

The repository `lib/` directory contains the dependency snapshot used by the current xwork build.

## Language Policy

Chinese docs are the primary source. English `.en.md` files are translated from reviewed Chinese drafts and kept structurally aligned.
