# xwork P3 完整能力规格与进度跟踪

本文件用于跟踪 `xwork` 下一阶段 P3 能力开发。P3 不再只是 future boundary；新的阶段目标是完整实现：

- 多 agent 并行调度。
- remote worker / control plane。
- deterministic replay platform。

状态说明：

- `[ ]` 未开始
- `[~]` 进行中
- `[x]` 已完成

阶段原则：

- P3 可以新增 public API，但必须先固定 ownership、lifetime、thread-safety、persistence format 和 error contract。
- P3 能力必须保持与现有 runtime / run / tool / policy / approval / checkpoint / artifact / persistence / xllm 集成兼容。
- 每个能力必须有最小 smoke、持久化恢复路径、文档和示例。
- 网络、远程执行和 replay 都必须保留安全边界：policy、approval、workspace path policy、network policy、secret redaction。

## 0. 当前 P3 基线

- [x] `xwork` 已具备单 run / 单 orchestrator loop / async handle / host tool / persistence / checkpoint / artifact 基础能力。
- [x] `xwork` 已具备 `xclaw` 单 agent autonomous baseline。
- [x] `xwork` 已具备 file persistence 和 run/event/checkpoint/artifact query surface。
- [x] `xwork` 已具备 local host tool bridge，可作为 remote worker bridge 的本地基线。
- [x] `xwork` 已具备 checkpoint/recovery，并已有 deterministic replay cassette baseline。

## 1. P3 Multi-Agent Parallel Scheduling

目标：实现 `xwork` 内建多 agent 并行调度能力，让 AI IDE / claw 能创建 agent graph、并发运行子 agent、汇总结果，并保持可审计、可取消、可恢复。

### 1.1 Public API And Object Model

- [x] 定义 `xwork_agent` / `xwork_agent_options` 公共对象。
- [x] 定义 `xwork_agent_role` 或 role string 约定：planner、coder、reviewer、tester、researcher、custom。
- [x] 定义 `xwork_agent_pool` / `xwork_agent_pool_options`。
- [x] 定义 `xwork_task_graph` / `xwork_task_node` / `xwork_task_edge`。
- [x] 定义 `xwork_agent_run` 或明确 agent 与现有 `xwork_run` 的映射关系。
- [x] 定义 parent/child run、agent id、task id、handoff id 的稳定字段（parent/agent/task 已进入 run options/summary/snapshot/index；handoff id 已进入 handoff schema）。
- [x] 定义 public init/reset/destroy/query API。
- [x] 写清楚对象 ownership、borrow/copy 规则、线程安全边界（见 `docs/MULTI_AGENT.md`）。

### 1.2 Scheduler Semantics

- [x] 实现最小 task graph scheduler。
- [x] 支持并行 fan-out。
- [x] 支持 fan-in join。
- [x] 支持依赖失败传播策略：fail-fast、best-effort、require-all。
- [x] 支持 max concurrency。
- [x] 支持 per-agent max turns / timeout / retry（max turns/timeout 已进入 task summary/snapshot；默认 task executor 将 max turns 映射到 orchestrator budget，并用 async wait/cancel 执行 timeout）。
- [x] 支持全局 cancel token 和 per-agent cancel。
- [x] 支持 agent run status 查询。
- [x] 支持 scheduler pause/resume。
- [x] 支持 scheduler execution guard，避免同一 graph 重入执行。

### 1.3 Agent Handoff And Shared Context

- [x] 定义 handoff request schema。
- [x] 定义 handoff result schema。
- [x] 支持 agent 间传递 artifact refs。
- [x] 支持 agent 间传递 memory context refs。
- [x] 支持 shared workspace。
- [x] 支持只读 shared context 和可写 workspace policy 区分。
- [x] 支持 planner 生成 task graph 的导入入口（已支持从 task graph snapshot 重建 graph）。
- [x] 支持最终 aggregate report artifact。

### 1.4 Events, Artifacts, Checkpoints

- [x] 新增 agent/task/scheduler event kinds。
- [x] 新增 agent spawned / started / paused / completed / failed / cancelled events。
- [x] 新增 task scheduled / started / joined / blocked / unblocked events（blocked/unblocked 已通过依赖等待任务的 child run 事件记录）。
- [x] 新增 handoff requested / accepted / rejected / completed events。
- [x] 新增 multi-agent checkpoint kind。
- [x] 新增 task graph snapshot（包含 node profile/session/workspace/dependency/state/handoff，可用于重建 graph）。
- [x] 新增 agent result report artifact schema。
- [x] 新增 aggregate report artifact schema。

### 1.5 Persistence And Recovery

- [x] 扩展 file persistence 保存 agent pool / task graph snapshot。
- [x] 扩展 run index 支持 parent/child、agent id、task id 查询。
- [x] 支持恢复 waiting / running / completed task graph（pending/completed/pause/handoff snapshot 恢复已实现；live in-flight worker/thread 仍按定义降级为 pending 重跑）。
- [x] 定义恢复后 live in-flight agent 的边界（READY/RUNNING/BLOCKED 恢复为 PENDING，重新执行）。
- [x] 支持恢复后继续 pending tasks。
- [x] 支持恢复后重新 join completed child runs。
- [x] 升级 persistence format version（v14 保存 task node max turns/timeout；v13 保存 replay raw payload；v12 保存 remote artifact blob chunks；v11 保存 remote stdout/stderr output chunks；v10 保存 task graph handoff；v9 保存 replay；v8 保存 remote error/protocol；v7 保存 remote artifacts；v6 保存 remote plane；v5 保存 task graph pause state；v4 保存 run-level agent/task id）。
- [x] 增加旧格式兼容读或明确拒绝策略（v3/v4 snapshot 兼容读取，新增字段为空；更新版本拒绝）。

### 1.6 Tests And Examples

- [x] 新增 `tests/xwork_multi_agent_smoke.c`。
- [x] smoke 覆盖 2 个 agent 并发执行。
- [x] smoke 覆盖 fan-out / fan-in。
- [x] smoke 覆盖子 agent 失败传播。
- [x] smoke 覆盖 cancel。
- [x] smoke 覆盖 checkpoint/recovery（task graph snapshot、handoff snapshot、graph 导入、file persistence 保存/加载、恢复后继续执行已覆盖）。
- [x] smoke 覆盖 persistence query（task graph snapshot load、agent/task/parent run index query 已覆盖）。
- [x] 新增 `examples/multi_agent_claw.c`。
- [x] 更新 CI matrix。

完成标准：

- [x] 可通过 public API 创建多个 agent 并行执行。
- [x] 可查询每个 agent/task 的状态、事件、artifact。
- [x] 可取消、恢复、审计多 agent graph。
- [x] 有 smoke、example、docs 和 CI 覆盖。

## 2. P3 Remote Worker / Control Plane

目标：实现可本地运行的 remote worker / control plane 基础设施，使 `xwork` 可以把 run/task/tool execution 分发到远程 worker，并可管理 worker 生命周期。

### 2.1 Public API And Object Model

- [x] 定义 `xwork_control_plane` / `xwork_control_plane_options`。
- [x] 定义 `xwork_worker` / `xwork_worker_options`。
- [x] 定义 `xwork_worker_id`、capability、label、lease、heartbeat。
- [x] 定义 `xwork_remote_task` / assignment / result。
- [x] 定义 control plane init/start/stop/destroy API。
- [x] 定义 worker register/unregister/heartbeat API。
- [x] 定义 task assign/claim/complete/fail API。
- [x] 定义 query API：workers、leases、assignments、queues。
- [x] 写清楚 ownership、thread-safety、shutdown contract（`xwork.h` public comment + `docs/REMOTE_WORKER.md` 已覆盖 copy/reset/destroy、borrowed runtime/worker、外部串行化、stop/destroy、live process/terminal 不恢复等边界）。

### 2.2 Transport And Protocol

- [x] 选择 v1 transport：local in-process transport + HTTP decoded-message transport（control plane 接受并持久化 `XWORK_REMOTE_TRANSPORT_HTTP_BOUNDARY`；HTTP socket/auth/retry/blob streaming 由宿主层实现，decoded JSON message 进入同一 control-plane API；`docs/REMOTE_WORKER.md` 固定 endpoint 和 wire JSON schema）。
- [x] 定义 worker protocol message schema（public C struct + `docs/REMOTE_WORKER.md` wire JSON schema 已覆盖 register/heartbeat/assignment/result/artifact/output/error envelope）。
- [x] 定义 register message。
- [x] 定义 heartbeat message。
- [x] 定义 assignment message。
- [x] 定义 result upload message。
- [x] 定义 artifact upload message（`xwork_remote_artifact_upload` + `xwork_control_plane_upload_artifact`，覆盖 artifact summary/ref、blob ref、content hash、chunk 元数据和 chunk payload）。
- [x] 定义 error message 和 retryable/non-retryable 分类（public result/summary/snapshot 暴露 `sErrorKind`、`sErrorMessage`、`bRetryable`）。
- [x] 支持 protocol version（`XWORK_REMOTE_PROTOCOL_VERSION_CURRENT`，plane/worker/task/assignment/result/snapshot 均携带版本并拒绝未知版本）。
- [x] 支持 capability negotiation。

### 2.3 Worker Execution Model

- [x] 支持 remote `process.exec`。
- [x] 支持 remote filesystem read/write/apply_patch 受 workspace policy 限制。
- [x] 支持 remote tool execution callback。
- [x] 支持 remote terminal 的明确边界或最小实现（in-process worker 可通过 capability allowlist 执行 `process.start_terminal` / `process.list_terminals` / `process.terminal_stop`，live terminal session 仍不跨进程恢复）。
- [x] 支持 timeout / cancel / kill semantics（cancel queued/assigned task、remote `process.exec` timeout 和 `timeout_stop` 结果记录已覆盖）。
- [x] 支持 stdout/stderr streaming 或 chunked capture（`xwork_remote_output_chunk` + `xwork_control_plane_upload_output_chunk`，summary/snapshot/persistence 可查询恢复；HTTP decoded transport 由宿主层承载 wire streaming/retry/auth 后进入同一 control-plane API）。
- [x] 支持 artifact 回传（result-attached artifact summary/ref、blob/chunk upload message、in-process plane-owned blob store、query API 和 persistence recovery 已完成；HTTP transport streaming 仍由 transport 边界承载）。
- [x] 支持 diagnostics 回传（diagnostics report summary/ref 可随 remote task result 回传并恢复）。
- [x] 支持 worker crash / disconnect / stale lease 处理。

### 2.4 Security And Policy

- [x] remote task 必须经过 policy/approval。
- [x] 支持 worker capability allowlist。
- [x] 支持 workspace root enforcement。
- [x] 支持 network policy enforcement。
- [x] 支持 destructive command risk escalation。
- [x] 支持 secret redaction。
- [x] 定义 worker auth 边界。
- [x] 定义 tenant/project 边界，如果 v1 不做多租户则明确单租户限制。

Worker auth boundary:

- 当前 in-process transport 不提供网络认证；调用方持有 `xwork_control_plane *` 即视为可信控制面调用者。
- `xwork_worker_options::sWorkerId` 是 control-plane 内唯一身份标识，重复注册返回 `XWORK_ERROR_ALREADY_EXISTS`。
- worker 能力不由 worker 自证即可越权；control plane 可通过 `psAllowedCapabilities` + `bEnforceCapabilityAllowlist` 限制 worker 注册能力和 task required capability。
- 生产 remote transport / cloud control plane 必须在 transport 层补充 worker token、mTLS 或等价认证，并在进入 `xwork_control_plane_register_worker()` 前完成身份校验；v1 API 不持久化 secret。

Tenant/project boundary:

- 当前 control plane 是单租户、单 project 边界；`sPlaneId` 是隔离单元。
- 不支持跨 tenant worker pool、跨 project queue 或 cloud control-plane 多租户调度。
- 调用方需要为不同项目创建独立 `xwork_control_plane`、独立 persistence root 或至少独立 `sPlaneId`。
- persistence snapshot 不包含 tenant secret；恢复时由调用方重新提供 runtime/policy/options。

### 2.5 Persistence And Recovery

- [x] 持久化 worker registry。
- [x] 持久化 worker heartbeat / lease。
- [x] 持久化 assignment queue。
- [x] 持久化 remote task result。
- [x] 持久化 remote artifacts（artifact summary/ref 和 blob chunk payload 已随 `.plane` snapshot 持久化并可恢复查询）。
- [x] recovery 后能识别 stale assignment。
- [x] recovery 后能重试或标记 orphaned task。
- [x] 升级 persistence format version。

### 2.6 Tests And Examples

- [x] 新增 `tests/xwork_remote_worker_smoke.c`。
- [x] smoke 覆盖 worker register/heartbeat。
- [x] smoke 覆盖 assignment claim/complete。
- [x] smoke 覆盖 remote process.exec。
- [x] smoke 覆盖 remote terminal start/list/stop 最小闭环。
- [x] smoke 覆盖 remote filesystem read/write/apply_patch 和 root policy enforcement。
- [x] smoke 覆盖 cancel/timeout。
- [x] smoke 覆盖 artifact upload message、blob ref、chunk payload、恢复后的 artifact summary/ref 和 blob chunk 查询。
- [x] smoke 覆盖 stdout/stderr output chunk upload 和恢复。
- [x] smoke 覆盖 worker disconnect/stale lease。
- [x] smoke 覆盖 persistence recovery。
- [x] 新增 `examples/remote_worker_agent.c`。
- [x] 更新 CI matrix。

完成标准：

- [x] control plane 可以管理至少一个 worker。
- [x] worker 可以执行远程 task/tool 并回传 result/artifact（process/filesystem result、artifact/diagnostics summary refs、artifact upload message、blob chunk store/query/recovery 已完成；HTTP transport streaming 仍是边界）。
- [x] disconnect/recovery 有明确行为（stale/orphan、持久化 control-plane recovery、恢复后 queued task 继续执行已完成）。
- [x] 有 smoke、example、docs 和 CI 覆盖。

## 3. P3 Deterministic Replay Platform

目标：实现可重复执行和可审计的 deterministic replay 平台，使历史 run 可以在记录的 model/tool/filesystem 输入下重放，并报告 divergence。

### 3.1 Public API And Object Model

- [x] 定义 `xwork_replay_engine` / `xwork_replay_options`。
- [x] 定义 `xwork_replay_manifest`。
- [x] 定义 `xwork_replay_result`。
- [x] 定义 `xwork_replay_divergence`。
- [x] 定义 model cassette API（通过 generic replay entry record/load/replay 支持 model request/response hash）。
- [x] 定义 tool result cassette API（通过 generic replay entry record/load/replay 支持 tool arguments/result hash）。
- [x] 定义 filesystem snapshot/ref API（`xwork_replay_filesystem_ref_*` typed API 已支持 ref id、path、metadata JSON、content hash、record/load/replay/list；持久化复用 v13+ replay raw payload 字段，无需新增文件格式）。
- [x] 定义 replay init/run/cancel/destroy API。
- [x] 写清楚 replay ownership、thread-safety、determinism guarantee 边界（API copy/reset、runtime borrowed replay engine、summary/list ownership、外部串行化和 host replay 输出生命周期已写入 `xwork.h` / `docs/REPLAY.md`）。

### 3.2 Recording

- [x] 记录 model request。
- [x] 记录 model response。
- [x] 记录 streaming model events（`xwork_replay_event_options_from_model_event` + event log record/replay）。
- [x] 记录 tool call。
- [x] 记录 tool result。
- [x] 记录 host tool side effects（runtime host service invoke 在 record mode 自动记录 host/filesystem/process entry，side-effect blocking 会在调用真实 host 前返回 `XWORK_ERROR_PAUSED`）。
- [x] 记录 filesystem read/write metadata（filesystem host service 自动记录 request arguments、result payload/hash 和 status；更细 filesystem snapshot/ref 文件格式仍由 3.1 单项跟踪）。
- [x] 记录 process command/stdout/stderr/exit/timing policy（process host service 自动记录 request arguments、result payload/hash 和 status；stdout/stderr 已由 process result/remote output chunk 承载）。
- [x] 记录 terminal interaction 或明确 terminal replay 限制（event-log hash comparison；live terminal 不重建，见 `docs/REPLAY.md`）。
- [x] 记录 artifact content hash。
- [x] 记录 checkpoint hash。

### 3.3 Replay Execution

- [x] 支持 replay model response cassette。
- [x] 支持 replay tool result cassette。
- [x] 支持 replay host tool output（runtime host service invoke 在 strict/audit mode 从 replay cassette 返回记录的 `sResultJson`，不调用真实 host service）。
- [x] 支持 readonly replay mode。
- [x] 支持 side-effect-blocking replay mode。
- [x] 支持 strict mode：任何 mismatch 失败。
- [x] 支持 audit mode：允许 mismatch 但生成 divergence report。
- [x] 支持从 checkpoint 开始 replay（`xwork_replay_engine_seek_checkpoint` 定位 checkpoint entry 后续）。
- [x] 支持 replay cancel。
- [x] 支持 replay summary。

### 3.4 Divergence Detection

- [x] 比较 event sequence（`xwork_replay_engine_replay_event` 按事件游标比较 kind/key/name/type/hash/status）。
- [x] 比较 model request normalized hash（JSON payload 使用 `xwork_replay_hash_json()` normalized hash；旧 text-hash cassette 仍兼容）。
- [x] 比较 model response normalized hash（JSON payload 使用 `xwork_replay_hash_json()` normalized hash；旧 text-hash cassette 仍兼容）。
- [x] 比较 tool call id/name/arguments。
- [x] 比较 tool result status/output hash。
- [x] 比较 artifact metadata/content hash。
- [x] 比较 checkpoint snapshot hash。
- [x] 输出 divergence report artifact（`xwork_replay_engine_emit_report_artifact` 输出 typed report artifact）。
- [x] 支持 first-divergence query。

### 3.5 Persistence Format

- [x] 定义 replay manifest 文件布局（v9 `.replay` binary file）。
- [x] 定义 cassette 文件布局（v13 `.replay` entry raw payload + hash list；v9-v12 hash-only cassette 兼容读取）。
- [x] 定义 content hash algorithm（v1 `fnv1a64` text hash）。
- [x] 定义 replay event log schema（`xwork_replay_event_options` / `xwork_replay_event_summary`，见 `docs/REPLAY.md`）。
- [x] 扩展 file persistence query API（store/list/load manifest/entries/result/engine）。
- [x] 支持 replay manifests list/load。
- [x] 支持 replay results list/load。
- [x] 升级 persistence format version（v13 replay raw payload；v9 replay manifest/cassette/result）。
- [x] 增加 migration/future-version fixture（`.replay` newer format manifest load 拒绝）。

### 3.6 Tests And Examples

- [x] 新增 `tests/xwork_replay_smoke.c`。
- [x] smoke 覆盖 model cassette replay。
- [x] smoke 覆盖 tool result replay。
- [x] smoke 覆盖 strict mode success。
- [x] smoke 覆盖 divergence detection。
- [x] smoke 覆盖 divergence report artifact。
- [x] smoke 覆盖 checkpoint replay。
- [x] smoke 覆盖 replay event log schema/model stream/terminal/event sequence。
- [x] smoke 覆盖 replay persistence query 和 future-version rejection。
- [x] smoke 覆盖 runtime host-service record/replay，并验证恢复后的 replay 不访问真实 host。
- [x] 新增 `examples/replay_agent_run.c`。
- [x] 更新 CI matrix。

完成标准：

- [x] 记录一次 run 后可以在不访问真实 model/provider/host 的情况下 replay（model/tool cassette 已支持，host service 严格回放已覆盖；完整 orchestrator 自动录制 model provider 仍由调用方接 `xwork_replay_engine_record_entry`）。
- [x] replay 能生成成功结果或 divergence report。
- [x] replay 数据可持久化、查询、恢复。
- [x] 有 smoke、example、docs 和 CI 覆盖。

## 4. Cross-Cutting Requirements

- [x] 更新 `xwork.h` public API，并补完整 ownership/lifetime/thread-safety 注释。
- [x] 更新 `src/xwork_core/xwork_internal.h` 内部结构。
- [x] 更新 `xwork.c` 聚合入口。
- [x] 更新 persistence backend interface。
- [x] 更新 file persistence format version 和文档（v14 保存 task node max turns/timeout；v13 保存 replay raw payload；v12 保存 remote artifact blob chunks；v11 保存 remote stdout/stderr output chunks；v10 保存 task graph handoff；v9 保存 replay manifest/cassette/result；v8 保存 remote protocol version 与 task error classification；v7 保存 remote task artifact summary refs）。
- [x] 更新 status/error mapping。
- [x] 更新 policy/approval 文档（见 `docs/POLICY_APPROVAL.md`）。
- [x] 更新 host tool contract，如 remote/replay 影响 host tool 语义（runtime replay、side-effect blocking、terminal replay 边界已写入 `docs/HOST_TOOL_CONTRACTS.md` / `docs/REPLAY.md`）。
- [x] 更新 examples README。
- [x] 更新 tests README。
- [x] 更新 CI matrix。
- [x] 更新 CHANGELOG。

## 5. 建议开发顺序

1. [x] 先做 public API 设计草案：multi-agent、remote worker、replay 三组对象和错误码边界。
2. [x] 实现 multi-agent 最小 scheduler：in-process fan-out/fan-in + cancel + smoke。
3. [x] 实现 multi-agent persistence/recovery。
4. [x] 实现 remote worker in-process control plane transport。
5. [x] 实现 remote worker persistence/recovery。
6. [x] 实现 deterministic recording manifest/cassette。
7. [x] 实现 deterministic replay strict/audit modes。
8. [x] 补 full smoke、examples、docs、CI（multi-agent/remote worker/replay smoke、example 和 CI 已补；replay raw-payload/host replay 已补 smoke/docs）。

## 6. 验证矩阵

- [x] `gcc -std=c11 -Wall -Wextra -pedantic -c xwork.c`
- [x] `tests/xwork_multi_agent_smoke.c`
- [x] `tests/xwork_remote_worker_smoke.c`
- [x] `tests/xwork_replay_smoke.c`
- [x] 现有 core/host/orchestrator/persistence/profile/provider/stress smoke 不回归。
- [x] examples 全部通过（AI IDE、claw、multi-agent claw、remote worker、replay 已纳入）。

## 7. 更新规则

- 每完成一个 P3 子任务，必须同步更新本文件。
- 新增 public API 前必须先写清楚 compatibility 和 persistence 影响。
- 新增持久化字段必须 bump format version 或写清楚兼容读策略。
- 每新增 event/artifact schema，必须补 docs、persistence、query 和 smoke。
- remote worker 和 replay 涉及外部副作用，默认必须走 policy/approval。
