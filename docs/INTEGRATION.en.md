# Integration and Packaging

> Status: English draft, pending review.

## Build Shape

xwork exposes `xwork.h` and the aggregation implementation entry `xwork.c`. The repository also contains internal module sources under `src/`.

Minimal compile check:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -c xwork.c
```

## Dependencies

| Dependency | Role |
| --- | --- |
| `xrt` | Platform/runtime utilities, filesystem, process, terminal, and subprocess capability. |
| `xllm` | Model runtime, provider adapters, sessions, memory, and streaming. |
| `sqlite` | Local dependency used by examples and smoke tests. |
| Windows system libraries | Examples usually link `ws2_32` and `iphlpapi`. |

## Version Contracts

Persistence format is controlled by `XWORK_PERSISTENCE_FORMAT_VERSION`.

Remote worker protocol is controlled by `XWORK_REMOTE_PROTOCOL_VERSION_CURRENT`.

When these constants change, update persistence, replay, remote-worker, and migration docs in the same change.

## Host Responsibilities

The host product owns UI/CLI, secrets, provider configuration, network transport, worker authentication, tenant isolation, and product-level policy.
