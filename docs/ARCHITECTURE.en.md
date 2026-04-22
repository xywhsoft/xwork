# Architecture

> Status: English draft, pending review.

xwork sits between `xllm` and product layers such as `xcode` and `xclaw`.

```text
xrt
xllm
xwork
xcode / xclaw
```

## Layer Responsibilities

| Layer | Responsibility |
| --- | --- |
| `xrt` | Platform primitives: filesystem, process, terminal, threading, networking, and utilities. |
| `xllm` | Model provider adapters, sessions, memory, request/response handling, and streaming. |
| `xwork` | Agent workflow runtime: workspaces, tools, policy, artifacts, persistence, remote workers, replay. |
| `xcode` / `xclaw` | Product UI/CLI, planning behavior, user interaction, deployment, and product policy. |

## Core Object Model

```text
xwork_runtime
  workspaces
  tool registry
  profiles
  runs
  host services
  persistence backend
  replay engine

xwork_run
  events
  checkpoints
  artifacts
  model/tool loop state
```

## Orchestration Loop

```text
run execute
  xllm model turn
  tool call parsing
  policy / approval
  host tool execution
  event / artifact persistence
  summary
```

## Advanced Capabilities

Multi-agent, remote-worker, and replay capabilities are built on top of the same run/event/artifact/persistence primitives instead of separate subsystems.

## Boundaries

xwork does not implement product UI, cloud deployment, worker authentication, tenant isolation, or provider SDK behavior. These remain host-product responsibilities.
