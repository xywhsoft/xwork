# xwork internal and historical documentation

> This directory saves design records, internal contracts, compatibility instructions and historical tracking data during the development process. The official user documentation entrance is at [`../../docs/README.md`](../../docs/README.md).

These documents can serve as source material for formal documentation, but do not serve as the main entry point for users. Stable content needs to be distilled into API, tutorial or example documentation under `docs/`.

Current documents:

- [PACKAGING.md](PACKAGING.md): source-level packaging, aggregate `xwork.c` usage, include/lib layout, and version rule.
- [COMPATIBILITY.md](COMPATIBILITY.md): current xwork/xllm/xrt/sqlite compatibility snapshot and dependency update rule.
- [API_FREEZE_0_1.md](API_FREEZE_0_1.md): 0.1.0 public API freeze checklist and release gate.
- [HOST_TOOL_CONTRACTS.md](HOST_TOOL_CONTRACTS.md): builtin local host tool JSON request/response contracts.
- [HOST_TOOL_EXAMPLES.md](HOST_TOOL_EXAMPLES.md): compact request/response examples for builtin host tools.
- [MULTI_AGENT.md](MULTI_AGENT.md): multi-agent ownership, thread-safety, scheduling, handoff, event, and recovery contract.
- [POLICY_APPROVAL.md](POLICY_APPROVAL.md): policy, approval, risk, network, remote worker, and replay contract.
- [REMOTE_WORKER.md](REMOTE_WORKER.md): remote worker/control-plane ownership, thread-safety, shutdown, transport, execution, artifact blob data-plane, and recovery contract.
- [PERSISTENCE_FORMAT.md](PERSISTENCE_FORMAT.md): file persistence layout, format version, recovery boundary, and query contract.
- [REPLAY.md](REPLAY.md): deterministic replay entry/event schema, divergence behavior, and terminal replay boundary.
- [PROVIDER_SMOKE.md](PROVIDER_SMOKE.md): optional real-provider smoke runbook and provider drift log template.

The detailed event, tool, and profile contracts are currently documented in
[`../../README.md`](../../README.md),
these internal notes, and the public documentation under [`../../docs/README.md`](../../docs/README.md).

The current API reference rewrite is tracked in
[`../API_REFERENCE_SPEC.md`](../API_REFERENCE_SPEC.md).
