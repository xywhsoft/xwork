# xwork 设计文档 v1

## 1. 文档目的

本文档用于定义 `xwork` 的正式设计方向。

`xwork` 是位于 `xllm` 之上的共享 orchestration/runtime 层，服务于上层产品：

- `xcode`
- `xclaw`

本文档重点回答以下问题：

1. `xwork` 到底是什么，不是什么
2. 为什么 `xllm` 之上还需要一层 `xwork`
3. `xwork` 与 `xrt`、`xllm`、`xcode`、`xclaw` 的边界如何划分
4. `xwork` 应该抽象哪些共享能力
5. `xwork` 的核心对象、状态模型和执行模型是什么
6. `xwork` v1 的默认实现顺序和范围边界是什么

## 2. 项目定位与边界

### 2.1 `xwork` 的定位

`xwork` 是一个共享的 AI 工作流与任务运行时层。

它位于：

- 下层：`xllm`
- 上层：`xcode` / `xclaw`

一句话概括：

- `xllm` 负责“统一模型调用”
- `xwork` 负责“围绕工作区、工具、任务、状态与恢复去编排模型调用”

### 2.2 为什么 `xllm` 之上还需要 `xwork`

`xllm` 已经足够承担 LLM runtime 底座职责，但它设计上明确不负责：

- Agent 框架
- 工具真实业务执行逻辑
- UI / CLI / Web 网关逻辑
- 会话持久化后端
- 长期工作流编排

而 `xcode` 和 `xclaw` 会共享大量不属于 `xllm` 的上层能力，例如：

- 工作区与文件变更模型
- 工具注册、权限和审批
- shell / process / git / patch / test 的统一执行语义
- 任务状态、checkpoint、恢复与重放
- 运行事件总线与可观测性
- 自动编排 `session` / `memory` / tool loop 的策略
- 单 agent 与多 agent 的共享调度模型

如果没有 `xwork`，这些能力会被：

- 重复写在 `xcode`
- 再重复写一遍在 `xclaw`

这会导致：

- 语义漂移
- 权限模型不一致
- 状态恢复模型不一致
- 回归和观测体系分裂

### 2.3 `xwork` 负责什么

`xwork` 负责：

- 统一工作区模型
- 统一任务 / run / step 状态机
- 统一工具注册与执行编排
- 统一审批与策略控制
- 统一 shell / process / vcs / patch / test 等共享工具语义
- 基于 `xllm` 的模型调用编排
- 基于 `xllm_session` 的短期上下文编排
- 基于 `xllm_memory` 的长期记忆接入策略
- checkpoint / resume / replay
- 运行事件、trace、artifact、日志和评测挂点
- 多任务 / 子任务 / delegation 的共享运行时语义

### 2.4 `xwork` 不负责什么

`xwork` 不负责：

- provider 协议适配
- 模型请求 / 响应底层标准化
- token 计数、流式协议解析、thinking / artifact 底层语义
- editor UI、terminal UI、chat UI
- 具体 IDE 产品体验
- 具体 autonomous agent 产品策略品牌化表达
- 外部数据库实现细节
- 远程云端控制平面

### 2.5 `xwork` 不是

`xwork` 不是：

- `xllm` 的替代品
- 某家模型厂商 SDK
- 某个具体 UI 产品
- 只服务 `xcode` 的私有中间层
- 只服务 `xclaw` 的 agent 框架壳

它应当是一层对两个产品都成立的共享基础设施。

## 3. 分层关系

默认分层如下：

- `xrt`
- `xllm`
- `xwork`
- `xcode` / `xclaw`

各层职责如下：

### 3.1 `xrt`

负责：

- 基础运行时
- 内存、字符串、容器、异步、网络、文件、时间等基础设施

### 3.2 `xllm`

负责：

- 多 provider / 多模型统一调用
- request / response / stream / tool / error 统一语义
- `session`
- `memory`

### 3.3 `xwork`

负责：

- 将“模型能力”组织成“可恢复的工作流能力”
- 将“工具调用语义”组织成“可审批、可追踪、可恢复的工具执行”
- 将“工作区和任务状态”组织成“产品可用的 runtime”

### 3.4 `xcode` / `xclaw`

负责：

- 产品交互层
- 产品策略层
- 产品体验层
- 产品特有 host bridge

## 4. 总体设计原则

### 4.1 `xllm` 保持纯净，`xwork` 承担编排

`xwork` 不能把 `xllm` 的边界重新污染掉。

原则是：

- provider / protocol 问题留在 `xllm`
- workflow / task / tool / checkpoint 问题放在 `xwork`

### 4.2 工作区优先，而不是聊天优先

`xwork` 的核心对象不应是“纯 chat 会话”。

`xwork` 面向的是：

- 工作区
- 任务
- 步骤
- 工具
- 变更
- 恢复

聊天只是这些对象上的一个交互面。

### 4.3 状态可恢复优先

`xwork` 的所有关键流程都应以可恢复为前提设计。

这意味着：

- 重要边界有 checkpoint
- 关键动作有事件记录
- tool 执行前后有明确状态转移
- run 中断后可恢复而不是整体丢失

### 4.4 副作用显式化

`xwork` 面向真实工作区和真实命令执行。

因此副作用必须是显式对象，而不是隐式函数调用：

- 文件写入
- patch 应用
- shell 执行
- git 变更
- 外部网络访问
- 工具审批

都需要进入统一状态机和事件流。

### 4.5 审批与策略是一等能力

`xcode` 和 `xclaw` 的核心差异之一在于自主性与审批策略不同。

所以审批不能散落在各个工具实现里，而应成为 `xwork` 的一等模型。

### 4.6 嵌入式优先，服务化保留空间

`xwork` v1 默认按“库内嵌模式”设计。

但其核心对象、事件和状态应能无损映射为 JSON / 持久化记录，为未来：

- 本地 daemon
- 远程 worker
- 多进程 host

预留空间。

### 4.7 产品策略可插拔

`xwork` 不应把“交互式 coding assistant”与“autonomous agent”硬编码成两套完全不同的 runtime。

更好的做法是：

- 统一 run engine
- 统一 tool / checkpoint / approval / event 模型
- 通过 profile / policy / orchestration mode 区分产品策略

## 5. 核心对象模型

`xwork` v1 建议至少建立以下一级对象：

- `runtime`
- `workspace`
- `run`
- `step`
- `tool`
- `approval_request`
- `checkpoint`
- `artifact`
- `event`

### 5.1 `runtime`

`runtime` 是全局运行时容器。

负责持有：

- `xllm_runtime`
- host service registry
- tool registry
- policy registry
- persistence backends
- logger / trace / metrics hooks

### 5.2 `workspace`

`workspace` 表示一个工作上下文。

它至少应包含：

- workspace id
- root path
- ignore / include 规则
- 文件系统视图
- git / vcs 句柄
- diagnostics 视图
- 可选 editor bridge
- 可选 `xllm_memory`

### 5.3 `run`

`run` 表示一次完整任务执行。

它应包含：

- run id
- parent run id
- mode
- goal / instruction
- workspace refs
- state
- policy snapshot
- model profile snapshot
- session state ref
- checkpoint cursor

### 5.4 `step`

`step` 表示 `run` 中的最小状态推进单元。

典型 step 类型包括：

- model turn
- tool execution
- approval wait
- checkpoint save
- replay / resume
- subtask spawn
- merge result

### 5.5 `tool`

`tool` 是可注册、可发现、可审批、可追踪的能力单元。

它不只是一个函数指针，还应有：

- tool id
- kind
- description
- input schema
- side-effect class
- approval policy
- timeout / retry policy
- idempotency hints

### 5.6 `approval_request`

`approval_request` 表示一个待决策动作。

它应至少包含：

- request id
- run id
- tool id
- reason
- risk level
- scope
- proposed action summary
- decision state

### 5.7 `checkpoint`

`checkpoint` 表示某一时刻可恢复状态。

它应覆盖：

- run state
- pending step
- session state
- tool outputs snapshot
- workspace snapshot refs
- artifact refs

### 5.8 `artifact`

`artifact` 表示运行过程中形成的重要输出。

包括但不限于：

- transcript
- plan
- patch
- changed file list
- command stdout / stderr summary
- test report
- model visible output
- structured JSON result

### 5.9 `event`

`event` 是 `xwork` 的统一观测真相。

所有关键动作都应被表达为 typed event，而不是零散日志字符串。

## 6. 核心执行模型

### 6.1 单次 run 主线

默认执行主线：

1. 创建 `run`
2. 绑定 `workspace`
3. 读取 / 恢复 `session`
4. 构建当前上下文
5. 按策略接入 `memory`
6. 调用 `xllm`
7. 消费模型输出
8. 如需工具，则进入审批 / 执行 / 写回
9. 形成 checkpoint
10. 继续下一轮，直到完成 / 中止 / 失败

### 6.2 tool loop 属于 runtime，不属于模型适配层

在 `xwork` 中：

- `xllm` 提供 tool call 统一语义
- `xwork` 决定是否执行、如何执行、何时 checkpoint、何时审批、何时重试

这层职责不能反向压回 `xllm`。

### 6.3 run 应支持前台与后台两种模式

前台模式适合：

- `xcode`
- 用户交互式编程

后台模式适合：

- `xclaw`
- 长任务
- 队列任务
- 恢复后继续运行

### 6.4 delegation 应是 run 树，而不是 ad-hoc 递归

为支持 OpenClaw 风格能力，`xwork` 应把子任务视为一等对象：

- parent run
- child run
- result merge

而不是临时在 prompt 里拼装“子 agent”概念。

v1 可以先不做复杂并行调度，但数据模型上应原生支持：

- parent-child run relation
- child result artifact
- merge step

## 7. 数据与状态模型

### 7.1 run 状态

建议至少区分以下状态：

- `created`
- `ready`
- `running`
- `waiting_approval`
- `waiting_tool`
- `checkpointing`
- `paused`
- `completed`
- `cancelled`
- `failed`

### 7.2 step 状态

建议至少区分：

- `pending`
- `running`
- `succeeded`
- `failed`
- `cancelled`
- `skipped`

### 7.3 工具副作用分级

工具不应只分“能用 / 不能用”，还应有副作用等级。

建议至少分：

- `read_only`
- `workspace_write`
- `process_exec`
- `network_access`
- `external_mutation`

审批策略和默认自动化策略应基于这个等级。

### 7.4 变更模型

`xwork` 应建立统一 change set 模型。

至少覆盖：

- 文件新增
- 文件修改
- 文件删除
- patch hunk
- command-generated artifact
- test / build result impact

这样 `xcode` 和 `xclaw` 就能共享：

- 预览
- 审批
- 应用
- 回滚
- 汇总

### 7.5 checkpoint 模型

checkpoint 不应只是“存一段聊天记录”。

它应至少能恢复：

- 当前 run state
- 已执行 step 列表
- `xllm_session` state
- 待处理 tool queue
- approval queue
- artifact refs
- workspace state refs

## 8. 模块拆分建议

建议 `xwork` 对内按以下模块拆分：

- `core`
- `workspace`
- `tools`
- `orchestrator`
- `policy`
- `persistence`
- `artifacts`
- `host`
- `profiles`
- `eval`

### 8.1 `core`

负责：

- 公共对象
- state machine
- ids
- event envelope
- error model

### 8.2 `workspace`

负责：

- workspace root
- 路径规范化
- ignore 规则
- snapshot / change set
- 与 `xllm_memory` 的接入编排

### 8.3 `tools`

负责：

- tool registry
- tool schema
- side-effect classification
- tool execution adapter

### 8.4 `orchestrator`

负责：

- run engine
- step scheduling
- model turn 编排
- tool loop 编排
- delegation 编排

### 8.5 `policy`

负责：

- approval
- sandbox policy
- path / command / network rules
- autonomy level

### 8.6 `persistence`

负责：

- event log
- checkpoint store
- run index
- artifact metadata

### 8.7 `artifacts`

负责：

- patch artifact
- command artifact
- report artifact
- transcript artifact

### 8.8 `host`

负责：

- filesystem bridge
- process / shell bridge
- git / vcs bridge
- diagnostics bridge
- editor bridge

### 8.9 `profiles`

负责：

- interactive coding profile
- autonomous task profile
- approval defaults
- retry / checkpoint defaults

### 8.10 `eval`

负责：

- replay
- benchmark
- trace export
- regression scenario hooks

## 9. 与 `xllm` 的关系

### 9.1 `xwork` 必须复用 `xllm`，而不是绕开

`xwork` 中所有模型调用都应经由 `xllm`。

`xwork` 不应重新实现：

- provider adapter
- response normalization
- tool call wire protocol
- session compaction
- memory retrieval engine

### 9.2 `xwork` 负责使用策略，不负责底层能力复制

例如：

- 什么时候调用 `xllm_session_chat`
- 什么时候做 `xllm_session_compact`
- 什么时候执行 `xllm_memory_search`
- 如何把 memory hit 注入 request
- tool result 如何回灌下一轮

这些是 `xwork` 的职责。

### 9.3 `xwork` 应统一 `session` 与 `memory` 的接入时机

`xllm` 提供能力，但不替产品决定策略。

`xwork` 应至少支持以下策略点：

- run 开始时是否加载历史 session
- 每轮调用前是否检索 memory
- 哪些 tool result 值得写入 memory
- 哪些 artifact 进入长期索引

### 9.4 `xwork` 应保留对 `xllm` profile 的显式控制

`xwork` 不应隐藏模型选择真相。

它可以提供 orchestration profile，但最终仍应能明确映射到：

- `xllm` profile id
- call options
- session options
- memory profile

## 10. host service 模型

`xwork` 不能假设只有一种宿主。

因此应定义 host service 抽象，而不是把逻辑写死在某个 UI 进程里。

建议至少抽象以下 service：

- `filesystem`
- `process`
- `terminal`
- `vcs`
- `diagnostics`
- `editor`
- `notification`
- `secrets`

### 10.1 `filesystem`

负责：

- 读文件
- 写文件
- 删除
- move
- stat
- directory scan

### 10.2 `process`

负责：

- 执行命令
- 收集 stdout / stderr
- timeout
- cancellation

### 10.3 `vcs`

负责：

- status
- diff
- add
- commit
- branch context

### 10.4 `editor`

这是可选 bridge。

它只服务需要 editor buffer 语义的产品，例如 `xcode`。

它不应成为 `xwork` 的强依赖。

## 11. 工具系统设计

### 11.1 tool 定义应强于“命令列表”

tool 至少应包含：

- `tool_id`
- `display_name`
- `kind`
- `description`
- `input_schema`
- `output_schema`
- `side_effect_class`
- `approval_mode`
- `supports_stream`

### 11.2 tool 执行必须有统一结果模型

统一结果模型至少应包含：

- status
- structured output
- visible summary
- stdout / stderr refs
- changed files
- produced artifacts
- retryability

### 11.3 patch 不能只是 shell 的副产物

对 coding 类产品，patch 应是正式一等对象。

因为它参与：

- 预览
- 审批
- 冲突处理
- 回滚
- 最终结果汇总

### 11.4 shell 只是工具的一种

不要把 `xwork` 设计成“LLM + shell wrapper”。

`shell` 只是工具家族中的一个成员。

与之并列的还应包括：

- file tools
- patch tools
- search tools
- vcs tools
- diagnostics tools
- test tools
- subtask tools

## 12. 审批与策略模型

### 12.1 自主性等级应可配置

建议至少支持：

- `manual`
- `semi_auto`
- `auto`

### 12.2 审批决策应基于上下文

审批不能只看 tool id。

还应综合：

- side-effect class
- path scope
- workspace trust
- network target
- command summary
- run mode

### 12.3 策略快照应绑定到 run

run 开始后，核心策略应形成快照并与 run 一起持久化。

这样恢复时：

- 行为一致
- 审计一致
- 回放一致

## 13. 持久化与恢复

### 13.1 建议采用“事件日志 + 快照”双轨

只存最终状态不够。

推荐至少保存：

- event log
- latest materialized run snapshot
- checkpoint blobs
- artifact metadata

### 13.2 恢复粒度

`xwork` 应支持至少两种恢复：

- run 级恢复
- checkpoint 级恢复

### 13.3 可回放性

对调试和评测来说，仅能恢复还不够，还需要可回放。

回放目标包括：

- prompt / response 路径
- tool decision 路径
- approval 路径
- failure path

## 14. 可观测性与评测

### 14.1 event 是观测真相

日志可以是派生物，但 event 必须是正式对象。

### 14.2 trace 维度

建议至少记录：

- run lifecycle
- model request / response summary
- tool execution
- approval wait / decision
- checkpoint save / load
- artifact emission
- delegation tree

### 14.3 评测能力应从 v1 预埋

即使 v1 不做完整 eval 平台，也应为以下能力预留：

- scenario replay
- deterministic fixture
- golden output compare
- trace export

## 15. 面向 `xcode` 与 `xclaw` 的产品映射

### 15.1 `xcode`

`xcode` 主要复用 `xwork` 的：

- workspace
- file / patch / search / test 工具
- interactive run profile
- approval
- checkpoint
- diagnostics bridge

`xcode` 自己负责：

- editor UX
- inline diff / accept / reject
- buffer 语义
- 面向用户的交互细节

### 15.2 `xclaw`

`xclaw` 主要复用 `xwork` 的：

- run engine
- background task
- delegation model
- durable checkpoint
- policy / approval
- tool runtime

`xclaw` 自己负责：

- autonomous planning 策略
- 队列和任务管理体验
- 面向 agent 的产品行为设定

### 15.3 共享层不应偏向某一个产品

一个简单检验标准是：

- 如果某能力只对 editor 有意义，应放在 `xcode`
- 如果某能力只对 autonomous queue 有意义，应放在 `xclaw`
- 如果两者都需要一致语义，应放在 `xwork`

## 16. v1 范围建议

### 16.1 v1 必做

- runtime / workspace / run / step 基础对象
- tool registry
- approval model
- process / filesystem / vcs 基础 host service
- `xllm` 集成
- `xllm_session` 集成
- `xllm_memory` 接入编排
- checkpoint save / load
- event log
- patch / command / report artifact
- interactive profile
- autonomous profile 的最小闭环

### 16.2 v1 可选增强

- child run / delegation
- richer diagnostics bridge
- replay runner
- benchmark harness
- network tool family
- remote worker

### 16.3 v1 不做

- 分布式调度
- 多机控制平面
- 云端队列系统
- 复杂多租户权限系统
- 完整 GUI

## 17. 实现顺序建议

### M0: 核心对象与事件模型

完成：

- ids
- run / step state
- event envelope
- error model

### M1: host service 与 tool registry

完成：

- filesystem
- process
- vcs
- tool registry
- approval skeleton

### M2: `xllm` 编排闭环

完成：

- model turn
- tool result 回灌
- session state 管理
- checkpoint 前后状态转换

### M3: workspace 与 memory

完成：

- workspace snapshot
- `xllm_memory` 接入
- change set
- patch artifact

### M4: checkpoint / resume

完成：

- event log
- snapshot store
- run recovery

### M5: 产品 profile

完成：

- `xcode` interactive profile
- `xclaw` autonomous profile

## 18. 仓库组织建议

建议初始目录结构：

```text
xwork/
  DESIGN.md
  README.md
  include/
  src/
    xwork_core/
    xwork_workspace/
    xwork_tools/
    xwork_orchestrator/
    xwork_policy/
    xwork_persistence/
    xwork_artifacts/
    xwork_host/
    xwork_profiles/
  examples/
  tests/
  docs/
```

## 19. 当前结论

`xwork` 的存在是合理且必要的。

它的目标不是复制 `xllm`，而是在 `xllm` 之上建立一层共享的“工作流运行时真相”。

如果这层做对了：

- `xcode` 不需要自己重复造 agent runtime
- `xclaw` 不需要自己重复造 coding workspace runtime
- `xllm` 仍能保持清晰边界

最终三层关系应稳定为：

- `xllm`：模型能力真相
- `xwork`：工作流与任务运行时真相
- `xcode` / `xclaw`：产品体验真相
