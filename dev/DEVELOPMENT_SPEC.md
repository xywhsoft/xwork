# xwork 开发计划 Spec v1

## 1. 文档目的

本文档用于指导 `xwork` v1 的实际开发推进。

它不重复回答“`xwork` 应该是什么”，这部分由 [DESIGN.md](/D:/git/xwork/DESIGN.md) 定义。

本文档回答的是：

- 先做什么，后做什么
- 每个模块当前要交付什么
- 每个阶段如何验收
- 哪些内容属于 v1 必做
- 哪些内容可以延后

## 2. 当前基线

当前仓库已经具备：

- 设计基线：[DESIGN.md](/D:/git/xwork/DESIGN.md)
- 项目入口说明：[README.md](/D:/git/xwork/README.md)
- Agent runtime 进度跟踪：[AGENT_RUNTIME_TRACKING_SPEC.md](/D:/git/xwork/AGENT_RUNTIME_TRACKING_SPEC.md)
- 公共 API 草案：[xwork.h](/D:/git/xwork/xwork.h)
- 公共实现入口：[xwork.c](/D:/git/xwork/xwork.c)
- 已落成的最小共享 runtime 主线：
  - runtime / workspace / tool registry / run lifecycle
  - event / approval request / checkpoint / artifact 公共对象
  - `xllm` model-turn + tool-loop 最小编排闭环
  - approval pause/resume 与 checkpoint load/recover
  - workspace memory attach / tool-result ingest / artifact ingest 入口
- workspace root -> `xllm_memory_sync_workspace` 的最小同步入口
- workspace file -> `xllm_memory_sync_file` 的最小增量同步入口
- workspace memory sync policy 已暴露 allowed extensions、ignored directories、ignored extensions、ignored path patterns、ignored file names 与 max file bytes；xwork 负责把 public workspace options 复制并转发给 xllm，二进制内容过滤仍由 xllm memory ingest 侧执行，xwork 调用方可组合扩展名白名单和大小上限得到稳定同步边界
- 默认 memory context 注入策略已固定：当 `xwork_orchestrator_options.pfnResolveMemoryContext == NULL` 且 run 绑定了启用 memory 的 workspace 时，xwork 使用 run instruction 作为 query 搜索每个 workspace memory，并把 `xllm_memory_apply_search_to_request` 生成的 block 文本合并为一个 `XLLM_CONTEXT_MEMORY` model-turn context block；`iMemorySearchMaxHits / iMemoryContextMaxBlocks / iMemoryContextMaxCharsPerHit / iMemoryContextMaxTotalChars / iMemoryContextPriority / bMemoryContextPinned` 控制检索数量、字符预算、priority 和 pinned。当前 token 预算以 xllm memory context 的字符预算作为稳定 proxy，后续若 xllm 暴露 tokenizer-backed token limit 再切换为真实 token。
- artifact memory ingest policy 已固定：`bIngestArtifactsToMemory` 开启后，`uArtifactMemoryIngestKindMask / uArtifactMemoryIngestOutputClassMask / uArtifactMemoryIngestReportClassMask` 可按 kind、output class、report class 过滤，mask 为 0 表示不限制；默认跳过名称、摘要、正文、命令或 patch metadata 中含 password/secret/token/private key/credential 等敏感特征的 artifact，只有显式设置 `bIngestSensitiveArtifactsToMemory` 才允许进入 memory。
- typed patch/report/command/output artifact emit helper
- in-memory persistence 与 file persistence backend
- persisted run/event/checkpoint/artifact 查询面
- P3 remote worker/control plane 已支持 result 附带 artifact/diagnostics summary refs，并支持 artifact upload message 携带 blob ref、content hash、chunk 元数据和 payload bytes；artifact blob chunks 可查询并随 control-plane snapshot 持久化/恢复；stdout/stderr output chunk 也可上传、查询并恢复；in-process worker 也可通过 capability allowlist 执行 remote terminal start/list/stop 最小闭环；ownership/thread-safety/shutdown/transport/wire JSON schema/recovery contract 已落在 `docs/REMOTE_WORKER.md`
- P3 deterministic replay 已有 cassette baseline：record/load/replay entry、runtime host-service record/replay、event log schema、model stream/terminal event sequence replay、checkpoint seek、strict/audit divergence、divergence report artifact、manifest/result/query、raw payload persistence store/list/load/recover 和 stable text hash
- `xcode` / `xclaw` 内建 profile
- `examples/ai_ide_agent.c`、`examples/claw_autonomous_agent.c` 和 `examples/multi_agent_claw.c` 最小产品接入样例
- 内部模块目录：
  - [src/xwork_core](/D:/git/xwork/src/xwork_core)
  - [src/xwork_workspace](/D:/git/xwork/src/xwork_workspace)
  - [src/xwork_tools](/D:/git/xwork/src/xwork_tools)
  - [src/xwork_agents](/D:/git/xwork/src/xwork_agents)
  - [src/xwork_remote](/D:/git/xwork/src/xwork_remote)
  - [src/xwork_replay](/D:/git/xwork/src/xwork_replay)
  - [src/xwork_orchestrator](/D:/git/xwork/src/xwork_orchestrator)
  - [src/xwork_policy](/D:/git/xwork/src/xwork_policy)
  - [src/xwork_persistence](/D:/git/xwork/src/xwork_persistence)
  - [src/xwork_artifacts](/D:/git/xwork/src/xwork_artifacts)
  - [src/xwork_host](/D:/git/xwork/src/xwork_host)
  - [src/xwork_profiles](/D:/git/xwork/src/xwork_profiles)

当前状态已经不是纯 bootstrap，而是“最小闭环已具备、正在补硬化层”的阶段。

距离完整共享 runtime 仍有明显差距，但 v1 最小对象边界和主流程已经站住。

## 3. 开发目标

`xwork` v1 的目标不是一步做成完整产品，而是先形成可复用的共享 runtime 闭环，满足：

1. 能创建 runtime / workspace / run
2. 能统一注册和发现工具
3. 能在统一状态机里推进 run / step
4. 能围绕 `xllm` 执行一次最小编排闭环
5. 能记录关键事件并保存 checkpoint
6. 能分别支撑 `xcode` 和 `xclaw` 的上层接入

## 4. 外部依赖与约束

### 4.1 依赖关系

`xwork` 的上游依赖应为：

- `xrt`
- `xllm`

其中：

- `xrt` 提供基础运行时能力
- `xllm` 提供模型调用、session、memory 能力

### 4.2 边界约束

`xwork` 不应重新实现：

- provider adapter
- request / response normalization
- `session` compaction
- memory retrieval engine

### 4.3 Public API 命名与结构审查结论

当前 `xwork.h` 的 v1 public API 命名和结构按以下规则冻结：

- `xwork_status_cstr()` 保持现名，用于强调返回值是非拥有的稳定 C string literal，适合日志和诊断输出。
- `xwork_runtime_*persisted*` 是 runtime 级便捷查询入口，`xwork_file_persistence_*` 是 file backend 直连入口，两组命名保留以区分抽象层级。
- `*_summary`、`*_summary_list`、`*_query`、`*_index` 命名保留，分别表示单对象摘要、摘要列表、查询条件和可排序索引视图。
- `xwork_run_async_*` 命名保留，async handle 的 ownership / cancel / wait / destroy 语义已经由 `xwork.h` 注释定义。
- `xwork_model_event` 中的 `eType` 继续保持 `int`，这是到 `xllm` model event type 的轻量桥接，不在 `xwork` 内重复定义 provider event enum。
- `xwork_file_persistence` 与 `xwork_local_host` 是 caller-owned helper state struct。调用方负责 init/configure/reset，但 configure 之后不应直接改写其内部状态字段。
- 当前未发现必须在 v1 前重命名的临时 public symbol。后续若要迁移为 opaque helper handle，应作为兼容性破坏变更单独规划。

### 4.4 代码组织约束

建议继续沿用与 `xllm` 类似的源码级组织方式：

- 根 `xwork.h` 作为唯一公共头
- 根 `xwork.c` 作为公共聚合实现入口
- `src/*` 存放内部子模块

## 5. 模块职责

### 5.1 `xwork_core`

负责：

- 版本信息
- 基础对象
- 错误码
- run / step 状态模型
- 公共辅助逻辑

### 5.2 `xwork_workspace`

负责：

- workspace 注册与查找
- root path 与基本元数据
- workspace 列表和引用关系
- 后续 change set / snapshot 的接入入口

### 5.3 `xwork_tools`

负责：

- tool registry
- tool 定义验证
- tool 查找与元数据管理

### 5.4 `xwork_orchestrator`

负责：

- run engine
- step scheduling
- 与 `xllm` 的编排
- tool loop 编排

### 5.5 `xwork_policy`

负责：

- autonomy mode
- approval
- side-effect policy
- path / command / network policy

### 5.6 `xwork_persistence`

负责：

- event log
- checkpoint store
- run snapshot

### 5.7 `xwork_artifacts`

负责：

- patch artifact
- report artifact
- command artifact
- output artifact

当前最小基线：

- 通用 artifact 对象已能携带 `content_text / command_text / exit_code`
- 公共 helper 已补 `emit_patch/report/command/output_artifact`
- 带 `content_text` 的 artifact 已补通用内容统计：`content_byte_count / content_line_count`，并贯穿 runtime 对象、summary、snapshot、file persistence 与 workspace memory ingest
- command artifact 已补最小结构化 I/O 统计：`stdout_byte_count / stderr_byte_count / stdout_truncated / stderr_truncated`，并贯穿 runtime 对象、summary、snapshot、file persistence 与 workspace memory ingest
- patch artifact 已补最小结构化统计：`file_count / hunk_count / added_line_count / deleted_line_count`，并携带 `xwork.patch_apply_result.v1` apply result 与 `xwork.patch_file_summary.v1` per-file summary JSON，贯穿 runtime 对象、summary、snapshot、file persistence 与 workspace memory ingest
- report/output artifact 已补最小 typed output metadata：`output_class / output_role`，并贯穿 runtime 对象、summary、snapshot、file persistence、summary query 与 workspace memory ingest；builtin filesystem/terminal bridge 会填充 file-content/file-change/terminal-state/terminal-inventory 分类
- report artifact 已补专用 typed metadata：`report_class / report_subject_ref`，并贯穿 runtime 对象、summary、snapshot、file persistence、summary query 与 workspace memory ingest
- report artifact JSON 内容可使用 `xwork.report.v1`，固定 `report_kind / status / subject_ref / title / summary / body_markdown / items` 字段，用于 AI IDE 与 claw 共享 plan/progress/final report contract
- planner 在 v1 中是边界能力而不是完整 autonomous planner：上层可以用 plan report artifact 记录计划，用 orchestrator options 注入 planner context / plan JSON，并通过 tool choice 控制下一轮模型可用工具策略；具体 planner 生成逻辑留给 AI IDE / claw。
- diagnostics artifact 已有最小 schema：`xwork.diagnostics.v1`，通过 `XWORK_ARTIFACT_REPORT_DIAGNOSTICS` report artifact 承载 `severity/source/location/message` 记录

### 5.8 `xwork_host`

负责：

- filesystem bridge
- process bridge
- vcs bridge
- diagnostics bridge
- editor bridge

### 5.9 `xwork_profiles`

负责：

- `xcode` interactive profile
- `xclaw` autonomous profile
- 默认策略组合

当前最小基线：

- `xcode` 默认 semi-auto、低风险 auto-approval 上限、关闭 workspace memory、关闭 planner boundary、关闭默认 auto-approve，并对未声明网络访问 deny-by-default。
- `xclaw` 默认 autonomous、高风险 auto-approval 上限、启用 workspace memory、启用 planner boundary、启用默认 auto-approve，并对未声明网络访问 deny-by-default。
- profile smoke 已覆盖关键默认值、runtime/workspace/run/orchestrator options 应用，以及默认 approval 行为差异。
- `examples/ai_ide_agent.c` 已覆盖 `xcode` profile、local filesystem host、example-only mock `xllm` model turn、dry-run patch/report artifacts，以及 `filesystem.apply_patch` 的 approval pause / submit / resume 闭环。
- `examples/claw_autonomous_agent.c` 已覆盖 `xclaw` profile、`process.exec`、command/report artifacts、file persistence snapshot 和 recovery。
- `examples/multi_agent_claw.c` 已覆盖 `xclaw` profile 下的 agent pool/task graph、planner/coder/tester/reviewer fan-out/fan-in、child run report artifacts、agent/task graph persistence、parent/agent/task run-index query 和 graph recovery。
- `examples/remote_worker_agent.c` 已覆盖 P3 remote worker/control plane：local worker `process.exec`、control-plane snapshot persistence、recovery orphaning 和 queued task continuation。
- `examples/replay_agent_run.c` 已覆盖 P3 deterministic replay：recorded cassette、checkpoint seek、strict replay、audit divergence 和 divergence report artifact。

## 6. 开发策略

整体策略：

1. 先固定共享对象模型
2. 再固定 host/tool/policy 边界
3. 再做 `xllm` 编排闭环
4. 再补 persistence / checkpoint
5. 最后补产品 profile 和更复杂的 orchestration

避免一开始同时推进：

- 全量工具系统
- 多 agent 并行
- 完整持久化
- editor 深度集成

这样会导致设计失焦。

## 7. 里程碑拆分

### M0: 核心对象可编译

目标：

- `xwork.h` 与 `xwork.c` 可独立编译
- runtime / workspace / run / tool 的最小对象闭环成立
- init / reset / create / destroy 都有最小实现

验收标准：

- `gcc -std=c11 -Wall -Wextra -c xwork.c` 通过

### M1: 共享注册模型可用

目标：

- workspace 可注册和查找
- tool 可注册和查找
- run 可创建和推进生命周期

### M2: host service 抽象

目标：

- 定义 filesystem / process / vcs 基础抽象
- 不先实现全部功能，但先固定接口和 ownership

当前最小基线：

- `xwork_host_service` / `xwork_host_services` 已接入 runtime
- `xwork_local_host` 已能跑通最小 filesystem/process/vcs dispatch
- builtin host tools 已有 `filesystem.read_text` / `filesystem.write_text` / `filesystem.list` / `filesystem.stat` / `filesystem.glob` / `filesystem.mkdir` / `filesystem.move` / `filesystem.delete` / `filesystem.apply_patch` / `process.exec` / `process.start_terminal` / `process.terminal_read` / `process.terminal_write` / `process.terminal_resize` / `process.terminal_stop` / `vcs.status` / `vcs.diff` / `vcs.log` / `vcs.branch`
- builtin host tool 在 orchestrator 中已能自动落最小 output/command artifact，VCS status/diff/log/branch 会合成 command artifact
- builtin terminal host tool 在 orchestrator 中已能自动落最小 session command/output artifact（start/write/stop）
- `filesystem.list` 已支持 `path` / `recursive` / `include_hidden` / `limit`，返回结构化 entries 和分页元数据
- `filesystem.stat` 已支持文件/目录/不存在路径的结构化探测，返回 type、size、mtime 等元数据
- `filesystem.glob` 已支持带 `pattern`、递归开关、隐藏文件开关和数量限制的本地搜索
- `filesystem.mkdir` 已支持递归创建、已存在处理和 dry-run
- `filesystem.move` 已支持 target path、overwrite、recursive overwrite、create_dirs 和 dry-run
- `filesystem.delete` 已支持文件/目录删除、递归目录删除和 dry-run
- `filesystem.apply_patch` 已支持单文件 exact text replacement patch：`old_text` / `new_text` / `dry_run`，并对 conflict 返回结构化错误
- local host filesystem 工具已支持可选路径策略：filesystem root、allow path prefixes、deny path prefixes；越界路径会返回结构化 `path_denied`
- local host process 工具已支持可选命令策略：command allow patterns、deny patterns、destructive command 拒绝；拒绝会在 spawn 前返回结构化 `command_denied` 或 `destructive_command`
- host service 公共边界已包含 `XWORK_HOST_NETWORK`，policy 层已支持 network host allow/deny patterns 和 deny-by-default 评估，用于约束联网副作用
- host service 公共边界已包含 diagnostics service 槽位，当前先落成 `from_process_output` contract 与 process output -> diagnostics report artifact 的 orchestrator bridge
- local host VCS 工具已支持 `status`、working-tree/staged `diff`、带 `limit` 的 `log`、以及返回当前 branch 和 dirty 状态的 `branch`
- builtin `filesystem.list/stat/glob/mkdir/move/delete` 已能在 orchestrator 中合成 JSON output artifact
- builtin `filesystem.apply_patch` 已能在 orchestrator 中合成 patch artifact，并填充 apply result / per-file summary schema metadata
- `filesystem.write_text` 最小 request contract 已支持 `mode=append`
- `filesystem.write_text` 最小 request contract 已支持 `mode=create`
- `filesystem.write_text` 最小 request contract 已支持 `create_dirs:true`
- `filesystem.read_text` 最小 request contract 已支持 `offset_bytes`
- `filesystem.read_text` 最小 response contract 已显式返回 `file_size_bytes` / `remaining_bytes` / `eof` / `next_offset_bytes`
- `filesystem.read_text/write_text` 失败路径已保留最小结构化结果，可区分 not_found / already_exists / parent_not_found
- orchestrator 已能把 `xllm` stream preference、model event callback、xllm cancel token 和协作式 interrupt check 接入同步 model-turn/tool-loop；取消会落 cancelled checkpoint 和 `RUN_CANCELLED` event
- model event 取消优先级已明确：event bridge 先检查 interrupt/cancel token，再调用用户 model event callback；callback 返回 `false` 会触发 cancel token（如存在）并统一收口为 `XWORK_ERROR_CANCELLED`
- host service invoke 已有 per-call context，可把 run/cancel/interrupt 元数据传入 builtin host；local `process.exec` 已能在 spawn 前和 subprocess polling wait 中协作取消
- 自定义 tool executor 已有 `pfnToolExecEx` 扩展回调、`xwork_tool_exec_context` 与 `xwork_tool_exec_context_should_cancel()` helper，可自行轮询取消并让 orchestrator 收口为 cancelled run
- orchestrator 已提供最小 async run handle：`xwork_run_execute_async` / `xwork_run_async_wait` / `xwork_run_async_wait_timeout` / `xwork_run_async_get_status` / `xwork_run_async_cancel` / `xwork_run_async_destroy`，内部复用同步 loop，并通过 owned 或外部 `xllm_cancel_token` 接入协作式取消；smoke 已覆盖 mock tool、未完成 handle destroy 与 local `process.exec` subprocess 取消
- async run 公共 API 已写明 run/options/callback user data 的浅拷贝生命周期约束，以及 wait timeout 与未完成状态读取语义
- run execution 已有 per-run execution guard，同一个 run 的并发 `xwork_run_execute` 入口会返回 `XWORK_ERROR_INVALID_STATE`
- provider / host / persistence 错误统一收敛到窄的 `xwork_status` 集合：caller contract 问题走 `INVALID_ARGUMENT / INVALID_STATE / NOT_FOUND / ALREADY_EXISTS / UNSUPPORTED`，协作取消走 `CANCELLED`，外部 I/O、provider、host 或 persistence 后端失败走 `EXTERNAL_FAILURE`；`xwork_status_cstr()` 提供稳定日志名称。
- provider smoke 主执行路径已切到 async run handle，保留 local stub provider matrix 验证 request/response normalization，并补了离线 model-call failure 路径；provider error-path matrix 已覆盖 upstream_5xx/auth/rate_limit/parse/timeout，orchestrator 统一收口为 `XWORK_ERROR_EXTERNAL_FAILURE` + `XWORK_RUN_FAILED`，错误 summary 固定包含 `xllm_error=<code>` 和 provider message
- session compaction knobs 已进入 public `xwork_session_policy`：auto compact、trigger ratio/turns、reserve output tokens、keep recent turns、keep active tool chain、compact strategy。orchestrator 在 model turn 成功提交后按 policy 调用 `xllm_session_compact()`，并在真实 compact 后写入 `XWORK_CHECKPOINT_SESSION_COMPACTED` 与 `XWORK_EVENT_SESSION_COMPACTED`；persistence snapshot format 已升到 v14，以保存新增 session policy 字段、run-level agent/task id、task graph pause/handoff state、remote control plane snapshot/output chunks/artifact blob chunks、replay cassette/result/raw payload，以及 task node max turns/timeout。
- 默认测试组织已固定：`tests/README.md` 记录 standalone/core/host/orchestrator/persistence/profile/provider-offline/stress/multi-agent/remote-worker/replay/examples/provider-real 测试组；`.github/workflows/ci.yml` 在 Windows/MSYS2 上跑 `xwork.c` standalone compile、core smoke、host smoke、主 orchestrator smoke、persistence smoke、profile smoke、provider offline smoke、stress smoke、multi-agent smoke、remote-worker smoke、replay smoke 和 examples，真实 provider smoke 通过 repository variable 显式开启且 `continue-on-error`，不阻塞默认 CI。真实 provider smoke 支持成功 tool-loop，也支持 `XWORK_PROVIDER_SMOKE_EXPECT_ERROR=1` 的错误路径断言，用于坏密钥、坏模型或受控错误端点。
- `tests/xwork_stress_smoke.c` 已独立覆盖 terminal long-output capture/events，以及 many-runs/artifacts persistence query + pagination。
- `tests/xwork_multi_agent_smoke.c` 已覆盖 P3 第一阶段 agent pool/task graph：2 个 agent 并发 fan-out、fan-in join、child run 映射、max concurrency、require-all 失败传播、per-agent retry、cooperative cancel、scheduler pause/resume、handoff refs/events/snapshot recovery、agent result/aggregate report artifacts、child-run event audit、agent pool snapshot、可重建 graph snapshot、file persistence 保存/加载、snapshot-to-graph 导入、persistence recovery 后继续执行、状态查询，以及 parent/agent/task run index 查询。
- `tests/xwork_remote_worker_smoke.c` 已覆盖 P3 remote worker control plane：worker register/heartbeat、task policy/approval gate、network policy denial、secret redaction、capability matching、control-plane capability allowlist 拒绝、assignment queue、claim/complete、local worker remote `process.exec`、remote `process.exec` timeout/timeout_stop、destructive command denial、remote terminal start/list/stop、remote filesystem `write_text/read_text/apply_patch` 且继承 filesystem root policy 并覆盖越界 path denial、artifact upload message、blob ref、chunk payload 查询/恢复、stdout/stderr output chunk 上传与恢复、cancel、stale lease 和 orphaned assignment 标记、worker/task query、control plane file snapshot、load/recover，以及恢复后 queued task 继续执行、in-flight assignment 标记为 orphaned。
- `tests/xwork_replay_smoke.c` 已覆盖 P3 deterministic replay cassette baseline：record/load/replay entry、typed filesystem snapshot/ref、normalized JSON hash 兼容、runtime host-service record/replay、raw result payload recovery、event log schema、model stream event helper、terminal interaction event、event sequence replay、checkpoint seek/replay、model/tool hash、strict success、audit divergence、divergence report artifact、side-effect blocking、cancel、manifest/result、entry query、file-persisted replay list/load/result、`.replay` future-version rejection 和 cassette-to-engine recovery。
- `tests/xwork_remote_worker_smoke.c` 已覆盖 P3 remote worker/control-plane baseline：in-process control plane、HTTP decoded-message transport marker、worker register/heartbeat/lease、assignment、policy/approval/network/capability gates、process/filesystem/terminal worker execution、artifact/blob/output chunk recovery、stale lease/orphan、snapshot persistence 和恢复后 queued work continuation。
- `process.exec` 最小 request contract 已支持 `cwd` 覆盖
- `process.exec` 最小 request contract 已支持 `max_output_bytes` 截断
- `process.exec` 最小 request contract 已支持 `env:["KEY=VALUE"]`
- `process.exec` 最小 request contract 已支持 `stdin_text`
- `process.exec` 本地 host 路径已切到 `xrt subprocess`
- `process.exec` 最小 request contract 已支持 `timeout_ms`
- `process.exec` 最小 request contract 已支持 `timeout_stop`（`interrupt` / `terminate` / `kill` / `kill_tree`）
- `process.exec` 最小 request contract 已支持 `allow_nonzero_exit:true`
- `process.exec` 最小 request contract 已支持 `merge_stderr:false`
- `process.exec` 最小 request contract 已支持 `include_events:true`
- `process.exec` 最小 request contract 已支持 `use_terminal:true` 与可选 `terminal_cols/terminal_rows`
- `process.exec` 最小 response contract 已显式返回 `stdout` / `stderr` 与 per-stream truncation flags
- `process.exec` 最小 response contract 已能返回有序 `xrt subprocess` events，带 stream/kind/text/exit metadata
- `process.exec` terminal mode 最小 response contract 已带回 `use_terminal`、terminal size、`terminal_output_captured` 与有序 lifecycle events；terminal 文本捕获仍允许平台差异
- 本地 host 已支持 interactive terminal session：`process.start_terminal` / `process.terminal_read` / `process.terminal_write` / `process.terminal_resize` / `process.terminal_stop`
- `process.terminal_resize` 已显式返回 `resize_applied`，使 terminal resize 可以 best-effort 降级，而不是直接打断整个 session
- interactive terminal session state result 已显式返回 `output_text` / `output_bytes`，并带 `event_end_seq` / `has_more_events` / `event_stream_done`，调用方不必自己拆 event 或推断是否还有后续输出
- `process.terminal_write` 已支持 `include_state:true`、可选 `after_seq` / `max_events`，以及 `write_eof:true` 显式关闭 terminal stdin；单次写入后可直接返回 post-write 增量 terminal state
- interactive terminal session state result 现在也带稳定元数据 `session_index` / `stdin_closed`，调用方不需要再从单次 write 结果反推会话状态
- local host 现在支持带 `session_name` / `running` / `done` / `after_session_index` / `limit` 过滤条件的 `process.list_terminals`，并返回基于 `session_index_asc` 的分页元数据（`has_more_sessions` / `next_after_session_index`）；terminal session 也可以携带稳定的 `session_name`，调用方可以重新发现并分页查询活跃 interactive terminal session，而不需要把所有会话状态都自行缓存到外部
- builtin `process.list_terminals` 现在也会在 orchestrator run 中合成 JSON output artifact（`terminal-sessions://active`），让 terminal inventory 能跟其他 run artifact 一起进入持久化与恢复链路
- builtin `process.terminal_resize` 现在也会在 orchestrator run 中合成 JSON output artifact，让 terminal geometry 变化能和 terminal session 其他状态一起进入 artifact/persistence 链路
- builtin `process.terminal_write` 现在也会在返回 terminal state 时合成 JSON output artifact，让 post-write terminal state window 能和 write command 一起进入 artifact/persistence 链路
- builtin `process.terminal_read` 现在也会在 orchestrator run 中合成 JSON output artifact，让增量 terminal state window 能和 terminal transcript 一起进入 artifact/persistence 链路
- builtin `process.terminal_stop` 现在也会在 orchestrator run 中合成 JSON output artifact，让 terminal session 的最终 stop state 能和 transcript/output 一起进入 artifact/persistence 链路
- terminal session JSON artifact 已固定 schema 标识：state window 使用 `xwork.terminal_state.v1`，inventory 使用 `xwork.terminal_inventory.v1`
- 在 interactive terminal 支持可用时，file persistence smoke 现在也会验证 terminal session 的 `resize/write/read/stop` 这些 JSON artifact 可以被列出并读回；同时公共 persistence API 也补了按 artifact 名称直接查找的最小读面
- persisted artifact 现在也有轻量 summary list 读面（`id/kind/output_class/output_role/name/mime/storage_ref/summary/sequence`，以及存在时的 content/patch stats），调用方可以先检查 terminal JSON artifact 链，再按需加载完整 artifact
- persisted artifact summary 现在也支持最小 metadata query（`kind` / `output_class` / `output_role` / `name_prefix` / `mime` / `storage_ref` / `exit_code` / `sequence`），调用方可以先按条件筛出 terminal JSON artifact，再按需加载完整对象
- artifact summary query 现在也支持精确 `artifact_name`，调用方可以不退回完整 artifact load 就定位单个 persisted terminal JSON artifact
- persisted artifact summary query 现在也支持 `after_sequence + limit`，而 summary list 会返回 `has_more/next_after_sequence`，terminal artifact 链可以按 sequence 稳定翻页
- artifact summary query 现在也支持 `mime_prefix` / `storage_ref_prefix`，调用方可以按 transport metadata 切出同一条 terminal session artifact 链，而不需要完整读取 artifact 内容
- artifact summary query 现在也支持 `output_role_prefix`，并且 terminal JSON artifact 的 durable smoke 会验证 `terminal_state` 分类和对应 role 可恢复
- artifact summary query 现在也支持 `report_class` / `report_subject_ref_prefix`，durable smoke 会验证 report plan artifact 的专用 metadata 可恢复
- artifact summary query 的 durable smoke 现在也覆盖 `exit_code` 和 min/max `sequence` 过滤，确保 command artifact 的 metadata 查询语义可恢复
- builtin `process.exec` artifact bridge 已保留 stderr，不再只落 stdout
- builtin `process.exec` artifact bridge 会在 stderr、非零退出或 build/test-like command 时生成 diagnostics report；成功的 test/build 命令会得到 `diagnostic_count:0` 的结构化摘要
- command artifact 现在也能保留结构化 stdout/stderr byte counts 与 per-stream truncation flags，`process.exec` artifact bridge 已将 host result 中的相关字段映射进 artifact metadata
- `process.exec` 失败路径已保留最小结构化结果，可区分 invalid request / timeout / cancelled / non-zero exit，并带回请求 stop policy 与观察到的 stop reason
- `process.exec` 最小 stdin contract 已会校验 configured `iMaxProcessInputBytes`
- `process.exec` 最小 env contract 已会校验 configured `iMaxProcessEnvEntries`
- persistence recovery 以 latest run snapshot 为边界；approval resolved 会刷新 latest snapshot，因此 pending tool 在批准后即使进程重启也能恢复到可 resume 状态。live OS process handle 和 interactive terminal session 不跨进程恢复，持久化的 terminal/process artifact 只作为审计和输出记录，宿主需要显式重新发现或重启 live session

这一层接下来更重要的是：

- 收紧 request/response contract，而不是继续堆新的 host kind
- 补更真实的 filesystem/process/vcs 操作面和平台差异处理
- 继续硬化 async run 生命周期、并发观察与 richer artifacts 边界

### M3: `xllm` 编排最小闭环

目标：

- 引入 `xllm` 依赖
- 支持最小 model turn orchestration
- 支持最小 tool result 回灌

### M4: policy 与 approval

目标：

- 副作用分级
- autonomy mode
- approval request 对象

### M5: checkpoint 与恢复

目标：

- 最小 event log
- 最小 checkpoint snapshot
- run resume

### M6: workspace + memory 接入

目标：

- 将 workspace 与 `xllm_memory` 编排打通
- 固定 memory search 注入 request 的策略入口
- 提供 workspace root 到 `xllm_memory` 的最小 sync helper
- 提供单文件变更到 `xllm_memory` 的最小 sync helper

### M7: `xcode` / `xclaw` profile

目标：

- 提供两套默认 profile
- 只做策略组合，不做产品 UI

## 8. v1 必做范围

- runtime / workspace / run / tool 基础对象
- host service 基础抽象
- tool registry
- basic policy / approval
- `xllm` integration
- checkpoint / resume 最小闭环
- event log 最小闭环
- `xcode` / `xclaw` profile 最小版本

## 9. v1 延后范围

- 真正的多 agent 并行调度
- 复杂 replay 平台
- 远程 worker
- 云端控制平面
- 完整 editor buffer 同步
- 分布式 artifact store

## 10. 测试策略

建议测试分为四层：

- 单元测试
- mock runtime 测试
- `xllm` 集成 smoke
- 产品接入 smoke

## 11. 当前 v1 基线与后续方向

当前 `AGENT_RUNTIME_TRACKING_SPEC.md` 中的 P0/P1/P2 任务已经收敛为可验证基线：

- public API、ownership/lifetime、错误码、版本字符串和 persistence format version 已有明确 contract。
- builtin host tool 面已覆盖 filesystem/process/terminal/vcs/editor/diagnostics 的 agent 常用路径。
- policy/approval、checkpoint/recovery、artifact schema、workspace memory、xllm provider/tool-loop、product profile/example、CI smoke matrix 已形成默认可用路径。
- P3 multi-agent 已具备 in-process agent pool/task graph、并发 fan-out/fan-in、cancel/pause/retry、snapshot/recovery、handoff request/result schema、artifact refs、memory context refs、shared workspace refs、handoff audit events、handoff 持久化恢复，以及 agent result / aggregate report artifact schema。
- core/host/orchestrator/persistence/profile/provider/stress/examples 都有独立或聚合 smoke，可作为后续重构的安全网。

下一阶段不应继续无边界扩张 v1 API，优先做 hardening：

1. 把主 orchestrator 聚合 smoke 逐步拆薄为更小的主题文件，保留端到端聚合入口。
2. 为 host tool JSON request/response 持续维护独立 contract 文档和示例 corpus；当前 contract 文档已落在 `docs/HOST_TOOL_CONTRACTS.md`，示例 corpus 已落在 `docs/HOST_TOOL_EXAMPLES.md`。
3. 为 persistence format v6 持续维护 schema 文档和迁移测试 fixture；当前 format 文档已落在 `docs/PERSISTENCE_FORMAT.md`，focused newer-version fixture 已落在 `tests/xwork_persistence_smoke.c`。
4. 用真实 provider 环境定期跑成功/错误两条可选 smoke，并记录 provider 差异；运行说明和差异记录模板已落在 `docs/PROVIDER_SMOKE.md`。
5. 在 release 前做一次 `xwork.h` ABI/source compatibility review，冻结 0.1.0 对外快照；冻结检查清单已落在 `docs/API_FREEZE_0_1.md`。
6. P3 下一阶段完整能力单独跟踪：多 agent 并行调度、remote worker/control plane、deterministic replay platform；跟踪文件为 `P3_FUTURE_BOUNDARY_TRACKING_SPEC.md`。
