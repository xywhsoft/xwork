# Multi-Agent API

Multi-Agent API in-process agent pool_ask graph_andoff_hild run_snapshot AI IDE, Law, Law, Law, Law, Law, Law, Law, Law, and Law.缁勭粐鴴愬嬲瀹¤銆佸彲鎭㈠鄄勪 Change 锷″浘銆?
##妯″桡杈戈晫

- agent pool, agent, agent, task graph - `xwork_run` model-turn tool-loop run/orchestrator - snapshot鍙仮侶嶅彽搴忓垪鍖栫堫姐侊绂native thread銆乧乧allback 镙堛€丸鮮?live handle 涓鈭掭㈠銆?-graph mutation 搴斾笌 execute/query涓茶鍖栵绂task callback 鑻ュ苟鍙戣繍盛狋纴 dark€罽彜琛豼iao鎶ゅRuibang祫婧愩€?
## gallium€chain夋戈绾﹀畾

| Silicon thin | Gallium chain |
| --- | --- |
|
|
|
|
|
| `*_list_reset` / `*_snapshot_reset` |

## 鏏 manuscript瀷璋卂敤椤 coax

```text
xwork_agent_pool_options_init
xwork_agent_pool_create
xwork_agent_options_init
xwork_agent_pool_add_agent
xwork_task_graph_options_init
xwork_task_graph_create
xwork_task_node_options_init
xwork_task_graph_add_node
xwork_task_graph_add_dependency
xwork_task_graph_execute
xwork_task_graph_get_snapshot
xwork_task_graph_destroy
xwork_agent_pool_destroy
```

## 卒濆鍖栦笌Read僃斁绾﹀畾

`*_init` `NULL` `*_reset` `NULL` init钖庣姸镐与€坝皟颢ㄨ叏鍙?summary/list/snapshot 鄄?API铓嶏纴夤锤鍏?init锛涘鐢ㄥ涓涓€鼁洴瀯铓嶅厛reset?
## Agent Pool 涶?Agent

### xwork_agent_pool_options_init

Why?`xwork_agent_pool_options`?
**锷绻兘锛?*

描?pool options 哓嵂浂唛屼negative鍒涘涘 agent pool 火橩婳徶囥€?
**What's the point?*

```c
XWORK_API void xwork_agent_pool_options_init(xwork_agent_pool_options *pOptions);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰嶈祫婧愶绂璋卂敤揂 Visit粛鎷ユ恁缁撴瀯钴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

- `pRuntime` pool `pRuntime`
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_pool_options opts;
xwork_agent_pool_options_init(&opts);
opts.sPoolId = "main";
opts.pRuntime = runtime;
```

**What is the API?*

- `xwork_agent_pool_create`

---

### xwork_agent_options_init

Why?`xwork_agent_options`?
**锷绻兘锛?*

Agent options Agent options Agent options Agent agent
**What's the point?*

```c
XWORK_API void xwork_agent_options_init(xwork_agent_options *pOptions);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

`xwork_agent_pool_add_agent` `xwork_agent_pool_add_agent`浼橩鍒涒涶綷乚隣欑殑 chain note€?
**Chen ュ Pang Xuan cun 槑?*

- `XWORK_AGENT_ROLE_CUSTOM`? - `XWORK_AUTONOMY_SEMI_AUTO`?
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_options opts;
xwork_agent_options_init(&opts);
opts.sAgentId = "coder";
opts.eRole = XWORK_AGENT_ROLE_CODER;
```

**What is the API?*

- `xwork_agent_pool_add_agent`

---

### xwork_agent_snapshot_init

Why?`xwork_agent_snapshot`?
**锷绻兘锛?*

What is the snapshot API of the agent snapshot?
**What's the point?*

```c
XWORK_API void xwork_agent_snapshot_init(xwork_agent_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`锛氳鍒濆鍖栫殑 snapshot锛涘彲涓?`NULL`銆?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_agent_options_init` `xwork_agent_options_init`
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_snapshot snapshot;
xwork_agent_snapshot_init(&snapshot);
```

**What is the API?*

- `xwork_agent_snapshot_reset`
- `xwork_agent_pool_get_snapshot`

---

### xwork_agent_snapshot_reset

`xwork_agent_snapshot`?
**锷绻兘锛?*

Yue婃斁 snapshot 卐呴儴瀛楃涓诧纴骞鈮澶brand negative init Zhong Ruo€?
**What's the point?*

```c
XWORK_API void xwork_agent_snapshot_reset(xwork_agent_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`? `NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Yue僁恁 snapshot 鎸乹恁逄勬苴混综合礉瀛楁锛屼笉Read僃斁缁撴瀯铯洴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

xwork API
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_agent_snapshot_init`

---

### xwork_agent_snapshot_list_init

鍒濆鍖?agent snapshot 鍒楄〃銆?
**锷绻兘锛?*

How to use the snapshot API?
**What's the point?*

```c
XWORK_API void xwork_agent_snapshot_list_init(xwork_agent_snapshot_list *pList);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_agent_pool_get_snapshot`
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_snapshot_list list;
xwork_agent_snapshot_list_init(&list);
```

**What is the API?*

- `xwork_agent_snapshot_list_reset`

---

### xwork_agent_snapshot_list_reset

译婃斁 agent snapshot 鍒楄〃銆?
**锷绻兘锛?*

荙婃恁鍒楄〃卐呮 ulcer涓?agent snapshot 鍙婂垪曛ㄦ暟缁勩€?
**What's the point?*

```c
XWORK_API void xwork_agent_snapshot_list_reset(xwork_agent_snapshot_list *pList);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁鍒楄〃鎷ユ湁镄勫 Cliff绱犲拰珁 role play粍锛屼笉Read僃斁鍒楄〃缁撴瀯钴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

咋卂敤钖庡垪曛ㄥ洖鍒荒┖中闆€侊纴鍙啀娆′紶缁椤～鍏?API銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_snapshot_list_reset(&list);
```

**What is the API?*

- `xwork_agent_snapshot_reset`

---

### xwork_agent_pool_snapshot_init

What is the agent pool snapshot?
**锷绻兘锛?*

Agent pool?
**What's the point?*

```c
XWORK_API void xwork_agent_pool_snapshot_init(xwork_agent_pool_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`tAgents`?
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_pool_snapshot snapshot;
xwork_agent_pool_snapshot_init(&snapshot);
```

**What is the API?*

- `xwork_agent_pool_get_snapshot`
- `xwork_agent_pool_snapshot_reset`

---

### xwork_agent_pool_snapshot_reset

Read the agent pool snapshot?
**锷绻兘锛?*

Yue婃斁 pool id鍜?agent snapshot鍒楄〃銆?
**What's the point?*

```c
XWORK_API void xwork_agent_pool_snapshot_reset(xwork_agent_pool_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

荐僃斁 snapshot 卐呴儴鎷ユ湁镄勬脴鎷Mad礉璧勬簮锛屼笉Read僃斁缁撴瀯撴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_agent_pool_create_from_snapshot`
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_pool_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_agent_pool_create_from_snapshot`

---

### xwork_agent_pool_create

What is the agent pool?
**锷绻兘锛?*

The runtime is runtime, in-process agent pool, agent task graph, in-process agent pool.
**What's the point?*

```c
XWORK_API xwork_status xwork_agent_pool_create(
    const xwork_agent_pool_options *pOptions,
    xwork_agent_pool **ppPool
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

- `XWORK_ERROR_NO_MEMORY`唛橩唴瀛华垎喰嶅け璐ャ€?
**璧勬簮褰掎睘锛?*

`*ppPool` `*ppPool` `xwork_agent_pool_destroy` `*ppPool` runtime runtime untime pool Wa's teachings and spinning emerald harp?
**Chen ュ Pang Xuan cun 槑?*

`sPoolId`
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_pool *pool = NULL;
xwork_agent_pool_options opts;
xwork_agent_pool_options_init(&opts);
opts.pRuntime = runtime;
opts.sPoolId = "main";
xwork_status st = xwork_agent_pool_create(&opts, &pool);
```

**What is the API?*

- `xwork_agent_pool_destroy`
- `xwork_agent_pool_add_agent`

---

### xwork_agent_pool_create_from_snapshot

浠?snapshot 鎭㈠agent pool銆?
**锷绻兘锛?*

What is the pool snapshot and the agent pool is the agent pool?
**What's the point?*

```c
XWORK_API xwork_status xwork_agent_pool_create_from_snapshot(
    xwork_runtime *pRuntime,
    const xwork_agent_pool_snapshot *pSnapshot,
    xwork_agent_pool **ppPool
);
```

**卙四暟锛?*

- `pRuntime` is a pool that has a runtime pool and a runtime pool -
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

Reset?
**Chen ュ Pang Xuan cun 槑?*

鍭㈠鍙正钖?agent 鍏冩暟鎹纴涓嶆仮澶?runtime 鍐呭閮?live 正典 €?
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_pool *pool = NULL;
xwork_status st = xwork_agent_pool_create_from_snapshot(runtime, &snapshot, &pool);
```

**What is the API?*

- `xwork_agent_pool_get_snapshot`
- `xwork_agent_pool_destroy`

---

### xwork_agent_pool_destroy

阌€骣?agent pool銆?
**锷绻兘锛?*

The pool is the pool where the agent is.
**What's the point?*

```c
XWORK_API void xwork_agent_pool_destroy(xwork_agent_pool *pPool);
```

**卙四暟锛?*

- `pPool`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read the pool, runtime?
**Chen ュ Pang Xuan cun 槑?*

阌€姣?pool 铓嶏纴搴濛厛阌€姣丸€窺椤璇?pool 镄?task graph銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_pool_destroy(pool);
```

**What is the API?*

- `xwork_agent_pool_create`

---

### xwork_agent_pool_add_agent

钖?pool剉ㄥ彽agent銆?
**锷绻兘锛?*

涶嶅尗 agent options锛屽碢 agent 瀹hydrogen箟锷珲uke pool锛屽苟鍙繑锲?borrowed agent 鎸 returned拋銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_agent_pool_add_agent(
    xwork_agent_pool *pPool,
    const xwork_agent_options *pOptions,
    xwork_agent **ppAgent
);
```

**卙四暟锛?*

- `pPool` agent pool agent `ppAgent`?borrowed agent?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

agent 鐢?pool 鎷ユ湁揗旗ppAgent` 杩洿洖chain熺椤掸Back拋招宲ool 阌€姣丗澶综合晥抆?
**Chen ュ Pang Xuan cun 槑?*

agent id pool
**锣冧緥締ｇ爜锛?*

```c
xwork_agent *agent = NULL;
xwork_agent_options opts;
xwork_agent_options_init(&opts);
opts.sAgentId = "reviewer";
opts.eRole = XWORK_AGENT_ROLE_REVIEWER;
xwork_agent_pool_add_agent(pool, &opts, &agent);
```

**What is the API?*

- `xwork_agent_pool_find_agent`
- `xwork_agent_get_id`

---

### xwork_agent_pool_get_agent_count

銮峰彇agent 鏁比忺銆?
**锷绻兘锛?*

杩斿洖pool涓WherePingㄥ唽鄄?agent涓暟銆?
**What's the point?*

```c
XWORK_API size_t xwork_agent_pool_get_agent_count(const xwork_agent_pool *pPool);
```

**卙四暟锛?*

- `pPool` gent pool?
**杩斿洴 alkali fine**

杩斿洖agent 鏁比噺攛沗pPool` 涓?`NULL` 鏃惰繑鍥?`0`抆?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

璇ュ€和槸褰揿堠 in-memory pool 锄勫揩镦у甇璇画暟銆?
**锣冧緥締ｇ爜锛?*

```c
size_t count = xwork_agent_pool_get_agent_count(pool);
```

**What is the API?*

- `xwork_agent_pool_add_agent`

---

### xwork_agent_pool_find_agent

鎸?id 镆ユ缦 agent銆?
**锷绻兘锛?*

What's the pool number?
**What's the point?*

```c
XWORK_API xwork_agent *xwork_agent_pool_find_agent(
    const xwork_agent_pool *pPool,
    const char *sAgentId
);
```

**卙四暟锛?*

- `pPool` gent pool? - `sAgentId` gent id?
**杩斿洴 alkali fine**

`NULL` `NULL`?
**璧勬簮褰掎睘锛?*

杩斿洴Chain shoulder 敱汕 pool 鎷ユ湁锛岃皟鐢ㄨ€呬笉鑳狠淳蕉€?
**Chen ュ Pang Xuan cun 槑?*

What is the agent pool?
**锣冧緥締ｇ爜锛?*

```c
xwork_agent *coder = xwork_agent_pool_find_agent(pool, "coder");
```

**What is the API?*

- `xwork_agent_get_role`

---

### xwork_agent_pool_get_snapshot

銮峰彇 agent pool snapshot銆?
**锷绻兘锛?*

pool id pool id agent agent snapshot shot?
**What's the point?*

```c
XWORK_API xwork_status xwork_agent_pool_get_snapshot(
    const xwork_agent_pool *pPool,
    xwork_agent_pool_snapshot *pSnapshot
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

snapshot 鎷ユ湁娣比嫹琐濴婴縸纴咋卂敤Key呯椤
**Chen ュ Pang Xuan cun 槑?*

鍑 must be used
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_pool_snapshot snapshot;
xwork_agent_pool_snapshot_init(&snapshot);
xwork_agent_pool_get_snapshot(pool, &snapshot);
xwork_agent_pool_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_agent_pool_create_from_snapshot`

---

### xwork_agent_get_id

銮峰彇agent id銆?
**锷绻兘锛?*

杩斿洖 agent 镄?id 瀛楃涓layer€?
**What's the point?*

```c
XWORK_API const char *xwork_agent_get_id(const xwork_agent *pAgent);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

杩濿洖 borrowed id锛旗pAgent` 涓?`NULL` 鏃惰繑鍥?`NULL`抆?
**璧勬簮褰掎睘锛?*

杩斿洖chainshoulder敱汕agent pool掷ユ湁锛岃皟鐢ㄨ€呬笉鑳综合狠鏀都€?
**Chen ュ Pang Xuan cun 槑?*

璇?id 鍙捤浜?task node 鄄?`sAgentId`銆?
**锣冧緥締ｇ爜锛?*

```c
const char *id = xwork_agent_get_id(agent);
```

**What is the API?*

- `xwork_task_graph_add_node`

---

### xwork_agent_get_role

銮峰彇agent 铮厕磊銆?
**锷绻兘锛?*

杩洿洖agent鄄?`xwork_agent_role`銆?
**What's the point?*

```c
XWORK_API xwork_agent_role xwork_agent_get_role(const xwork_agent *pAgent);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

杩斿洖诺敕壣棊旗pAgent` 涓?`NULL` 鏃惰繑鍥?`XWORK_AGENT_ROLE_CUSTOM`抆?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

What's the point of this?
**锣冧緥締ｇ爜锛?*

```c
xwork_agent_role role = xwork_agent_get_role(agent);
```

**What is the API?*

- `xwork_agent_pool_add_agent`

---

## Task Node Juan?Task Graph

### xwork_task_node_options_init

What is the task node options?
**锷绻兘锛?*

What is the task graph?
**What's the point?*

```c
XWORK_API void xwork_task_node_options_init(xwork_task_node_options *pOptions);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

`xwork_task_graph_add_node` `xwork_task_graph_add_node`
**Chen ュ Pang Xuan cun 槑?*

`XWORK_AUTONOMY_SEMI_AUTO`?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_options opts;
xwork_task_node_options_init(&opts);
opts.sTaskId = "implement";
opts.sAgentId = "coder";
opts.sInstruction = "Implement the feature.";
```

**What is the API?*

- `xwork_task_graph_add_node`

---

### xwork_task_graph_options_init

What is the task graph options?
**锷绻兘锛?*

What is the task graph?
**What's the point?*

```c
XWORK_API void xwork_task_graph_options_init(xwork_task_graph_options *pOptions);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

- `1` `1` `1` graph What is the value of `pAgentPool`?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_options opts;
xwork_task_graph_options_init(&opts);
opts.sGraphId = "feature-graph";
opts.pAgentPool = pool;
opts.iMaxConcurrency = 2;
```

**What is the API?*

- `xwork_task_graph_create`

---

### xwork_task_node_summary_init

What is the task node summary?
**锷绻兘锛?*

鍑嗗涓€洓Change 锷℃憳簺粋鋶纴鐢ㄤ簬镆ヨ鍗曚釜鴴栧涓妭飣Guiya姸镐?
**What's the point?*

```c
XWORK_API void xwork_task_node_summary_init(xwork_task_node_summary *pSummary);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`XWORK_TASK_PENDING`?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_summary summary;
xwork_task_node_summary_init(&summary);
```

**What is the API?*

- `xwork_task_graph_get_node_summary`

---

### xwork_task_node_summary_reset

Read the task node summary?
**锷绻兘锛?*

Read the summary 涓敱API 壣典嫹LU戈殑瀛楃涓INSertGou韭㈠涓?init Zhongruo€and€?
**What's the point?*

```c
XWORK_API void xwork_task_node_summary_reset(xwork_task_node_summary *pSummary);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁 summary 鍍呴儴鎷ユ湁璧勬簮甛屼笉Read僃斁缁撴瀯钴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

`pUserData` Is there a problem?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_summary_reset(&summary);
```

**What is the API?*

- `xwork_task_node_summary_init`

---

### xwork_task_node_summary_list_init

鍒濆鍖?task node summary鍒楄〃銆?
**锷绻兘锛?*

鍑嗗涓€涓┖鍒楄〃锛倀敤浜庢崴鏀?graph 鑺傂偣鎽樿銆?
**What's the point?*

```c
XWORK_API void xwork_task_node_summary_list_init(xwork_task_node_summary_list *pList);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_task_graph_list_node_summaries`
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_summary_list list;
xwork_task_node_summary_list_init(&list);
```

**What is the API?*

- `xwork_task_graph_list_node_summaries`

---

### xwork_task_node_summary_list_reset

译婃斁 task node summary 鍒楄〃銆?
**锷绻兘锛?*

Read婃斁鍒楄〃涓卍chain?node summary鍜屽垪曛ㄦ暟缁卩€?
**What's the point?*

```c
XWORK_API void xwork_task_node_summary_list_reset(xwork_task_node_summary_list *pList);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Reading僃斁鍒楄〃鎷ユ湁镄勫崴縸纴涓嶉菀鞪瀛ㄧ粨鋋勪綋chain汉銆?
**Chen ュ Pang Xuan cun 槑?*

Read the API?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_summary_list_reset(&list);
```

**What is the API?*

- `xwork_task_node_summary_reset`

---

### xwork_task_node_snapshot_init

What is the task node snapshot?
**锷绻兘锛?*

鍑嗗涓€涓change锷¤妭飣?snapshot锛尀敤浜庢件侶嶆娸鸷箙鍖栥€?
**What's the point?*

```c
XWORK_API void xwork_task_node_snapshot_init(xwork_task_node_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

鍒濆鍖栧怗瀛楁涓红┖锛倀姸镐佷negative hazel樿 pending 璇箟銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_snapshot snapshot;
xwork_task_node_snapshot_init(&snapshot);
```

**What is the API?*

- `xwork_task_graph_get_snapshot`

---

### xwork_task_node_snapshot_reset

Read the task node snapshot?
**锷绻兘锛?*

译文婃斁浠氲姟鑺卜偣 snapshot 涓殑瀛楃涓layer€亀orkspace id 鏁衣粍鍜屼緷璧?id 鏁狠粍鍆?
**What's the point?*

```c
XWORK_API void xwork_task_node_snapshot_reset(xwork_task_node_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

荐婃斁 snapshot 卐呴儴鎷ユ捐璧勬簮锛屼笉Read僃斁缁撴瀯钴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

咋卂敤钖?snapshot 锲炲韌 init Zhong Duo€and€?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_task_node_snapshot_init`

---

### xwork_task_node_snapshot_list_init

鍒濆鍖?task node snapshot 鍒楄〃銆?
**锷绻兘锛?*

鍑嗗涓€涓┖ snapshot 鍒楄〃銆?
**What's the point?*

```c
XWORK_API void xwork_task_node_snapshot_list_init(xwork_task_node_snapshot_list *pList);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_task_graph_snapshot.tNodes` `xwork_task_graph_snapshot.tNodes`?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_snapshot_list list;
xwork_task_node_snapshot_list_init(&list);
```

**What is the API?*

- `xwork_task_node_snapshot_list_reset`

---

### xwork_task_node_snapshot_list_reset

Read the task node snapshot 卒楄〃銆?
**锷绻兘锛?*

Read the link?task node snapshot 鍜屾暟缁卩€?
**What's the point?*

```c
XWORK_API void xwork_task_node_snapshot_list_reset(xwork_task_node_snapshot_list *pList);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Reading僃斁鍒楄〃鎷ユ湁镄勫崴縸纴涓嶉菀鞪瀛ㄧ粨鋋勪綋chain汉銆?
**Chen ュ Pang Xuan cun 槑?*

`xwork_task_graph_snapshot_reset` 浼氶棿掺ヨ皟鐢ㄥ畠銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_snapshot_list_reset(&list);
```

**What is the API?*

- `xwork_task_graph_snapshot_reset`

---

### xwork_task_graph_result_init

What is the task graph result?
**锷绻兘锛?*

灏?graph galliumц缁撴灉璁℃暟娓崴浂銆?
**What's the point?*

```c
XWORK_API void xwork_task_graph_result_init(xwork_task_graph_result *pResult);
```

**卙四暟锛?*

- `pResult`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_task_graph_execute` 鍙～鍏呰缁撴瀯銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_result result;
xwork_task_graph_result_init(&result);
```

**What is the API?*

- `xwork_task_graph_execute`

---

### xwork_task_graph_snapshot_init

What is the task graph snapshot?
**锷绻兘锛?*

鍑嗗 graph snapshot锛妀敤浜庢崴逴贺粲鏁emerited for锷″洘中尊€and€?
**What's the point?*

```c
XWORK_API void xwork_task_graph_snapshot_init(xwork_task_graph_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰嶈祫婧愶绂捐呴儴卒楄〃鍒濆鍖栦negative廌heng€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_task_graph_get_snapshot`
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_snapshot snapshot;
xwork_task_graph_snapshot_init(&snapshot);
```

**What is the API?*

- `xwork_task_graph_get_snapshot`
- `xwork_task_graph_snapshot_reset`

---

### xwork_task_graph_snapshot_reset

Read the task graph snapshot?
**锷绻兘锛?*

Read婃斁 graph id銆佹殏 Pot?鍙栨秷秡緷洜銆佷change锷¤妭镣?snapshot鍒楄〃鍜?handoff鍒楄〃銆?
**What's the point?*

```c
XWORK_API void xwork_task_graph_snapshot_reset(xwork_task_graph_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

荐婃斁 snapshot 卐呴儴鎷ユ捐璧勬簮锛屼笉Read僃斁缁撴瀯钴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_task_graph_create_from_snapshot`
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_task_graph_create_from_snapshot`

---

### xwork_task_graph_create

What is the task graph?
**锷绻兘锛?*

鍒涘經€涓change锷″浘锛倀敤浜庢区锷?task node銆丶0鏄康緧砧栥€佢彛緽屽 agent 宸ヤ緔笧and€?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_create(
    const xwork_task_graph_options *pOptions,
    xwork_task_graph **ppGraph
);
```

**卙四暟锛?*

- `pOptions`?-`ppGraph`?owned graph?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

graph 椰掕皟鐢ㄨ€呮卍chain夛纴鐢?
**Chen ュ Pang Xuan cun 槑?*

Chain 缃?graph id 鞞Duojiao鐢ㄩ粯璁?id銆俙iMaxConcurrency
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph *graph = NULL;
xwork_task_graph_options opts;
xwork_task_graph_options_init(&opts);
opts.pAgentPool = pool;
opts.iMaxConcurrency = 2;
xwork_task_graph_create(&opts, &graph);
```

**What is the API?*

- `xwork_task_graph_destroy`
- `xwork_task_graph_add_node`

---

### xwork_task_graph_create_from_snapshot

浠?snapshot 鎭㈠task graph銆?
**锷绻兘锛?*

鍩婷簬 snapshot 卍卮簬 snapshot 荍兮妭颣广€䷷緷甧栥€乭andoff 鍜屾殏 Pot?鍙栨秧砧栥€和€?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_create_from_snapshot(
    const xwork_task_graph_options *pOptions,
    const xwork_task_graph_snapshot *pSnapshot,
    xwork_task_graph **ppGraph
);
```

**卙四暟锛?*

- `pSnapshot` is a graph snapshot? - `ppGraph` is a graph owned graph?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

掴愬姛钖?graph 褰掕皟鐢ㄨ€呮卍chain夛礂snapshot 涓嶈玺ョ銆?
**Chen ュ Pang Xuan cun 槑?*

READY抆丷UNNING銆丅LOCKED 绛?live in-flight Live handle
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph *graph = NULL;
xwork_task_graph_create_from_snapshot(&opts, &snapshot, &graph);
```

**What is the API?*

- `xwork_task_graph_get_snapshot`

---

### xwork_task_graph_destroy

阌€姣?task graph銆?
**锷绻兘锛?*

Read the graph銆佽妭飣广€乭andoff 鍜屽崴閮ㄧ姸镐和€?
**What's the point?*

```c
XWORK_API void xwork_task_graph_destroy(xwork_task_graph *pGraph);
```

**卙四暟锛?*

- `pGraph`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read the graph and use it. Agent pool and ancel token. Callback user data?
**Chen ュ Pang Xuan cun 槑?*

涓嶈鍦?graph 姝ｅ湪铓ц镞気槣丸畠銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_destroy(graph);
```

**What is the API?*

- `xwork_task_graph_create`

---

### xwork_task_graph_add_node

壣沲姞浠曲槟鑺傜偣銆?
**锷绻兘锛?*

What is the graph?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_add_node(
    xwork_task_graph *pGraph,
    const xwork_task_node_options *pOptions
);
```

**卙四暟锛?*

– `pGraph`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

graph 澶嶅嗗 task id銆佹寚浠ゃ€乸rofile id銆亀orkspace id 黛夊瓧娈碉绂`pUserData`涓椴窺敤掸Back拡抆?
**Chen ュ Pang Xuan cun 槑?*

`sAgentId`
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_options node;
xwork_task_node_options_init(&node);
node.sTaskId = "review";
node.sAgentId = "reviewer";
node.sInstruction = "Review the patch.";
xwork_task_graph_add_node(graph, &node);
```

**What is the API?*

- `xwork_task_graph_add_dependency`

---

### xwork_task_graph_add_dependency

壣貲姞浠曰槟渚濊禆銆?
**锷绻兘锛?*

`sAfterTaskId` `sAfterTaskId` `sBeforeTaskId` `sAfterTaskId`
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_add_dependency(
    xwork_task_graph *pGraph,
    const char *sBeforeTaskId,
    const char *sAfterTaskId
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

graph 涶嶅埗渚濊禆 id銆?
**Chen ュ Pang Xuan cun 槑?*

涓や釜浠谲姟閮 borrowed 椤曰Fanying birch adze 涘jing鐜緷緧栦瀦瀵Hardness嚧铓ц阒Dulm銳綶玺ㄨ繘銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_add_dependency(graph, "implement", "review");
```

**What is the API?*

- `xwork_task_graph_execute`

---

### xwork_task_graph_get_node_count

Luan Feng 彇浠氲姟麺傂偣遁 accompanying 噺銆?
**锷绻兘锛?*

杩斿洖graph涓殑task node 鏁比噺抆?
**What's the point?*

```c
XWORK_API size_t xwork_task_graph_get_node_count(const xwork_task_graph *pGraph);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

杩斿洖鑺傂偣鏁発攛沗pGraph` 涓?`NULL` 鏃惰繑鍥?`0`抆?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

璇ュ€between笉浠ｈ〃宸畲洴怪檪 for 锷℃暟Read忠€?
**锣冧緥締ｇ爜锛?*

```c
size_t count = xwork_task_graph_get_node_count(graph);
```

**What is the API?*

- `xwork_task_graph_list_node_summaries`

---

### xwork_task_graph_get_node_summary

What is the value of the product?
**锷绻兘锛?*

鎸?task id 銮峰彇翺傂偣鈥锣€与€佸皾璇曟鏁铁銺銆銆䷷緷緧砠暟翟翕瓑鎽樿皾璇曟鏁銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_get_node_summary(
    const xwork_task_graph *pGraph,
    const char *sTaskId,
    xwork_task_node_summary *pSummary
);
```

**卙四暟锛?*

- `pGraph` graph graph? -
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

summary 鎷ユ湁娣综合嫹LU濆瓧绗︿蛛妀椤
**Chen ュ Pang Xuan cun 槑?*

Summary
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_summary summary;
xwork_task_node_summary_init(&summary);
xwork_task_graph_get_node_summary(graph, "review", &summary);
xwork_task_node_summary_reset(&summary);
```

**What is the API?*

- `xwork_task_graph_list_node_summaries`

---

### xwork_task_graph_list_node_summaries

What’s the point?
**锷绻兘锛?*

铮峰彇 graph涓卍chain?task node 鄄勬憳簸丞chenㄣ€?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_list_node_summaries(
    const xwork_task_graph *pGraph,
    xwork_task_node_summary_list *pList
);
```

**卙四暟锛?*

- `pGraph`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

鍒楄〃鎷ユ湁 deep-copy 鍏卂礌雀敤
**Chen ュ Pang Xuan cun 槑?*

鍑 mustard 暟浼氶t郃緭鍑 coax 垪chen ㄧ殑镑啞у唴瀹广€?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_node_summary_list list;
xwork_task_node_summary_list_init(&list);
xwork_task_graph_list_node_summaries(graph, &list);
xwork_task_node_summary_list_reset(&list);
```

**What is the API?*

- `xwork_task_node_summary_list_reset`

---

### xwork_task_graph_get_node_run

銮峰彇麺偂偣瀵gui粲run銆?
**锷绻兘锛?*

杩斿洖镆愪采task node鍏Chong丈鄄?`xwork_run`銆?
**What's the point?*

```c
XWORK_API xwork_run *xwork_task_graph_get_node_run(
    const xwork_task_graph *pGraph,
    const char *sTaskId
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`NULL` `NULL`?
**璧勬簮褰掎睘锛?*

杩斿洖Chainshoulder敱敱 graph/run 璞四嫢厅夛纴璋卂敤敤Key呬笉鑳综合狠与€?
**Chen ュ Pang Xuan cun 槑?*

Ning Ge洤浜庺煡鐐?child run 浜嬩浩銆乻ummary鴴?artifact銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_run *run = xwork_task_graph_get_node_run(graph, "implement");
```

**What is the API?*

- `xwork_run_get_summary`

---

### xwork_task_graph_get_snapshot

銮峰彇task graph snapshot銆?
**锷绻兘锛?*

Di Bian Qianlu?graph褰揿堠clock rudder€侊纴鍖呮嫭麜偣銆佷緷甧頥€乭andoff銆佽彽琛粀粨鋋滃鏆鏆effective仠/鍙栨秷留秙笧銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_get_snapshot(
    const xwork_task_graph *pGraph,
    xwork_task_graph_snapshot *pSnapshot
);
```

**卙四暟锛?*

- `pGraph` graph銆?-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

snapshot 鎷ユ湁 deep-copy 鍍呭锛妀椤
**Chen ュ Pang Xuan cun 槑?*

snapshot persistence backend create-from-snapshot persistence backend create-from-snapshot persistence backend
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_snapshot snapshot;
xwork_task_graph_snapshot_init(&snapshot);
xwork_task_graph_get_snapshot(graph, &snapshot);
xwork_task_graph_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_task_graph_create_from_snapshot`

---

### xwork_task_graph_execute

Galliumцtask graph銆?
**锷绻兘锛?*

鎸変緷緧bi栧姧绯淯€丹涶澶у苟鍙戝拋澶Braid touch 绛栫淐It’s a good idea run?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_execute(
    xwork_task_graph *pGraph,
    xwork_task_graph_result *pResult
);
```

**卙四暟锛?*

– `pGraph`镞多玎玺Xuancun把graph 鍍呴儴缁洴灉銆?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`
**璧勬簮褰掎睘锛?*

涓嶈簉?graph gallium€chain夋潈曗旗pResult` 涓嶅戈锷ㄦ€佽祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

graph execute re-entry execution re-entry
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_result result;
xwork_task_graph_result_init(&result);
xwork_status st = xwork_task_graph_execute(graph, &result);
```

**What is the API?*

- `xwork_task_graph_pause`
- `xwork_task_graph_cancel`

---

### xwork_task_graph_cancel

What is the task graph?
**锷绻兘锛?*

璁Jujiang graph 鍙栨獙鍙锛€侊纴骞荜氰綷洜鄄?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_cancel(
    xwork_task_graph *pGraph,
    const char *sReason
);
```

**卙四暟锛?*

– `pGraph`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

graph
**Chen ュ Pang Xuan cun 槑?*

鍙栨秷涓気昌€姣?graph锛涜皟鐢ㄨ€嬬粛鍙煡璇?snapshot鍜?summary銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_cancel(graph, "user requested stop");
```

**What is the API?*

- `xwork_task_graph_is_cancelled`

---

### xwork_task_graph_is_cancelled

妫€镆?graph逄惁chenchencha姹effect彇娑四€?
**锷绻兘锛?*

杩斿洖graph 褰揿堠鍙栨秷镙肖銆?
**What's the point?*

```c
XWORK_API bool xwork_task_graph_is_cancelled(const xwork_task_graph *pGraph);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`true`?`false`?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

璇ュ嚱鏁 Board彧妫€镆ヨ姹傜姸镐侊纴涓璇璇役卍chain夎繛屼腑浠氲姟hen燬粲 Guo Cang銆?
**锣冧緥締ｇ爜锛?*

```c
if (xwork_task_graph_is_cancelled(graph)) {
    /* stop launching extra work */
}
```

**What is the API?*

- `xwork_task_graph_cancel`

---

### xwork_task_graph_pause

Does it work on the task graph?
**锷绻兘锛?*

璁jujiangjiangxiaoxuanxuanfeng簰锛屼佳铓ц鍦ㄨ皟搴﹁ Actually 鐣屽仠姝㈢屋缁惎锷ㄦ把浠淲姟銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_pause(
    xwork_task_graph *pGraph,
    const char *sReason
);
```

**卙四暟锛?*

– `pGraph`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

graph
**Chen ュ Pang Xuan cun 槑?*

揆effect仠涓嶅搓浜庡彇娑堬炂玭㈠钖?graph 鍙漁皟搴︽湭瀹屾尚浠毲姟銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_pause(graph, "waiting for approval");
```

**What is the API?*

- `xwork_task_graph_resume`
- `xwork_task_graph_is_paused`

---

### xwork_task_graph_resume

What is the task graph?
**锷绻兘锛?*

哓呴嫎鏆 effect仠璇风眰鍜豾殏殃管滃师锲箮€?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_resume(xwork_task_graph *pGraph);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

Read the graph 卍呬liaoying 樼殑鏆 effect 仠铡熷洜銆?
**Chen ュ Pang Xuan cun 槑?*

掭㈠涓鈥氕姩璋卂椤
**锣冧緥締ｇ爜锛?*

```c
xwork_task_graph_resume(graph);
xwork_task_graph_execute(graph, &result);
```

**What is the API?*

- `xwork_task_graph_pause`

---

### xwork_task_graph_is_paused

妫€镆?graph 鏄惁箸茶姹傛殏婆溿€?
**锷绻兘锛?*

What is the graph?
**What's the point?*

```c
XWORK_API bool xwork_task_graph_is_paused(const xwork_task_graph *pGraph);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`true`?`false`?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

璇ュ嚱鏁版镆ヨ姹傜姸羊纴涓brand bluffchenㄦ disease chain変change锷℃鍦ㄦ墽琛屻€?
**锣冧緥締ｇ爜锛?*

```c
bool paused = xwork_task_graph_is_paused(graph);
```

**What is the API?*

- `xwork_task_graph_pause`
- `xwork_task_graph_resume`

---

## Handoff

### xwork_handoff_request_options_init

What are the options for handoff request options?
**锷绻兘锛?*

鍑嗗涓€涓?handoff 璇风眰锛倀敤浜庡湪涓や鉜 task node 涔嬮棿浼抻€掍笂涓嬫枃抆?
**What's the point?*

```c
XWORK_API void xwork_handoff_request_options_init(
    xwork_handoff_request_options *pOptions
);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰嶈祫婧愶绂request API 浼橩鍒涶涷佷皣鐣欑殑瀛楃涓insert拋鏁 to play the role of 粍銆?
**Chen ュ Pang Xuan cun 槑?*

乇呴　璁剧江 handoff id銆乫rom task id鍜?to task id銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_handoff_request_options opts;
xwork_handoff_request_options_init(&opts);
opts.sHandoffId = "h1";
opts.sFromTaskId = "implement";
opts.sToTaskId = "review";
```

**What is the API?*

- `xwork_task_graph_request_handoff`

---

### xwork_handoff_result_options_init

What is the result of handoff result options?
**锷绻兘锛?*

卑嗗 handoff 澶勭恊缁洴灉锛倀敤浜庢帴鍙椤€佹嫆缁濇倨瀹屾垚 handoff銆?
**What's the point?*

```c
XWORK_API void xwork_handoff_result_options_init(
    xwork_handoff_result_options *pOptions
);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Resolve API
**Chen ュ Pang Xuan cun 槑?*

`eState` What is the value of `XWORK_HANDOFF_PENDING`?
**锣冧緥締ｇ爜锛?*

```c
xwork_handoff_result_options opts;
xwork_handoff_result_options_init(&opts);
opts.sHandoffId = "h1";
opts.eState = XWORK_HANDOFF_ACCEPTED;
```

**What is the API?*

- `xwork_task_graph_resolve_handoff`

---

### xwork_handoff_summary_init

What is the handoff summary?
**锷绻兘锛?*

鍑嗗涓€涓?handoff summary锛叀敤浜庢帴退逬姹幛垨镆ヨ缁撴灉銆?
**What's the point?*

```c
XWORK_API void xwork_handoff_summary_init(xwork_handoff_summary *pSummary);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

樇樿中闆€佷negative pending 璇箟銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_handoff_summary summary;
xwork_handoff_summary_init(&summary);
```

**What is the API?*

- `xwork_task_graph_request_handoff`

---

### xwork_handoff_summary_reset

Read the handoff summary?
**锷绻兘锛?*

译婃斁 handoff summary 涓殑瀛楃涓INSert拰liaokuang椤鏁狠粍銆?
**What's the point?*

```c
XWORK_API void xwork_handoff_summary_reset(xwork_handoff_summary *pSummary);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁 summary 鍍呴儴鎷ユ湁璧勬簮甛屼笉Read僃斁缁撴瀯钴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

artifact refs 鈆乵emory refs 鍜?workspace ids 閮箜寜瀛楃涓fork暟缁偯兯倀鏀Ju€?
**锣冧緥締ｇ爜锛?*

```c
xwork_handoff_summary_reset(&summary);
```

**What is the API?*

- `xwork_handoff_summary_init`

---

### xwork_handoff_summary_list_init

鍒濆鍖?handoff summary 鍒楄〃銆?
**锷绻兘锛?*

鍑嗗涓€渓┖鍒楄〃锛倀椤浜庢崴鏀?graph涓卍chain?handoff銆?
**What's the point?*

```c
XWORK_API void xwork_handoff_summary_list_init(xwork_handoff_summary_list *pList);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

咋卂椤 `xwork_task_graph_list_handoffs` 铓嶅簲鍒捒濆鍖栥€?
**锣冧緥締ｇ爜锛?*

```c
xwork_handoff_summary_list list;
xwork_handoff_summary_list_init(&list);
```

**What is the API?*

- `xwork_task_graph_list_handoffs`

---

### xwork_handoff_summary_list_reset

Yue僃恁 handoff summary 鍒楄〃銆?
**锷绻兘锛?*

Read婃斁鍒楄〃涓卍chain?handoff summary鍜屽垪曛ㄦ暟缁勩€?
**What's the point?*

```c
XWORK_API void xwork_handoff_summary_list_reset(xwork_handoff_summary_list *pList);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁鍒楄〃鎷ユ湁璧勬簮锛屼笉Read僃斁鍒楄〃缁撴瀯寒洴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

Read more
**锣冧緥締ｇ爜锛?*

```c
xwork_handoff_summary_list_reset(&list);
```

**What is the API?*

- `xwork_handoff_summary_reset`

---

### xwork_task_graph_request_handoff

鍒涘狠 handoff 璇风簰銆?
**锷绻兘锛?*

鍦ㄤ袱涓?task node 涔嬮棿璁 Board綍涓€涓?pending handoff锛屽苟鍙梼宁?artifact銆乵emory context鍜屽Ruibang?workspace 寮uku椤銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_request_handoff(
    xwork_task_graph *pGraph,
    const xwork_handoff_request_options *pOptions,
    xwork_handoff_summary *pSummary
);
```

**卙四暟锛?*

- XWORKPLACEHOLDER0 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

graph graph
**Chen ュ Pang Xuan cun 槑?*

from/to task 乇呴　瀛华湪曰樨andoff id 搴濿湪 graph 鍐呭殮涓€銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_handoff_summary summary;
xwork_handoff_summary_init(&summary);
xwork_task_graph_request_handoff(graph, &opts, &summary);
xwork_handoff_summary_reset(&summary);
```

**What is the API?*

- `xwork_task_graph_resolve_handoff`
- `xwork_task_graph_list_handoffs`

---

### xwork_task_graph_resolve_handoff

What's the point of handoff?
**锷绻兘锛?*

Xuan Cun 銊鮸湁 handoff 鄄勭姸镕和€佺姸镐人爜鍜屾秷鎭€?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_resolve_handoff(
    xwork_task_graph *pGraph,
    const xwork_handoff_result_options *pOptions,
    xwork_handoff_summary *pSummary
);
```

**卙四暟锛?*

- `pSummary`?summary?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

graph 澶嶅埗 message锛泂ummary 濡坝濉平曰倀敱咋卂敤Key?reset銆?
**Chen ュ Pang Xuan cun 槑?*

`eState`涓嶈嘘奇濇寔涓?`XWORK_HANDOFF_PENDING`銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_handoff_result_options result;
xwork_handoff_result_options_init(&result);
result.sHandoffId = "h1";
result.eState = XWORK_HANDOFF_COMPLETED;
xwork_task_graph_resolve_handoff(graph, &result, NULL);
```

**What is the API?*

- `xwork_task_graph_request_handoff`

---

### xwork_task_graph_list_handoffs

鍒楀吭 graph涓殑 handoff銆?
**锷绻兘锛?*

銮峰彇彰濿堠 task graph 鮸茶褰kuang殑 handoff summary 鍒楄〃銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_list_handoffs(
    const xwork_task_graph *pGraph,
    xwork_handoff_summary_list *pList
);
```

**卙四暟锛?*

- `pGraph`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

鍒楄〃鎷ユ湁 deep-copy 鍍呭锛倀椤
**Chen ュ Pang Xuan cun 槑?*

璇?API 鍙捤浜?UI 鏄 dramaず pending handoff 鴴栨丮澶嶅悗 READ嶅综合涓娄笅鏂囧叧绯簯氯€?
**锣冧緥締ｇ爜锛?*

```c
xwork_handoff_summary_list list;
xwork_handoff_summary_list_init(&list);
xwork_task_graph_list_handoffs(graph, &list);
xwork_handoff_summary_list_reset(&list);
```

**What is the API?*

- `xwork_task_graph_request_handoff`

---

## 鎶ュ憡涓庤仛钖?Artifact

### xwork_task_graph_emit_agent_result_report

Agent task 鐢熸垚缁撴灉玶ュ憡 artifact銆?
**锷绻兘锛?*

掶婃寚瀹?task 镄?child run 缁撴灉鍍椤叆璋卂椤鏂guigui彁渚涚殑 artifact 瀵 silicon thin銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_emit_agent_result_report(
    const xwork_task_graph *pGraph,
    const char *sTaskId,
    const char *sArtifactId,
    xwork_artifact *pArtifact
);
```

**卙四暟锛?*

- `pGraph` graph graph- `pArtifact` How to use artifact API?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

Artifact API reset?
**Chen ュ Pang Xuan cun 槑?*

The task is the run event.
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact artifact;
xwork_artifact_init(&artifact);
xwork_task_graph_emit_agent_result_report(graph, "review", "review-report", &artifact);
xwork_artifact_reset(&artifact);
```

**What is the API?*

- `xwork_task_graph_emit_aggregate_report`
- `xwork_artifact_reset`

---

### xwork_task_graph_emit_aggregate_report

What is the artifact?
**锷绻兘锛?*

掶婃暣涓?task graph 镄勬墽琛粀粨鋋滆仛钖韚埌鎸囧畾 run 镄?artifact涓€?
**What's the point?*

```c
XWORK_API xwork_status xwork_task_graph_emit_aggregate_report(
    const xwork_task_graph *pGraph,
    xwork_run *pRun,
    const char *sArtifactId,
    xwork_artifact *pArtifact
);
```

**卙四暟锛?*

- `pArtifact`?artifact?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

artifact 鍍呭鐢颭緭鍑 coaxPU℃寔chain庛纴璋卤椤Key呮寜 artifact API reset锛码un 涓嶈玺ョ銆?
**Chen ュ Pang Xuan cun 槑?*

What is the user interface of the UI?pipeline?
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact artifact;
xwork_artifact_init(&artifact);
xwork_task_graph_emit_aggregate_report(graph, run, "graph-report", &artifact);
xwork_artifact_reset(&artifact);
```

**What is the API?*

- `xwork_task_graph_emit_agent_result_report`
- `xwork_run`

---

## The manuscript is 叧鏂囨.

- [Run API](api-run.md)
- [Artifact API](api-artifacts.md)
- [澶?Agent 浠诲姟鍥綸(../guide/multi-agent-intro.md)
- [澶?Agent claw 鑼冧緥](../case/multi-agent-claw.md)
- [鍐呴儴 multi-agent contract](../../dev/docs/MULTI_AGENT.md)
