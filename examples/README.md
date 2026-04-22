# examples

Runnable `xwork` integration examples.

## AI IDE Agent

[ai_ide_agent.c](/D:/git/xwork/examples/ai_ide_agent.c) demonstrates the
`xcode` profile as an AI IDE integration baseline:

- creates runtime, workspace, local host services, and run
- reads `README.md` through the filesystem host service
- emits a file-content artifact
- emits a dry-run patch artifact with `xwork.patch_apply_result.v1` and
  `xwork.patch_file_summary.v1`
- runs a mock `xllm` model turn that requests `filesystem.apply_patch`
- pauses for UI approval, submits approval, resumes, and completes the tool loop
- emits a final report artifact using `xwork.report.v1`

Build and run from the repo root:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\ai_ide_agent.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\ai_ide_agent.exe -lws2_32 -liphlpapi
examples\ai_ide_agent.exe
```

## claw Autonomous Agent

[claw_autonomous_agent.c](/D:/git/xwork/examples/claw_autonomous_agent.c)
demonstrates the `xclaw` profile as an autonomous agent baseline:

- creates runtime, workspace, local host services, file persistence, and run
- executes `process.exec`
- emits a command artifact
- emits a final report artifact using `xwork.report.v1`
- stores a run snapshot and recovers the run from file persistence

Build and run from the repo root:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\claw_autonomous_agent.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\claw_autonomous_agent.exe -lws2_32 -liphlpapi
examples\claw_autonomous_agent.exe
```

## Multi-Agent claw

[multi_agent_claw.c](/D:/git/xwork/examples/multi_agent_claw.c)
demonstrates the `xclaw` profile with the P3 in-process multi-agent scheduler:

- creates runtime, workspace, file persistence, agent pool, and task graph
- runs planner/coder/tester/reviewer child runs with fan-out/fan-in dependencies
- emits per-task report artifacts from each child run
- persists agent pool and task graph snapshots
- queries the persisted child run index by parent run, agent id, and task id
- recovers the completed task graph from file persistence

Build and run from the repo root:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\multi_agent_claw.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\multi_agent_claw.exe -lws2_32 -liphlpapi
examples\multi_agent_claw.exe
```

## Remote Worker Agent

[remote_worker_agent.c](/D:/git/xwork/examples/remote_worker_agent.c)
demonstrates the P3 remote worker/control plane baseline:

- creates runtime, local host services, file persistence, and control plane
- exercises the HTTP decoded-message transport marker used by host-owned
  socket/auth layers
- registers a local worker with the `process.exec` capability
- enqueues and executes a remote `process.exec` task through the worker loop
- queries remote task result summaries
- persists the control plane snapshot
- recovers the control plane and marks in-flight assignments as orphaned
- continues queued work after recovery

Build and run from the repo root:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\remote_worker_agent.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\remote_worker_agent.exe -lws2_32 -liphlpapi
examples\remote_worker_agent.exe
```

## Replay Agent Run

[replay_agent_run.c](/D:/git/xwork/examples/replay_agent_run.c)
demonstrates the P3 deterministic replay baseline:

- records model, checkpoint, and tool cassette entries
- records and replays a typed filesystem snapshot/ref entry
- loads recorded hashes into a strict replay engine
- seeks to a checkpoint and replays only the following tool entry
- runs audit replay with a deliberately divergent request
- emits the divergence report as a typed run report artifact

Build and run from the repo root:

```powershell
gcc -std=c11 -Wall -Wextra -pedantic -I. -Ilib\sqlite examples\replay_agent_run.c examples\xwork_example_runtime.c lib\sqlite\sqlite3.c -o examples\replay_agent_run.exe -lws2_32 -liphlpapi
examples\replay_agent_run.exe
```

`xwork_example_runtime.c` is a small example-only aggregation unit that enables
the bundled `xrt`, `xllm-session`, and `xllm-memory` implementations before
including `xwork.c`.
