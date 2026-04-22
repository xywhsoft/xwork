# xwork Documentation

> English documentation entry. Chinese documentation remains the primary source until this translation set is fully reviewed.

## Quick Links

- [API index](api/README.en.md)
- [Guide index](guide/README.en.md)
- [Examples index](case/README.en.md)
- [Architecture](ARCHITECTURE.en.md)
- [Best Practices](BEST_PRACTICES.en.md)
- [FAQ](FAQ.en.md)
- [Migration](MIGRATION.en.md)
- [Integration and Packaging](INTEGRATION.en.md)
- [Docs Review Checklist](DOCS_REVIEW_CHECKLIST.en.md)
- [Repository README](../README.en.md)

## Reading Paths

| Goal | Start Here |
| --- | --- |
| Understand xwork quickly | [First xwork program](guide/first-xwork-program.en.md) |
| Integrate an AI IDE agent | [xllm orchestration and tool loop](guide/xllm-orchestrator-intro.en.md) |
| Build a claw-like autonomous agent | [Tools, approval, and artifacts](guide/tool-approval-artifact-intro.en.md) |
| Use multi-agent scheduling | [Multi-agent task graph](guide/multi-agent-intro.en.md) |
| Use remote workers | [Remote worker and control plane](guide/remote-worker-intro.en.md) |
| Use recovery and replay | [Persistence, checkpoint, and replay](guide/persistence-replay-intro.en.md) |
| Look up public APIs | [API index](api/README.en.md), [`xwork.h`](../xwork.h) |

## Documentation Areas

| Area | Content |
| --- | --- |
| `docs/api/` | Public API documentation, organized by usage model. |
| `docs/guide/` | Tutorials and integration guides. |
| `docs/case/` | Runnable example walkthroughs. |
| `docs/*.md` | Architecture, best practices, FAQ, migration, and integration notes. |
| `dev/` | Development specs, historical notes, and internal contracts. |

## Version Mapping

This documentation targets the current repository state:

- Current [`xwork.h`](../xwork.h).
- Current `XWORK_PERSISTENCE_FORMAT_VERSION`.
- Current `XWORK_REMOTE_PROTOCOL_VERSION_CURRENT`.
- Current examples and smoke tests.

## Maintenance Rule

When public APIs, examples, persistence format, remote protocol, profiles, schemas, or built-in host tools change, update the corresponding docs in the same change.
