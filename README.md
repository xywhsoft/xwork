# xwork v2

`xwork` is the C agent-runtime layer between `xllm-session` and product hosts such as `xcode`.

The v2 mainline is deliberately small. It implements the reliable single-agent loop needed for a usable coding agent before the project grows remote workers, multi-agent scheduling, replay, or database backends again. The previous broad implementation remains under `dev/v1` as design and implementation reference.

## Boundary

```text
xcode CLI / IDE host
        |
      xwork        agent loop, tools, policy, artifacts, compaction scheduling
        |
  xllm-session     context ledger, budgets, pruning, persistence, compaction transaction
        |
      xllm         one provider/model call, streaming normalization
        |
       xrt         HTTP/1.1, filesystem, subprocess, platform runtime
```

`xwork` is the only layer that executes model-requested tools. It borrows an `xllm_client` and an `xllm_session`; the host owns both and must keep them alive until the agent is destroyed.

## Current capabilities

- no hard agent-turn limit by default (`uMaxAgentTurns = 0`);
- repeated identical tool-batch and consecutive tool-failure loop guards;
- multiple tool calls per assistant response, executed in response order;
- assistant tool calls and tool results preserved as valid session pairs;
- automatic context-pressure checks and transactional summary compaction;
- explicit `xworkAgentCompact()` for a host-controlled safe compaction checkpoint;
- journal-backed atomic session checkpoints after prompts, assistant responses, tool batches, and compaction;
- explicit `xworkAgentResume()` recovery for interrupted model calls and partially completed tool batches, without appending a duplicate user prompt;
- streaming text/reasoning, model, tool, compaction, completion, and error events;
- cooperative cancellation;
- optional reference-counted operation context with inherited cancellation and monotonic deadlines;
- managed long-running processes with stable IDs, incremental output, stdin, stop, and cleanup;
- approval modes for automatic, callback-controlled, and read-only execution;
- structured per-tool permission decisions with risk and path/command/process resource metadata;
- before/after tool hooks over full, untruncated arguments and results;
- a completion verification gate that requires a successful command after the latest workspace mutation;
- workspace-contained filesystem path resolution;
- oversized tool output spill to `.xcode/artifacts`, with bounded head/tail context returned to the model;
- injectable model callback for deterministic offline tests.

Built-in tools:

| Tool | Purpose |
| --- | --- |
| `read_file` | Read UTF-8 text with line numbers and pagination. |
| `list_files` | Bounded directory listing with recursion and wildcard filtering. |
| `search_text` | Literal text search with path, wildcard, depth, and result limits. |
| `write_file` | Create, overwrite, or append files, optionally creating parents. |
| `replace_text` | Exact conflict-detecting text replacement. |
| `apply_patch` | Validate and transactionally apply multi-file create/replace/delete edits; each file is replaced atomically and earlier writes roll back on failure. |
| `exec_command` | Run a non-interactive shell command with cwd, timeout, exit metadata, bounded capture, and optional `expected_exit_codes` for negative tests. |
| `start_process` | Start a bounded-capture long-running process and return a stable process ID. |
| `poll_process` | Wait briefly and consume incremental stdout/stderr plus exit status. |
| `write_process` | Write text to process stdin or close stdin. |
| `stop_process` | Interrupt, terminate, kill, or kill the tree and release completed processes. |

Filesystem tools reject paths outside the configured workspace. `exec_command` starts inside the workspace, but it is a real shell and is not an OS sandbox. Hosts that do not fully trust commands should use `XWORK_APPROVAL_CALLBACK` or `XWORK_APPROVAL_READ_ONLY`.

Managed process IDs live for the lifetime of one `xwork_agent`. They intentionally are not serialized into the session checkpoint because OS process handles cannot be recovered safely after a host restart. Destroying the agent stops and releases every remaining managed process.

`exec_command` treats only exit code `0` as success by default. For a command that is expected to reject invalid input, pass a non-empty `expected_exit_codes` array such as `[1, 2]`; a normal exit matching the array is successful, while timeouts, cancellation, signals, and other abnormal exits remain failures.

Interrupted tool recovery is intentionally at-least-once: if a process stops after a side effect completes but before its result reaches the journal, that call is still pending and may be retried. Permission and hook checks run again. Hosts should favor idempotent operations and transactional `apply_patch` edits; managed OS processes cannot be reattached after restart. Recovery conservatively requires a fresh successful verification command before accepting completion.

## Minimal host setup

```c
xllm_client *client = /* configured provider client */;
xllm_session *session = /* new or loaded session */;
xwork_agent_config config;
xwork_error error;
xwork_agent *agent;
xwork_run_result result = {0};

xworkAgentConfigInit(&config);
config.pClient = client;
config.pSession = session;
config.pContext = operation_context;
config.sWorkspaceRoot = "D:/GIT/project";
config.sSessionPath = "D:/GIT/project/.xcode/session.json";
config.eApprovalMode = XWORK_APPROVAL_AUTO;

agent = xworkAgentCreate(&config, &error);
if (agent && xworkAgentRun(agent, "Inspect the project and fix its tests.", &result, &error) == XWORK_RESULT_OK) {
    puts(result.sFinalText);
}

xworkRunResultUnit(&result);
xworkAgentDestroy(agent);
```

The product host should render `xwork_event` values and install its own approval callback where human confirmation is required.

When `config.pContext` is non-NULL, the agent retains it until destruction and passes it to every normal and compaction model request. `xworkAgentCancel()` also cancels this context. A cancelled scope returns `XWORK_RESULT_CANCELLED`; an expired deadline returns `XWORK_RESULT_TIMEOUT` and `XWORK_ERROR_TIMEOUT`, including during provider transport, retry backoff, or a long `exec_command`. Scoped command waits interrupt, terminate, and finally kill the process tree instead of waiting for the tool timeout.

If startup inspection reports an interrupted durable run, call `xworkAgentResume(agent, &result, &error)` before accepting another prompt. A normal `xworkAgentRun` refuses to append new user input while the durable tail is waiting for a model response or has unresolved tool calls.

For finer control, set `OnPermission` in `xwork_agent_config`. It receives an `xwork_permission_request` for every tool call permitted by the hard read-only ceiling and may return `XWORK_PERMISSION_ALLOW`, `XWORK_PERMISSION_DENY`, or `XWORK_PERMISSION_DEFAULT` to fall back to the approval mode. `OnHook` brackets permitted tool execution; before-tool denial prevents execution, while after-tool denial marks the result failed and explicitly warns that completed side effects are not reversible. Transactional file rollback remains the responsibility of `apply_patch`.

`bRequireVerificationAfterWrite` is enabled by default. When a run mutates the workspace and then tries to finish without a successful `exec_command` after the latest edit, xwork appends a durable verification prompt and continues. `uCompletionVerificationRetries` bounds repeated premature completion attempts without imposing a general Agent turn limit.

## Build and test

From the repository root on Windows with GCC available:

```bat
build.bat
```

On Linux or macOS (set `CC` to select GCC, Clang, or a cross compiler):

```sh
sh build.sh
```

Cross builds use `RUN_TESTS=0`; sibling locations and flags are overrideable through
`XLLM_DIR`, `XRT_DIR`, `BUILD_DIR`, `RELEASE_DIR`, `CFLAGS`, `LDFLAGS`, and `LIBS`.

The optimized warning-as-error suite covers operation-deadline propagation, a forced context compaction followed by a multi-turn workflow using the built-in tools, transactional multi-file editing and rollback, managed-process stdin/output, artifact spill, session persistence, interrupted parallel-tool recovery, duplicate-prompt rejection, and a rejected workspace escape.
