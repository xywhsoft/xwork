# xwork Common Types API

> Version constants, status codes, and shared ownership rules used by all xwork modules.

[Back to API Index](README.en.md) | [Runtime API](api-runtime.en.md) | [Run API](api-run.en.md)

---

## Contents

- [Module Positioning](#module-positioning)
- [Version Constants](#version-constants)
- [Status Codes](#status-codes)
- [Ownership Rules](#ownership-rules)
- [Version and Status Functions](#version-and-status-functions)
  - [xwork_version](#xwork_version)
  - [xwork_status_cstr](#xwork_status_cstr)
- [Common Usage](#common-usage)
- [Common Errors](#common-errors)
- [Related Examples](#related-examples)

---

## Module Positioning

This page documents shared contracts used across all xwork modules:

- Version constants and format/protocol versions.
- `xwork_status` semantics.
- `init` / `reset` / `destroy` ownership rules.
- The two basic functions for version and status text lookup.

Runtime, workspace, tool, run, persistence, remote-worker, and replay object fields are documented in their module pages.

---

## Version Constants

| Constant | Current Value | Meaning | When to Use |
| --- | --- | --- | --- |
| `XWORK_VERSION_MAJOR` | `0` | Major version. `0.x` means the API is still converging. | Release gate, compatibility checks, diagnostics. |
| `XWORK_VERSION_MINOR` | `1` | Minor version. Usually increases when new capabilities are added. | Release gate and capability matrix. |
| `XWORK_VERSION_PATCH` | `0` | Patch version. Usually increases for fixes. | Diagnostics and build records. |
| `XWORK_PERSISTENCE_FORMAT_VERSION` | `14` | File persistence format version. | Store read/write, migration, compatibility checks. |
| `XWORK_REMOTE_PROTOCOL_VERSION_CURRENT` | `1` | Remote worker/control plane protocol version. | Worker registration and decoded transport messages. |

---

## Status Codes

### `xwork_status`

| Value | Meaning | Common Handling |
| --- | --- | --- |
| `XWORK_OK` | Operation completed. | Continue and inspect output parameters. |
| `XWORK_ERROR_INVALID_ARGUMENT` | Required pointer, ID, enum value, or option combination is invalid. | Fix caller input. |
| `XWORK_ERROR_NO_MEMORY` | Allocation failed. | Release resources, degrade, or abort the task. |
| `XWORK_ERROR_ALREADY_EXISTS` | A stable object ID already exists. | Use another ID or query the existing object. |
| `XWORK_ERROR_NOT_FOUND` | Object or persisted record does not exist. | Check IDs, store root, run ID, artifact ID. |
| `XWORK_ERROR_INVALID_STATE` | Object exists but its lifecycle state rejects the action. | Check run/graph/plane state and avoid duplicate execution. |
| `XWORK_ERROR_EXTERNAL_FAILURE` | xrt/xllm, provider, host service, filesystem, process, persistence, or callback failed. | Inspect events, artifacts, host results, or provider diagnostics. |
| `XWORK_ERROR_UNSUPPORTED` | Capability is valid but unavailable, disabled, or version-incompatible. | Check profile, host tool, remote protocol, persistence format. |
| `XWORK_ERROR_CANCELLED` | Cooperative cancellation was observed. | Treat run/task as cancelled, not failed. |
| `XWORK_ERROR_PAUSED` | Execution stopped at a resumable boundary. | Inspect approval/pending tool state and resume explicitly. |

---

## Ownership Rules

| Rule | Meaning |
| --- | --- |
| `*_init()` | Initializes caller-provided structs; usually safe for stack objects. |
| `*_reset()` | Releases deep-copied data inside a struct so it can be reused or discarded. |
| `*_destroy()` | Destroys opaque owned objects such as runtime, run, graph, control plane, or replay engine. |
| copied | xwork deep-copies input data during the call. |
| borrowed | xwork stores or temporarily uses a caller-owned pointer; the caller must keep it alive. |
| owned | xwork returns a new object/result; the caller must reset or destroy it. |

Getter-returned `const char *` and `const xwork_tool_def *` values are borrowed unless a function explicitly says otherwise.

---

## Version and Status Functions

### xwork_version

Get the xwork runtime version string.

**Functionality:**

Use this in logs, diagnostic reports, release gates, artifact metadata, or host UI so later debugging can identify which xwork build was used.

**Prototype:**

```c
XWORK_API const char *xwork_version(void);
```

**Parameters:**

None.

**Return Value:**

- Returns a static string, for example `"0.1.0"` in the current implementation.
- The caller must not free the returned value.

**Ownership:**

The returned value is a borrowed pointer with process lifetime.

**Notes:**

- Use `XWORK_VERSION_MAJOR`, `XWORK_VERSION_MINOR`, and `XWORK_VERSION_PATCH` if numeric version parts are needed.
- Do not use string comparison for compatibility checks.
- Use `XWORK_PERSISTENCE_FORMAT_VERSION` and `XWORK_REMOTE_PROTOCOL_VERSION_CURRENT` for persistence and remote-worker compatibility.

**Example:**

```c
#include "xwork.h"
#include <stdio.h>

int main(void)
{
    printf("xwork version: %s\n", xwork_version());
    return 0;
}
```

**Related API:**

- `XWORK_VERSION_MAJOR`
- `XWORK_VERSION_MINOR`
- `XWORK_VERSION_PATCH`

---

### xwork_status_cstr

Convert an `xwork_status` value to a stable string.

**Functionality:**

Use this in logs, failed assertions, smoke output, diagnostic artifacts, or host UI to avoid showing only numeric status values.

**Prototype:**

```c
XWORK_API const char *xwork_status_cstr(xwork_status eStatus);
```

**Parameters:**

| Parameter | Direction | Nullable | Description |
| --- | --- | --- | --- |
| `eStatus` | input | no | Status code to convert. Unknown numeric values return a fallback string. |

**Return Value:**

- Returns a static string such as `"XWORK_OK"`.
- Unknown values return `"XWORK_STATUS_UNKNOWN"`.
- The caller must not free the returned value.

**Ownership:**

The returned value is a borrowed pointer with process lifetime.

**Notes:**

- This function does not allocate and cannot fail.
- The string is for humans and diagnostics. Program logic should compare enum values directly.

**Example:**

```c
#include "xwork.h"
#include <stdio.h>

int main(void)
{
    xwork_status status = XWORK_ERROR_INVALID_ARGUMENT;
    printf("status: %s\n", xwork_status_cstr(status));
    return 0;
}
```

**Related API:**

- `xwork_status`
- `XWORK_OK`
- `XWORK_ERROR_INVALID_ARGUMENT`

---

## Common Usage

```c
printf(
    "xwork=%s persistence=%d remote=%d\n",
    xwork_version(),
    XWORK_PERSISTENCE_FORMAT_VERSION,
    XWORK_REMOTE_PROTOCOL_VERSION_CURRENT
);
```

```c
xwork_status status = xwork_runtime_create(&tOptions, &pRuntime);
if (status != XWORK_OK) {
    fprintf(stderr, "create runtime failed: %s\n", xwork_status_cstr(status));
}
```

## Common Errors

| Problem | Cause | Fix |
| --- | --- | --- |
| Using `strcmp(xwork_version(), "...")` for compatibility | Version string is for humans. | Use numeric constants or module-specific version constants. |
| Freeing the result of `xwork_version()` | The value is static and borrowed. | Do not free it. |
| Treating `XWORK_ERROR_PAUSED` as failure | Paused means resumable boundary. | Inspect approval/pending tool state and resume. |
| Treating `XWORK_ERROR_CANCELLED` as external failure | Cancelled means cooperative cancellation. | Mark the run/task as cancelled. |

## Related Examples

- `examples\first_xwork_program.c`
- `tests\xwork_core_smoke.c`
