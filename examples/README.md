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

`xwork_example_runtime.c` is a small example-only aggregation unit that enables
the bundled `xrt`, `xllm-session`, and `xllm-memory` implementations before
including `xwork.c`.
