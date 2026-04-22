# docs

Additional documentation beyond the main design and development specs.

Current documents:

- [PACKAGING.md](/D:/git/xwork/docs/PACKAGING.md): source-level packaging, aggregate `xwork.c` usage, include/lib layout, and version rule.
- [COMPATIBILITY.md](/D:/git/xwork/docs/COMPATIBILITY.md): current xwork/xllm/xrt/sqlite compatibility snapshot and dependency update rule.
- [API_FREEZE_0_1.md](/D:/git/xwork/docs/API_FREEZE_0_1.md): 0.1.0 public API freeze checklist and release gate.
- [HOST_TOOL_CONTRACTS.md](/D:/git/xwork/docs/HOST_TOOL_CONTRACTS.md): builtin local host tool JSON request/response contracts.
- [HOST_TOOL_EXAMPLES.md](/D:/git/xwork/docs/HOST_TOOL_EXAMPLES.md): compact request/response examples for builtin host tools.
- [MULTI_AGENT.md](/D:/git/xwork/docs/MULTI_AGENT.md): multi-agent ownership, thread-safety, scheduling, handoff, event, and recovery contract.
- [POLICY_APPROVAL.md](/D:/git/xwork/docs/POLICY_APPROVAL.md): policy, approval, risk, network, remote worker, and replay contract.
- [REMOTE_WORKER.md](/D:/git/xwork/docs/REMOTE_WORKER.md): remote worker/control-plane ownership, thread-safety, shutdown, transport, execution, artifact blob data-plane, and recovery contract.
- [PERSISTENCE_FORMAT.md](/D:/git/xwork/docs/PERSISTENCE_FORMAT.md): file persistence layout, format version, recovery boundary, and query contract.
- [REPLAY.md](/D:/git/xwork/docs/REPLAY.md): deterministic replay entry/event schema, divergence behavior, and terminal replay boundary.
- [PROVIDER_SMOKE.md](/D:/git/xwork/docs/PROVIDER_SMOKE.md): optional real-provider smoke runbook and provider drift log template.

The detailed event, tool, and profile contracts are currently documented in
[README.md](/D:/git/xwork/README.md),
[DEVELOPMENT_SPEC.md](/D:/git/xwork/DEVELOPMENT_SPEC.md), and
[AGENT_RUNTIME_TRACKING_SPEC.md](/D:/git/xwork/AGENT_RUNTIME_TRACKING_SPEC.md).

P3 full-capability tracking is recorded in
[P3_FUTURE_BOUNDARY_TRACKING_SPEC.md](/D:/git/xwork/P3_FUTURE_BOUNDARY_TRACKING_SPEC.md).
