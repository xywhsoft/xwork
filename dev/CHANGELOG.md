# Changelog

All notable source-level changes to `xwork` are recorded here.

## 0.1.0 - Unreleased

- Added the public runtime/workspace/tool/run object model.
- Added event, approval request, checkpoint, artifact, run snapshot, and persistence query surfaces.
- Added xllm-backed orchestrator execution with tool-loop, approval pause/resume, streaming model event forwarding, cancellation, and async run handles.
- Added local host services for filesystem, process, terminal, VCS, editor-buffer, diagnostics, policy, and approval integration.
- Added in-memory and file-backed persistence, checkpoint recovery, artifact summary queries, run index queries including parent/agent/task filters, and stress coverage.
- Added xcode and xclaw built-in profiles plus minimal AI IDE and claw examples.
- Added focused profile smoke coverage for built-in defaults, apply helpers, and bootstrap/run snapshot behavior.
- Added Windows/MSYS2 CI smoke matrix and optional real-provider smoke configuration.
- Added `xwork_version()` and standalone packaging, compatibility, API-freeze, provider-smoke, host-tool contract/example, persistence format documentation, and a focused newer-version persistence fixture.
- Added the first P3 multi-agent baseline: agent pools, task graph scheduling, child-run mapping, fan-out/fan-in, retry/cancel/pause/event/reconstructable-snapshot smoke coverage, file-persisted agent pool and task graph snapshots, snapshot-to-graph import, persistence recovery for pending/completed graph state, parent/agent/task run index filters, a multi-agent claw example, and xllm-memory/xrt callback compatibility glue.
- Added the first P3 remote worker/control plane baseline: control plane, worker register/heartbeat/lease state, assignment queue, HTTP decoded-message transport marker, capability matching, claim/complete/fail/cancel APIs, local worker `process.exec` and filesystem host-tool execution, remote artifact/diagnostics summary refs on task results, stale lease orphaning, worker/task query APIs, file-persisted control plane snapshots, recovery that orphans in-flight assignments while preserving queued/terminal tasks, remote worker example, and smoke/CI coverage.
- Added the P3 deterministic replay baseline: replay engine record/load/replay APIs, strict/audit modes, runtime host-service replay, typed filesystem snapshot/ref records, event-log comparison helpers, checkpoint seek, divergence report artifacts, file persistence, replay example, and smoke/CI coverage.
