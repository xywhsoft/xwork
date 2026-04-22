# Persistence, Checkpoint, and Replay

> Status: English draft, pending review.

Persistence stores recoverable run state. Checkpoints define recovery boundaries. Replay records model/tool/host interactions so a run can be compared later.

## Layers

| Layer | Purpose |
| --- | --- |
| Persistence | Save snapshots, events, checkpoints, artifacts, graphs, remote planes, and replay cassettes. |
| Checkpoint | Mark recoverable points inside a run. |
| Replay | Record and compare deterministic interaction entries and filesystem refs. |

## File Persistence

```c
xwork_file_persistence_options tOptions;
xwork_file_persistence tStore;
xwork_persistence_backend tBackend;

xwork_file_persistence_options_init(&tOptions);
xwork_file_persistence_init(&tStore);
xwork_persistence_backend_init(&tBackend);

tOptions.sRootPath = ".xwork_store";
xwork_file_persistence_configure_backend(&tStore, &tOptions, &tBackend);
```

## Recovery Boundary

Snapshots restore serializable state. Live OS process handles, interactive terminal sessions, native thread stacks, and callback stacks are not restored.

## Next

- [Persistence API](../api/api-persistence.en.md)
- [Replay API](../api/api-replay.en.md)
- [Replay example](../case/replay-agent-run.en.md)
