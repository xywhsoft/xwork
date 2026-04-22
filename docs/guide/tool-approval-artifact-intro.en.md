# Tools, Approval, and Artifacts

> Status: English draft, pending review.

Tools let the model request actions. Policy/approval decides whether those actions may run. Artifacts record durable outputs for UI, recovery, and audit.

## Flow

```text
model tool call
  tool registry lookup
  policy decision
  approval request if needed
  tool execution
  artifact/event recording
```

## Register a Tool

```c
xwork_tool_def tDef;

xwork_tool_def_init(&tDef);
tDef.sToolId = "example.echo";
tDef.eKind = XWORK_TOOL_VIRTUAL;
tDef.eSideEffect = XWORK_TOOL_SIDE_EFFECT_NONE;

xwork_runtime_register_tool(pRuntime, &tDef);
```

## Approval Boundary

Approval requests are data objects. xwork does not render dialogs or CLI prompts. The host product displays the request and resumes the run with an approval decision.

## Artifact Boundary

Artifacts should contain user-visible or audit-relevant results: patches, command output, reports, diagnostics, terminal state, and file summaries.

## Next

- [Tool API](../api/api-tools.en.md)
- [Policy / Approval API](../api/api-policy-approval.en.md)
- [Artifact API](../api/api-artifacts.en.md)
