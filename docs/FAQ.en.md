# FAQ

> Status: English draft, pending review.

## Is xwork a complete agent product?

No. xwork is an agent workflow runtime library. Product UI, CLI, planning behavior, cloud deployment, and user interaction belong to host products such as `xcode` or `xclaw`.

## How is xwork different from xllm?

`xllm` handles model providers, sessions, memory, and streaming. xwork wraps model calls in workspaces, tools, approval, artifacts, persistence, remote workers, and replay.

## Does remote worker include a network server?

No. xwork defines the control-plane and worker data model plus decoded transport boundary. The host product owns HTTP/socket implementation, authentication, retries, and deployment.

## Can recovery restore live processes or terminals?

No. Recovery restores serializable state such as snapshots, events, checkpoints, artifacts, task graphs, and control-plane state. Live OS handles and terminal sessions are not recovered.

## Is deterministic replay fully deterministic?

Replay is deterministic for recorded interactions and normalized hashes. It cannot guarantee exactly-once behavior for unrecorded external side effects.

## Should every tool be registered by default?

No. Register only the tools required by the active profile and workspace. Apply policy and approval before risky operations.
