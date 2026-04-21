# xwork Agent Runtime 规格与进度跟踪

本文件用于跟踪 `xwork` 作为 agent 开发通用库的实现进度。后续开发时，请随着设计、实现、测试和文档完善同步更新勾选状态。

核心目标：

- `xwork` 是 `xrt` / `xllm` 之上的 agent runtime 基础设施。
- `xwork` 向上支撑 AI IDE 和 `claw`，但不包含具体产品 UI。
- `xwork` 负责封装 agent 开发通用能力：workspace、run、tool、host、policy、approval、checkpoint、artifact、persistence、async/cancel。
- `xwork` 不重新实现 provider adapter、request/response normalization、session compaction、memory retrieval engine，这些保持在 `xllm`。

状态说明：

- `[ ]` 未开始
- `[~]` 进行中
- `[x]` 已完成

优先级说明：

- `P0` v1 必须完成，否则不应作为通用库对外依赖。
- `P1` v1 应完成，直接影响 AI IDE / claw 可用性。
- `P2` v1 可延后，但需要保留接口边界。
- `P3` 后续版本范围。

## 0. 当前可用基线

- [x] 实现 runtime / workspace / tool registry / run lifecycle。
- [x] 实现 event / approval request / checkpoint / artifact 公共对象。
- [x] 跑通 `xllm` model-turn + tool-loop 最小编排闭环。
- [x] 接入 stream preference / model event callback / cancel token / interrupt check。
- [x] 实现 async run handle：wait / timed wait / status / cancel / destroy。
- [x] 实现 per-run execution guard，避免同一 run 并发 execute。
- [x] 实现 local host：filesystem read/write、process exec、interactive terminal、vcs status。
- [x] 实现 `process.exec` cwd/env/stdin/timeout/stop policy/nonzero/stderr/events/truncation/terminal mode/cancel。
- [x] 实现 artifact：patch/report/command/output，content/patch/command stats，typed output/report metadata。
- [x] 实现 in-memory/file persistence，run/event/checkpoint/artifact query surface。
- [x] 实现 workspace memory sync / single-file sync / tool+artifact ingest。
- [x] 实现内建 `xcode` / `xclaw` profiles。
- [x] 实现 offline provider smoke 和 provider failure smoke。

## 1. P0 Public API Contract

- [x] 固化 `runtime/workspace/run/async/artifact/persistence/local_host` 的 ownership / lifetime / thread-safety 规则。
- [x] 在 `xwork.h` 为核心对象和 async API 补完整 ownership 注释。
- [x] 用 smoke 覆盖 async lifetime、destroy-cancel、并发 execute 拒绝。
- [x] 文档化所有 public API 的返回码行为。
- [ ] 收紧 `INVALID_ARGUMENT / INVALID_STATE / NOT_FOUND / ALREADY_EXISTS / UNSUPPORTED / EXTERNAL_FAILURE / CANCELLED` 的使用边界。
- [x] 形成错误码使用表。
- [ ] 用 smoke 覆盖核心错误码路径。
- [ ] 稳定 provider / host / persistence 错误到 xwork status 的映射。
- [ ] 对 `xwork.h` 做一次向后兼容前的 public API 命名和结构审查。
- [ ] 对明显临时命名完成重命名或文档化。

## 2. P0 Host Tool Surface

- [ ] 实现 `filesystem.list`，支持 root/cwd、相对路径、隐藏文件、目录分页或数量限制。
- [ ] 实现 `filesystem.stat`，返回类型、大小、mtime、路径等结构化 JSON。
- [ ] 实现 `filesystem.glob`，支持 workspace 内安全搜索和数量限制。
- [ ] 覆盖 filesystem list/stat/glob 的不存在、越界、权限、路径非法场景。
- [ ] 实现 `filesystem.apply_patch` 或 `workspace.apply_patch`。
- [ ] apply patch 支持 dry-run。
- [ ] apply patch 支持冲突检测和结构化错误。
- [ ] apply patch 自动生成 patch/file-change artifact。
- [ ] apply patch 受 policy/approval 管控。
- [ ] 实现 `filesystem.mkdir`，支持递归和已存在处理。
- [ ] 实现 `filesystem.move`，支持 overwrite 策略。
- [ ] 实现 `filesystem.delete`，支持文件/目录、递归开关、dry-run。
- [ ] destructive filesystem 操作必须进入 approval/policy。
- [ ] 实现 `vcs.diff`，支持 working tree 和 staged diff。
- [ ] 实现 `vcs.log`，支持 limit。
- [ ] 实现 `vcs.branch`，返回当前分支和 dirty 状态。
- [ ] vcs 工具输出可生成 command/report artifact。
- [ ] 定义 diagnostics host service 边界。
- [ ] 支持从 process output 转换 diagnostics artifact。
- [ ] 支持 test/build command result 的结构化摘要。
- [ ] 定义 editor buffer bridge contract：open buffer、dirty buffer、selection/range、apply edit。
- [ ] buffer edit 受 policy/approval 管控。
- [ ] buffer artifact 能进入 persistence。

## 3. P0 Policy And Approval

- [ ] 实现路径策略，支持 workspace root、allowlist、denylist。
- [ ] read/write/delete/patch 都经过路径策略。
- [ ] 越界路径返回稳定错误和可审计 reason。
- [ ] smoke 覆盖 workspace 外路径。
- [ ] 实现命令策略，覆盖 `process.exec` 和 terminal session。
- [ ] 支持 command allowlist/denylist。
- [ ] 支持 destructive command risk level。
- [ ] 高风险命令触发 approval。
- [ ] 定义 network side-effect 类型。
- [ ] 支持 network host allowlist/denylist contract。
- [ ] 默认 profile 区分 IDE 交互模式和 autonomous 模式。
- [ ] approval reason、scope、action summary、risk level 全链路持久化。
- [ ] 明确 rejected/cancelled 后 run 行为。
- [ ] smoke 覆盖 approval 恢复后继续执行。

## 4. P0 Orchestrator And Agent Loop

- [ ] 定义显式 run step public object 或内部可查询模型。
- [ ] model turn、tool call、approval、checkpoint 都能映射到 step。
- [ ] persistence 可查询 step history。
- [ ] 实现 provider/timeout/transient tool failure 的 retry/backoff 策略。
- [ ] 区分 retryable/non-retryable。
- [ ] 支持 max retries 和 backoff。
- [ ] retry 进入 event/checkpoint。
- [ ] 定义 planner boundary，不在 v1 实现完整 autonomous planner。
- [ ] 定义 plan/report artifact schema。
- [ ] orchestrator 可接受 planner 生成的下一步提示或 tool policy。
- [ ] `xclaw` profile 能启用 planner boundary。
- [~] 硬化 streaming event contract。
- [x] 接入 stream preference 和 model event bridge。
- [ ] 对 token/tool-call/progress/error/cancel event 做完整 smoke。
- [ ] 文档说明 callback 返回 false 与 cancel token 的优先级。
- [~] 硬化 async run。
- [x] wait/status/cancel/destroy smoke 覆盖 mock tool。
- [x] local `process.exec` async cancel smoke 覆盖。
- [x] per-run execution guard smoke 覆盖。
- [~] provider async success/failure 覆盖。
- [ ] 增加 async 多线程并发观察压力测试。

## 5. P0 Persistence And Recovery

- [ ] 为每类 persisted object 添加 format version。
- [ ] 定义旧版本读取兼容策略。
- [ ] 对 unknown newer version 返回明确错误。
- [ ] file backend 使用 temp file + atomic rename。
- [ ] 半写文件不会污染索引。
- [ ] smoke 覆盖损坏文件恢复。
- [ ] 支持从 last checkpoint 恢复 run。
- [ ] pending tool/approval 能恢复到可继续状态。
- [ ] 明确 terminal/process session 恢复边界。
- [~] 继续补齐 query performance 和 pagination。
- [x] artifact summary query 已支持多条件和分页。
- [ ] run index query 覆盖更多排序和分页。
- [ ] 大量 artifacts/events 下查询稳定。

## 6. P1 Artifact Schema

- [~] 固化 patch artifact schema。
- [x] patch stats 贯穿 runtime/persistence/memory。
- [ ] 定义 patch apply result schema。
- [ ] 支持 per-file change summary。
- [~] 固化 report artifact schema。
- [x] report class metadata 贯穿 runtime/persistence/memory。
- [ ] 稳定 plan/progress/final report 字段。
- [ ] AI IDE 和 claw 示例使用同一 report schema。
- [ ] 定义 diagnostics artifact schema。
- [ ] 定义 diagnostic severity/source/location/message。
- [ ] 支持 command output 到 diagnostics artifact。
- [ ] 支持 diagnostics persistence query。
- [~] 固化 terminal/session artifact schema。
- [x] terminal read/write/resize/stop/list artifacts 已生成。
- [x] artifact summary query 可恢复 terminal chain。
- [ ] 文档化 terminal session artifact schema。

## 7. P1 Workspace Memory And Context

- [~] 硬化 memory sync policy。
- [x] workspace root sync helper 已存在。
- [x] single-file sync helper 已存在。
- [ ] 支持 include/exclude patterns。
- [ ] 支持最大文件大小/二进制文件策略。
- [ ] 固定 memory search 结果注入 model request 的策略。
- [ ] 支持 max blocks/tokens。
- [ ] 支持 priority/pinned context。
- [ ] smoke 覆盖 memory context truncation。
- [~] 固化 tool/artifact memory ingest policy。
- [x] tool/artifact ingest hooks 已接入。
- [ ] 支持按 artifact class/kind 控制 ingest。
- [ ] 敏感内容不默认 ingest。

## 8. P1 xllm Integration

- [~] 扩展 real provider smoke matrix。
- [x] OpenAI-compatible / Anthropic / Ollama offline stub 覆盖。
- [ ] 至少一个真实 provider 成功路径可选 smoke。
- [ ] 真实 provider 错误路径可选 smoke。
- [~] 扩展 provider error-path matrix。
- [x] offline HTTP 500 -> failed run 覆盖。
- [ ] 覆盖 auth/rate-limit/malformed/timeout。
- [ ] 稳定错误 summary 和 status。
- [ ] 定义 run/session policy 中的 compaction knobs。
- [ ] compaction event/checkpoint 可观察。
- [ ] smoke 覆盖 compaction 触发。

## 9. P1 Product Profiles And Examples

- [ ] 创建 minimal AI IDE agent example。
- [ ] AI IDE example 创建 runtime/workspace/local host/profile。
- [ ] AI IDE example 演示读取文件、调用模型、产出 patch/report artifact。
- [ ] AI IDE example 支持 approval pause/resume。
- [ ] 创建 minimal claw autonomous example。
- [ ] claw example 使用 `xclaw` profile。
- [ ] claw example 执行 process/terminal tool。
- [ ] claw example 持久化后可恢复。
- [~] 硬化 `xcode` / `xclaw` profile。
- [x] builtin profile 已存在。
- [ ] 明确 `xcode` 默认交互式 approval 策略。
- [ ] 明确 `xclaw` 默认 autonomous 策略。
- [ ] profile smoke 覆盖关键默认值。

## 10. P1 Testing And CI

- [~] 拆分 smoke test 组织。
- [ ] core/host/orchestrator/persistence/provider smoke 分离。
- [ ] 每个 smoke 有独立构建命令。
- [ ] README 同步测试命令。
- [ ] 建立 CI matrix。
- [ ] 添加 C11 编译 job。
- [ ] 添加 `xwork.c` standalone compile job。
- [ ] 添加 smoke test job。
- [ ] 可选 provider smoke 不阻塞默认 CI。
- [ ] 添加 stress and race tests。
- [ ] async status/cancel/destroy 并发压力测试。
- [ ] terminal long-output stress。
- [ ] persistence many-runs/artifacts query stress。

## 11. P2 Packaging And Release

- [ ] 文档化 single-header/single-c aggregation 使用方式。
- [ ] 稳定 include/lib 目录布局。
- [ ] 示例工程可编译。
- [ ] 定义 semver 或内部版本规则。
- [ ] 维护 changelog。
- [ ] 记录 xrt/xllm 兼容版本。

## 12. P3 Deferred

- [x] v1 不实现真正多 agent 并行调度，只保留未来边界。
- [x] v1 不实现 remote worker / cloud control plane。
- [x] v1 不实现完整 deterministic replay platform。

## 13. 建议推进顺序

1. `API-001` / `API-002`: 先固定 public API contract 和错误语义。
2. `HOST-001` / `HOST-002`: 补 agent 最常用 workspace 探索和 patch apply。
3. `POLICY-001` / `POLICY-002`: 给文件和命令操作加硬边界。
4. `PERSIST-001` / `PERSIST-002`: 加 format versioning 和 crash-safe writes。
5. `EXAMPLE-001` / `EXAMPLE-002`: 做 AI IDE 和 claw 最小接入样例。
6. `TEST-001` / `TEST-002`: 拆 smoke 并建立 CI。

## 14. 更新规则

- 每完成一个任务，更新本文件对应勾选状态。
- 每新增 public API，必须补 ownership/lifetime/error contract。
- 每新增 host tool，必须补 policy/approval 行为和 smoke。
- 每新增 artifact schema，必须贯穿 runtime snapshot、persistence、query、memory ingest。
- 每新增 persistence 字段，必须考虑 format versioning。
