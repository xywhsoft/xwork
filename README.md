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
- streaming text/reasoning, model, tool, compaction, completion, and error events;
- cooperative cancellation;
- approval modes for automatic, callback-controlled, and read-only execution;
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
| `exec_command` | Run a non-interactive shell command with cwd, timeout, exit metadata, and bounded capture. |

Filesystem tools reject paths outside the configured workspace. `exec_command` starts inside the workspace, but it is a real shell and is not an OS sandbox. Hosts that do not fully trust commands should use `XWORK_APPROVAL_CALLBACK` or `XWORK_APPROVAL_READ_ONLY`.

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

## Build and test

From the repository root on Windows with GCC available:

```bat
build.bat
```

The optimized warning-as-error suite covers a forced context compaction followed by a multi-turn workflow using all six built-in tools, artifact spill, session persistence, and a rejected workspace escape.
