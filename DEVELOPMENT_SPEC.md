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
  - typed patch/report/command/output artifact emit helper
  - in-memory persistence 与 file persistence backend
  - persisted run/event/checkpoint/artifact 查询面
  - `xcode` / `xclaw` 内建 profile
- 内部模块目录：
  - [src/xwork_core](/D:/git/xwork/src/xwork_core)
  - [src/xwork_workspace](/D:/git/xwork/src/xwork_workspace)
  - [src/xwork_tools](/D:/git/xwork/src/xwork_tools)
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

### 4.3 代码组织约束

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
- patch artifact 已补最小结构化统计：`file_count / hunk_count / added_line_count / deleted_line_count`，并贯穿 runtime 对象、summary、snapshot、file persistence 与 workspace memory ingest
- report/output artifact 已补最小 typed output metadata：`output_class / output_role`，并贯穿 runtime 对象、summary、snapshot、file persistence、summary query 与 workspace memory ingest；builtin filesystem/terminal bridge 会填充 file-content/file-change/terminal-state/terminal-inventory 分类
- report artifact 已补专用 typed metadata：`report_class / report_subject_ref`，并贯穿 runtime 对象、summary、snapshot、file persistence、summary query 与 workspace memory ingest

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
- builtin host tools 已有 `filesystem.read_text` / `filesystem.write_text` / `process.exec` / `process.start_terminal` / `process.terminal_read` / `process.terminal_write` / `process.terminal_resize` / `process.terminal_stop` / `vcs.status`
- builtin host tool 在 orchestrator 中已能自动落最小 output/command artifact
- builtin terminal host tool 在 orchestrator 中已能自动落最小 session command/output artifact（start/write/stop）
- `filesystem.write_text` 最小 request contract 已支持 `mode=append`
- `filesystem.write_text` 最小 request contract 已支持 `mode=create`
- `filesystem.write_text` 最小 request contract 已支持 `create_dirs:true`
- `filesystem.read_text` 最小 request contract 已支持 `offset_bytes`
- `filesystem.read_text` 最小 response contract 已显式返回 `file_size_bytes` / `remaining_bytes` / `eof` / `next_offset_bytes`
- `filesystem.read_text/write_text` 失败路径已保留最小结构化结果，可区分 not_found / already_exists / parent_not_found
- orchestrator 已能把 `xllm` stream preference、model event callback、xllm cancel token 和协作式 interrupt check 接入同步 model-turn/tool-loop；取消会落 cancelled checkpoint 和 `RUN_CANCELLED` event
- host service invoke 已有 per-call context，可把 run/cancel/interrupt 元数据传入 builtin host；local `process.exec` 已能在 spawn 前和 subprocess polling wait 中协作取消
- 自定义 tool executor 已有 `pfnToolExecEx` 扩展回调、`xwork_tool_exec_context` 与 `xwork_tool_exec_context_should_cancel()` helper，可自行轮询取消并让 orchestrator 收口为 cancelled run
- orchestrator 已提供最小 async run handle：`xwork_run_execute_async` / `xwork_run_async_wait` / `xwork_run_async_wait_timeout` / `xwork_run_async_get_status` / `xwork_run_async_cancel` / `xwork_run_async_destroy`，内部复用同步 loop，并通过 owned 或外部 `xllm_cancel_token` 接入协作式取消；smoke 已覆盖 mock tool、未完成 handle destroy 与 local `process.exec` subprocess 取消
- async run 公共 API 已写明 run/options/callback user data 的浅拷贝生命周期约束，以及 wait timeout 与未完成状态读取语义
- run execution 已有 per-run execution guard，同一个 run 的并发 `xwork_run_execute` 入口会返回 `XWORK_ERROR_INVALID_STATE`
- provider smoke 主执行路径已切到 async run handle，保留 local stub provider matrix 验证 request/response normalization，并补了离线 model-call failure 路径
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
- command artifact 现在也能保留结构化 stdout/stderr byte counts 与 per-stream truncation flags，`process.exec` artifact bridge 已将 host result 中的相关字段映射进 artifact metadata
- `process.exec` 失败路径已保留最小结构化结果，可区分 invalid request / timeout / cancelled / non-zero exit，并带回请求 stop policy 与观察到的 stop reason
- `process.exec` 最小 stdin contract 已会校验 configured `iMaxProcessInputBytes`
- `process.exec` 最小 env contract 已会校验 configured `iMaxProcessEnvEntries`

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

## 11. 近期建议

最合理的下一步顺序：

1. 扩 builtin host tool 面，并继续硬化 filesystem/process/vcs request contract
2. 继续硬化 persistence format、versioning 和 direct query surface
3. 扩大真实 `xllm` smoke 覆盖，保留离线 stub 基线并增加 provider matrix
4. 硬化 async run 的 handle lifetime、并发观察和取消覆盖
5. 扩大真实 `xllm` provider smoke，并把 provider 错误路径矩阵扩展到离线 HTTP failure 基线之外

当前阶段最重要的不是再发明更多名词，而是把已经存在的 runtime 主线做稳、做可测、做可恢复。
