# Artifact Queries

> Status: English draft, pending review.

Artifact queries let products show reports, patches, command output, diagnostics, terminal state, and other durable results in UI or CLI.

## Query Boundary

Use runtime-level queries for product integration and file-backend queries for tools/smoke tests.

```text
xwork_runtime_query_artifact_summaries
xwork_runtime_query_persisted_artifact_summaries
xwork_file_persistence_query_artifact_summaries
```

## UI Rule

Show summaries in lists and load full content only when needed. Large output should be chunked or persisted as blob/artifact content rather than embedded everywhere.

## Next

- [Artifact API](../api/api-artifacts.en.md)
- [Persistence API](../api/api-persistence.en.md)
