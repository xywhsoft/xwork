# Changelog

All notable source-level changes to `xwork` are recorded here.

## 0.1.0 - Unreleased

- Added the public runtime/workspace/tool/run object model.
- Added event, approval request, checkpoint, artifact, run snapshot, and persistence query surfaces.
- Added xllm-backed orchestrator execution with tool-loop, approval pause/resume, streaming model event forwarding, cancellation, and async run handles.
- Added local host services for filesystem, process, terminal, VCS, editor-buffer, diagnostics, policy, and approval integration.
- Added in-memory and file-backed persistence, checkpoint recovery, artifact summary queries, run index queries, and stress coverage.
- Added xcode and xclaw built-in profiles plus minimal AI IDE and claw examples.
- Added focused profile smoke coverage for built-in defaults, apply helpers, and bootstrap/run snapshot behavior.
- Added Windows/MSYS2 CI smoke matrix and optional real-provider smoke configuration.
- Added `xwork_version()` and standalone packaging, compatibility, API-freeze, provider-smoke, host-tool contract/example, persistence format documentation, and a focused newer-version persistence fixture.
