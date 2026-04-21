# xwork File Persistence Format

This document records the v1 file persistence contract for the built-in
`xwork_file_persistence` backend.

## Scope

The file backend is intended for local durable agent runs:

- latest run recovery
- checkpoint recovery
- event audit logs
- artifact storage and summary queries
- run index and step history queries

It is not a distributed store, a multi-writer database, or a deterministic
replay platform.

## Root Layout

Configured root:

```text
<root>/
  runs/
    <hex-encoded-run-id>/
      latest.snapshot
      events.log
      checkpoints/
        <hex-encoded-checkpoint-id>.snapshot
      artifacts/
        <hex-encoded-artifact-id>.meta
```

Run ids, checkpoint ids, and artifact ids are hex-encoded before they are used
as filesystem names. Callers should treat paths as backend-owned implementation
details and use public query/load APIs instead of constructing paths manually.

## Format Versions

`XWORK_PERSISTENCE_FORMAT_VERSION` is currently `3`.

Snapshot/meta files use the current binary snapshot format header and version.
The loader behavior is:

- current or older supported version: load with compatibility defaults
- unknown newer version: return `XWORK_ERROR_UNSUPPORTED`
- corrupt or incomplete file: return `XWORK_ERROR_EXTERNAL_FAILURE`
- missing object: return `XWORK_ERROR_NOT_FOUND`

Event logs use a separate line-oriented header:

```text
#xwork-events\t1
```

Event log format version is currently `1`.

## Snapshot Files

Snapshot files are used for:

- `latest.snapshot`
- `checkpoints/<checkpoint-id>.snapshot`

The latest snapshot is the recovery boundary used by
`xwork_runtime_recover_run_from_persistence()`. It includes enough run state to
recover:

- run id, parent id, instruction, profile ids, autonomy, state
- workspace ids
- session policy
- pending tool call
- last approval request and approval decision state
- last checkpoint metadata
- last event metadata
- artifact summaries and typed artifact metadata
- run steps needed for query history

Live OS process handles and interactive terminal session handles are not
rehydrated. Persisted process/terminal artifacts are audit/output records; host
integrations must rediscover or restart live sessions explicitly.

## Event Log

`events.log` is append-oriented and stores run events in sequence order. It is
used for:

- event listing
- last event lookup
- step history reconstruction where applicable
- audit trail for approvals, checkpoints, retries, cancellation, failure, and
  completion

The backend tolerates older event-log versions where explicitly supported and
rejects unknown newer versions.

## Artifact Metadata

Artifacts are stored as `.meta` records under `artifacts/`.

The persisted artifact metadata includes:

- id, kind, name, mime, storage ref, summary
- output class and output role
- report class and report subject ref
- content text and content stats when present
- patch stats and patch apply/file summary JSON when present
- command text, command output text, exit code, and I/O stats when present
- sequence

Artifact content is stored inside the metadata record for the current backend.
External blob stores can be represented by `sStorageRef`, but the built-in file
backend does not manage a distributed artifact blob store.

## Atomicity

The file backend writes snapshot and artifact metadata through a temp-file then
rename flow. A half-written temp file must not become the committed object. The
durable smoke coverage includes corrupt/newer-file handling so callers get a
stable `xwork_status` instead of partially loaded objects.

`events.log` is append-oriented. A corrupt event record should be treated as a
backend failure rather than silently producing a misleading recovery state.

## Query Contract

The public query APIs are the stable interface:

- `xwork_file_persistence_list_runs`
- `xwork_file_persistence_list_run_summaries`
- `xwork_file_persistence_list_run_index`
- `xwork_file_persistence_query_run_index`
- `xwork_file_persistence_query_run_steps`
- `xwork_file_persistence_list_events`
- `xwork_file_persistence_load_event`
- `xwork_file_persistence_load_last_event`
- `xwork_file_persistence_list_checkpoints`
- `xwork_file_persistence_load_checkpoint`
- `xwork_file_persistence_load_last_checkpoint`
- `xwork_file_persistence_list_artifacts`
- `xwork_file_persistence_list_artifact_summaries`
- `xwork_file_persistence_query_artifact_summaries`
- `xwork_file_persistence_find_artifact_by_name`

Run index queries support filtering and sorting over persisted summaries,
approval/checkpoint presence, artifact/event counts, and state. Artifact summary
queries support filtering by kind, output/report metadata, name/mime/storage
prefixes, exit code, and sequence windows.

Pagination cursors are backend-owned values exposed through the public list
objects. Callers should pass them back to the next query and should not parse or
persist them as a stable external format.

## Compatibility Rule

When changing persisted fields:

1. Bump `XWORK_PERSISTENCE_FORMAT_VERSION` for incompatible or newly serialized fields.
2. Keep old-version reads working where possible by filling defaults.
3. Reject unknown newer versions with `XWORK_ERROR_UNSUPPORTED`.
4. Add or update smoke fixtures for corrupt, older, newer, and pagination cases.
5. Update this document and `docs/COMPATIBILITY.md`.

Focused current-format and newer-version rejection coverage lives in
`tests/xwork_persistence_smoke.c`. Broader corrupt/legacy/future event-log
coverage lives in `tests/xwork_orchestrator_smoke.c`.
