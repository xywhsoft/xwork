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
- 公共 API 草案：[xwork.h](/D:/git/xwork/xwork.h)
- 公共实现入口：[xwork.c](/D:/git/xwork/xwork.c)
- 已落成的最小共享 runtime 主线：
  - runtime / workspace / tool registry / run lifecycle
  - event / approval request / checkpoint / artifact 公共对象
  - `xllm` model-turn + tool-loop 最小编排闭环
  - approval pause/resume 与 checkpoint load/recover
  - workspace memory attach / tool-result ingest / artifact ingest 入口
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
- builtin host tools 已有 `filesystem.read_text` / `filesystem.write_text` / `process.exec` / `vcs.status`
- builtin host tool 在 orchestrator 中已能自动落最小 output/command artifact
- `filesystem.write_text` 最小 request contract 已支持 `mode=append`
- `process.exec` 最小 request contract 已支持 `cwd` 覆盖
- `process.exec` 最小 request contract 已支持 `max_output_bytes` 截断
- `process.exec` 最小 request contract 已支持 `env:["KEY=VALUE"]`

这一层接下来更重要的是：

- 收紧 request/response contract，而不是继续堆新的 host kind
- 补更真实的 filesystem/process/vcs 操作面和平台差异处理
- 为后续 streaming / richer artifacts 保留稳定边界

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
4. 补更细的 artifact 语义和输出类型，而不是继续堆更多顶层对象
5. 在当前同步 loop 之上再考虑 streaming / interrupt / richer orchestration

当前阶段最重要的不是再发明更多名词，而是把已经存在的 runtime 主线做稳、做可测、做可恢复。
