# Replay Agent Run Example

> Status: English draft, pending review.

This example maps to [`examples/replay_agent_run.c`](../../examples/replay_agent_run.c). It demonstrates record/strict/audit replay and divergence reporting.

## What It Demonstrates

- Replay cassette recording.
- Strict replay.
- Audit replay with divergence report.
- Filesystem ref and hash comparison.

## Key APIs

- `xwork_replay_engine_create()`
- `xwork_replay_engine_record_entry()`
- `xwork_replay_engine_replay_entry()`
- `xwork_replay_hash_json()`
- `xwork_file_persistence_load_replay_engine()`

## Modes

| Mode | Use |
| --- | --- |
| Record | Capture model/tool/host interactions. |
| Strict | Stop at the first mismatch. |
| Audit | Continue and report divergences. |
