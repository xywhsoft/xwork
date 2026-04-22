# Replay API

> Status: English draft, pending review.

Replay API provides deterministic replay cassettes for models, tools, host services, filesystem refs, checkpoints, and events.

## Related Declarations

- `xwork_replay_engine`
- `xwork_replay_options`
- `xwork_replay_entry_options`
- `xwork_replay_result`
- `xwork_replay_divergence`
- `xwork_replay_engine_create()`
- `xwork_replay_engine_record_entry()`
- `xwork_replay_engine_replay_entry()`
- `xwork_replay_hash_text()`
- `xwork_replay_hash_json()`

## Minimal Call

```c
xwork_replay_options tOptions;
xwork_replay_engine *pEngine = NULL;

xwork_replay_options_init(&tOptions);
tOptions.sReplayId = "replay-1";
tOptions.eMode = XWORK_REPLAY_MODE_RECORD;

xwork_replay_engine_create(&tOptions, &pEngine);
xwork_replay_engine_destroy(pEngine);
```

## Modes

| Mode | Meaning |
| --- | --- |
| `XWORK_REPLAY_MODE_RECORD` | Record entries and events. |
| `XWORK_REPLAY_MODE_STRICT` | Compare against cassette and stop on first mismatch. |
| `XWORK_REPLAY_MODE_AUDIT` | Compare and continue while recording divergences. |

## Related Docs

- [Persistence API](api-persistence.en.md)
- [Replay example](../case/replay-agent-run.en.md)
