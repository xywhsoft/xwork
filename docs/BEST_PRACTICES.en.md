# Best Practices

> Status: English draft, pending review.

## Keep Host Tool Policy Tight

Default to least privilege. Register only the tools needed by the current profile and workspace.

## Separate Read, Dry-Run, and Write

Prefer read-only inspection first. For edits, use dry-run patch artifacts before applying changes. Require approval for destructive or broad writes.

## Treat Process and Terminal as High Risk

`process.exec` and terminal tools should have timeouts, output limits, cancellation checks, working-directory validation, and approval gates.

## Design Artifacts for UI and Audit

Artifacts should be stable, queryable, and user-facing. Store summaries for lists and full content only when needed.

## Persist Recoverable State

Use checkpoints before risky operations and after meaningful milestones. Do not assume live process or terminal handles can be recovered.

## Use Replay for Audit, Not Magic

Replay can compare recorded model/tool/host interactions. It cannot make unrecorded external side effects deterministic.

## Choose Profiles Intentionally

Use `xcode` defaults for IDE-style guided workflows and `xclaw` defaults for autonomous command-line workflows. Override policy where the product needs stricter behavior.
