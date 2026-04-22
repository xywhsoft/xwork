# Persistence API

> Zhong Ruo €侊 fine Juan 枃 阬嚱鏁鏁 Board 嬬Key 溴纴寰呬Hanchen ュ阒呫€?
The Persistence API has a runtime function and a heckpoint function and a rtifact and agent pool and a request graph and an emote control plane and a replay cassette and a durable backend function.
##妯″潡瀹hydrogen綅

Persistence, xwork, durable agent run, file backend涓嶆槸鍒嗗嫷寮忔暟鎹簱锛屼篃涶嶆槸澶氩开Key呭瓨鍌绂濡悛灉awn€簽turn绋?DB `xwork_persistence_backend`?`xwork_persistence_backend`?
## chain

| 绫淲埆 | 澹典槑 |
| --- | --- |
| `xwork_persistence_backend`, `xwork_file_persistence_options`, `xwork_file_persistence` |
| `### xwork_*` `### xwork_*` |

## 镙敕笗嗟湰

褰揿堠`XWORK_PERSISTENCE_FORMAT_VERSION`涓?`14`抆?
What's the point?
- 褰揿堠洴栧Fan鏀寔鄄勬棫鐗徟湰锛氭寜鍏鈥稿Chain fried 鞞枞姐€?- `XWORK_ERROR_UNSUPPORTED`?-`XWORK_ERROR_EXTERNAL_FAILURE`?- What is the value of `XWORK_ERROR_NOT_FOUND`?
## gallium€chain勋戋戁诺勫寯

- runtime options `pPersistenceBackend` `pPersistenceBackend` `pPersistenceBackend` backend callback鐢熷懡 forge ㄦ湡銆?- Gallium € chain?list/load/query 枈揿叭缁洯鐢铟鐢ㄦ南鎷ユ湁锛涘～鍏呭怗鹇呴　璋卂椤瀵rose簲 reset銆?- recover API杩斿洖鄄?pool/graph/control plane/run涓?owned object锛岃皟鐢ㄦ南璐绻 chu阌€姣和€?
## Ning Ge?
- `XWORK_ERROR_NOT_FOUND` `XWORK_ERROR_NO_MEMORY`, deep-copy, backend callback, `XWORK_ERROR_EXTERNAL_FAILURE`, I/O backend callback.
## 阃氱敤锣姧緥

```c
#include "xwork.h"

int configure_store(void) {
    xwork_file_persistence_options options;
    xwork_file_persistence store;
    xwork_persistence_backend backend;

    xwork_file_persistence_options_init(&options);
    xwork_file_persistence_init(&store);
    xwork_persistence_backend_init(&backend);

    options.sRootPath = ".xwork_store";
    if (xwork_file_persistence_configure_backend(&store, &options, &backend) != XWORK_OK) {
        return 1;
    }

    xwork_file_persistence_reset(&store);
    return 0;
}
```

---

### xwork_persistence_backend_init

鍒濆鍖?persistence backend callback 琛ㄣ€?
**锷绻兘锛?*

鍒涘缓鑷畾涔?backend 鎴栨帴鏀?file backend 閰嶇疆鍓嶏紝灏?callback 琛ㄦ竻闆躲€?
**What's the point?*

```c
XWORK_API void xwork_persistence_backend_init(xwork_persistence_backend *pBackend);
```

**卙四暟锛?*

- `pBackend` is not the same as `NULL` is `NULL` is it?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

`pUserData` `pUserData`
**Chen ュ Pang Xuan cun 槑?*

- runtime 浼氭寜chain fried鍒?backend 琛ㄣ€?
**锣冧緥締ｇ爜锛?*

```c
xwork_persistence_backend backend;
xwork_persistence_backend_init(&backend);
```

**What is the API?*

- `xwork_file_persistence_configure_backend`

---

### xwork_file_persistence_options_init

What are the file persistence options?
**锷绻兘锛?*

What is the file backend?
**What's the point?*

```c
XWORK_API void xwork_file_persistence_options_init(
    xwork_file_persistence_options *pOptions
);
```

**卙四暟锛?*

- `pOptions` is not the same as `NULL` is `NULL` is it?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

The root path is the root path of the root path.
**Chen ュ Pang Xuan cun 槑?*

- `sRootPath` file backend file backend
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_options options;
xwork_file_persistence_options_init(&options);
options.sRootPath = ".xwork_store";
```

**What is the API?*

- `xwork_file_persistence_configure_backend`

---

### xwork_file_persistence_init

What is the file persistence store?
**锷绻兘锛?*

`xwork_file_persistence` file backend
**What's the point?*

```c
XWORK_API void xwork_file_persistence_init(xwork_file_persistence *pStore);
```

**卙四暟锛?*

- `pStore` is not the same as `NULL` is `NULL` is it?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

鍑簟涓嶅垎閰制祫婧橩€俢onfigure钖?store鎷ユ湁root path铓湰銆?
**Chen ュ Pang Xuan cun 槑?*

- `xwork_file_persistence_reset`?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence store;
xwork_file_persistence_init(&store);
```

**What is the API?*

- `xwork_file_persistence_reset`

---

### xwork_file_persistence_reset

Read the file persistence store?
**锷绻兘锛?*

File backend store file backend store root path init init Zhongduo€?
**What's the point?*

```c
XWORK_API void xwork_file_persistence_reset(xwork_file_persistence *pStore);
```

**卙四暟锛?*

- `pStore`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Yue婃斁 store 鎷ユ湁镄?root path铓湰銆?
**Chen ュ Pang Xuan cun 槑?*

- 涓氪 fine鍒犋掎纾亴涓婄殑 persistence 鏁版偁銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_reset(&store);
```

**What is the API?*

- `xwork_file_persistence_init`

---

### xwork_file_persistence_configure_backend

file backend?
**锷绻兘锛?*

掶?file store 缁戝畾鍒?root path锛屽苟濉Pang
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_configure_backend(
    xwork_file_persistence *pStore,
    const xwork_file_persistence_options *pOptions,
    xwork_persistence_backend *pBackend
);
```

**卙四暟锛?*

– `pStore` `pOptions` `NULL` `sRootPath` `sRootPath` `pBackend`锛氲緷鍑鍑鬍雳雁雁 effect退椤椤氪
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

store 鎷ユ湁 root path 铓湰锛暚ackend callback 琛ㄥ€熺昤 store 毻怀negative user data锛宻tore 鹇呴　窙洊 runtime 雛囥€熺昤?
**Chen ュ Pang Xuan cun 槑?*

- 卑 must be used as a guide to reset store/backend
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_configure_backend(&store, &options, &backend);
```

**What is the API?*

- `xwork_runtime_create`

---

### xwork_file_persistence_list_runs

Run id?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_list_runs(
    const xwork_file_persistence *pStore,
    xwork_string_list *pList
);
```

**锷绻兘锛?*

铓弿 file backend涓fanqi濆瓨鄄?run銆?
**卙四暟锛?*

- `pStore` - `pList` - `pList` - `pList` `NULL`四屾帴馀?owned
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

咋卂敤Key呭繀椤捤椤
**Chen ュ Pang Xuan cun 槑?*

- 杩斿洖镄勬槧 run id锛屼鬉锷纺水 run 鍐呭銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_string_list list;
xwork_string_list_init(&list);
xwork_file_persistence_list_runs(&store, &list);
xwork_string_list_reset(&list);
```

**What is the API?*

- `xwork_runtime_list_persisted_runs`

---

### xwork_file_persistence_list_run_summaries

Run summary?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_list_run_summaries(
    const xwork_file_persistence *pStore,
    xwork_run_summary_list *pList
);
```

**锷绻兘锛?*

file backend
**卙四暟锛?*

- `pStore` is the only one that can be used for wedding purposes? - `pList`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

`xwork_run_summary_list_reset`?
**Chen ュ Pang Xuan cun 槑?*

- 鐢ㄤ簬 run history UI?
**锣冧緥締ｇ爜锛?*

```c
xwork_run_summary_list list;
xwork_run_summary_list_init(&list);
xwork_file_persistence_list_run_summaries(&store, &list);
xwork_run_summary_list_reset(&list);
```

**What is the API?*

- `xwork_runtime_list_persisted_run_summaries`

---

### xwork_file_persistence_list_run_index

How can I run index?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_list_run_index(
    const xwork_file_persistence *pStore,
    xwork_run_index_list *pList
);
```

**锷绻兘锛?*

杩濖洖鍖呭戈 summary涓?last objects鄄?run index銆?
**卙四暟锛?*

- `pStore` is the only one that can be used for wedding purposes? - `pList`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

`xwork_run_index_list_reset`?
**Chen ュ Pang Xuan cun 槑?*

- 江変环浜庢椤 query 鏉′Huan鄄?run index 镆ヨ銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_run_index_list list;
xwork_run_index_list_init(&list);
xwork_file_persistence_list_run_index(&store, &list);
xwork_run_index_list_reset(&list);
```

**What is the API?*

- `xwork_file_persistence_query_run_index`

---

### xwork_file_persistence_query_run_index

Run index?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_query_run_index(
    const xwork_file_persistence *pStore,
    const xwork_run_index_query *pQuery,
    xwork_run_index_list *pList
);
```

**锷绻兘锛?*

鎸?run state銆乤utonomy銆鹴ast event/checkpoint/approval黛夋浔浠浠酠煇璇?run index銆?
**卙四暟锛?*

- `pStore` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

Reset
**Chen ュ Pang Xuan cun 槑?*

- query 瀛楃涓管瓧娈典negative borrowed銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_run_index_query query;
xwork_run_index_query_init(&query);
```

**What is the API?*

- `xwork_run_index_query_init`

---

### xwork_file_persistence_list_checkpoints

Checkpoint id run
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_list_checkpoints(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_string_list *pList
);
```

**锷绻兘锛?*

Checkpoint run
**卙四暟锛?*

- `pStore` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

`xwork_string_list_reset`?
**Chen ュ Pang Xuan cun 槑?*

- What is the checkpoint?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_list_checkpoints(&store, "run-1", &list);
```

**What is the API?*

- `xwork_file_persistence_load_checkpoint`

---

### xwork_file_persistence_list_events

鍒楀吭 run 镄?event 銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_list_events(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_string_list *pList
);
```

**锷绻兘锛?*

鍒楀嚭厸囧畾 run 镄?event log涓彲锷銺水 event id銆?
**卙四暟锛?*

- `pStore` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

咋卂敤Key?reset卒楄〃銆?
**Chen ュ Pang Xuan cun 槑?*

- 鐢ㄤ簬瀹¤铡嗗彶姆嶅巻銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_list_events(&store, "run-1", &list);
```

**What is the API?*

- `xwork_file_persistence_load_event`

---

### xwork_file_persistence_list_artifacts

鍒楀吭 run 镄?artifact id銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_list_artifacts(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_string_list *pList
);
```

**锷绻兘锛?*

卒楀嚭厸囧畾 run 宸叀卛瀛?artifact銆?
**卙四暟锛?*

- `pStore` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

咋卂敤Key?reset卒楄〃銆?
**Chen ュ Pang Xuan cun 槑?*

- What is the artifact?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_list_artifacts(&store, "run-1", &list);
```

**What is the API?*

- `xwork_file_persistence_load_artifact`

---

### xwork_file_persistence_list_artifact_summaries

鍒楀吭 run 镄?artifact summary銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_list_artifact_summaries(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_artifact_summary_list *pList
);
```

**锷绻兘锛?*

锷纺irrigate鎸囧畾 run 锄?artifact metadata鍒楄〃銆?
**卙四暟锛?*

- `pStore` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

`xwork_artifact_summary_list_reset`?
**Chen ュ Pang Xuan cun 槑?*

- 涓嶅姞枞袁珁?content text銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_list_artifact_summaries(&store, "run-1", &summaries);
```

**What is the API?*

- `xwork_file_persistence_query_artifact_summaries`

---

### xwork_file_persistence_query_artifact_summaries

Artifact summary銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_query_artifact_summaries(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const xwork_artifact_summary_query *pQuery,
    xwork_artifact_summary_list *pList
);
```

**锷绻兘锛?*

鸸?kind銆乷utput class銆乺ole銆丶ame銆丮IME銆乻torage ref銆鹪xit code鍜?sequence镆ヨ artifact summary銆?
**卙四暟锛?*

- `pStore` `pQuery`?`NULL`?-`pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

咋卂敤Key?reset枈揿嚭鍒楄〃銆?
**Chen ュ Pang Xuan cun 槑?*

- 绌?query 绛夊搓浜?list summaries銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact_summary_query query;
xwork_artifact_summary_query_init(&query);
```

**What is the API?*

- `xwork_artifact_summary_query_init`

---

### xwork_file_persistence_query_run_steps

镆ヨ鎸丷箙鍖?run step銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_query_run_steps(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const xwork_run_step_query *pQuery,
    xwork_run_step_list *pList
);
```

**锷绻兘锛?*

浠庢寔涔呭寲 event/checkpoint 涓擓鎴?step 鍒楄〃銆?
**卙四暟锛?*

- `pStore` `pQuery`?`NULL`?-`pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

`xwork_run_step_list_reset`?
**Chen ュ Pang Xuan cun 槑?*

- step event/checkpoint event/checkpoint
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_query_run_steps(&store, "run-1", NULL, &steps);
```

**What is the API?*

- `xwork_run_step_query_init`

---

### xwork_file_persistence_load_event

What is the event?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_event(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const char *sEventId,
    xwork_event *pEvent
);
```

**锷绻兘锛?*

浠?file backend 锷纺水鎸囧畾 run 鄄勬寚瀹?event銆?
**卙四暟锛?*

- `pStore` XWORKPLACEHOLDER2 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

event 鎺ユ湕 owned 瀛楁曃它皟鐢ㄨ€?reset銆?
**Chen ュ Pang Xuan cun 槑?*

- init?
**锣冧緥締ｇ爜锛?*

```c
xwork_event event;
xwork_event_init(&event);
xwork_file_persistence_load_event(&store, "run-1", "event-1", &event);
xwork_event_reset(&event);
```

**What is the API?*

- `xwork_file_persistence_list_events`

---

### xwork_file_persistence_load_last_event

What is the event?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_last_event(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_event *pEvent
);
```

**锷绻兘锛?*

锷纺irrigate鸧畾 run 锄勬涶钖庺竴涓?event銆?
**卙四暟锛?*

- `pStore` `pEvent`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the reset event key?
**Chen ュ Pang Xuan cun 槑?*

- 娌℃湁 event 锞 inert鹑锲?`XWORK_ERROR_NOT_FOUND`銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_last_event(&store, "run-1", &event);
```

**What is the API?*

- `xwork_runtime_load_persisted_last_event`

---

### xwork_file_persistence_load_run_snapshot

What is the latest run snapshot?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_run_snapshot(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_run_snapshot *pSnapshot
);
```

**锷绻兘锛?*

锷纺irrigate鎸囧畾 run 锄?latest snapshot锛妀敤浜庺仮澶?run銆?
**卙四暟锛?*

- `pStore` `pSnapshot`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

snapshot 鎺ユ湕 owned 瀛楁曃它皟鐢ㄨ€?reset銆?
**Chen ュ Pang Xuan cun 槑?*

- What is the workspace/tool/xllm/host service?
**锣冧緥締ｇ爜锛?*

```c
xwork_run_snapshot snapshot;
xwork_run_snapshot_init(&snapshot);
xwork_file_persistence_load_run_snapshot(&store, "run-1", &snapshot);
xwork_run_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_runtime_recover_run`

---

### xwork_file_persistence_load_checkpoint_snapshot

锷纺irrigation checkpoint 瀵gui粲 run snapshot銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_checkpoint_snapshot(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const char *sCheckpointId,
    xwork_run_snapshot *pSnapshot
);
```

**锷绻兘锛?*

Checkpoint
**卙四暟锛?*

- `pStore` XWORKPLACEHOLDER2 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset snapshot?
**Chen ュ Pang Xuan cun 槑?*

- 鐢ㄤ簬浠庡巻鍙?checkpoint 鎭㈠銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_checkpoint_snapshot(&store, "run-1", "ckpt-1", &snapshot);
```

**What is the API?*

- `xwork_file_persistence_load_run_snapshot`

---

### xwork_file_persistence_store_task_graph_snapshot

What is the task graph snapshot?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_store_task_graph_snapshot(
    const xwork_file_persistence *pStore,
    const xwork_task_graph_snapshot *pSnapshot
);
```

**锷绻兘锛?*

What is the multi-agent task graph and the file backend?
**卙四暟锛?*

– `pStore`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

鍑 must隟璇燲彇 snapshot锛屼鬉玺ョ铓€chain夋潈銆?
**Chen ュ Pang Xuan cun 槑?*

- What is the snapshot id link?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_store_task_graph_snapshot(&store, &snapshot);
```

**What is the API?*

- `xwork_file_persistence_load_task_graph_snapshot`

---

### xwork_file_persistence_load_task_graph_snapshot

What is the task graph snapshot?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_task_graph_snapshot(
    const xwork_file_persistence *pStore,
    const char *sGraphId,
    xwork_task_graph_snapshot *pSnapshot
);
```

**锷绻兘锛?*

锷纺水鎸囧畾task graph 锄勬崔涔呭寲 Zhongruo€and€?
**卙四暟锛?*

- `pStore` `pSnapshot`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset snapshot?
**Chen ュ Pang Xuan cun 槑?*

- What is the agent pool and runtime?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_task_graph_snapshot(&store, "graph-1", &snapshot);
```

**What is the API?*

- `xwork_file_persistence_recover_task_graph`

---

### xwork_file_persistence_store_agent_pool_snapshot

Qi Chuan agent pool snapshot?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_store_agent_pool_snapshot(
    const xwork_file_persistence *pStore,
    const xwork_agent_pool_snapshot *pSnapshot
);
```

**锷绻兘锛?*

Agent pool Agent snapshot agent pool
**卙四暟锛?*

– `pStore`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

鍑 must隟璇燲彇 snapshot锛屼鬉玺ョ铓€chain夋潈銆?
**Chen ュ Pang Xuan cun 槑?*

- 鐢ㄤ簬 multi-agent 鎭㈠銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_store_agent_pool_snapshot(&store, &pool_snapshot);
```

**What is the API?*

- `xwork_file_persistence_load_agent_pool_snapshot`

---

### xwork_file_persistence_load_agent_pool_snapshot

What is the agent pool snapshot?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_agent_pool_snapshot(
    const xwork_file_persistence *pStore,
    const char *sPoolId,
    xwork_agent_pool_snapshot *pSnapshot
);
```

**锷绻兘锛?*

What is the agent pool?
**卙四暟锛?*

- `pStore` `pSnapshot`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset snapshot?
**Chen ュ Pang Xuan cun 槑?*

- load the live pool?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_agent_pool_snapshot(&store, "pool-1", &snapshot);
```

**What is the API?*

- `xwork_agent_pool_create_from_snapshot`

---

### xwork_file_persistence_store_control_plane_snapshot

What is the control plane snapshot?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_store_control_plane_snapshot(
    const xwork_file_persistence *pStore,
    const xwork_control_plane_snapshot *pSnapshot
);
```

**锷绻兘锛?*

Qiciquan remote worker control plane Zhongduo€?
**卙四暟锛?*

– `pStore`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

鍑 must隟璇燲彇 snapshot锛屼鬉玺ョ铓€chain夋潈銆?
**Chen ュ Pang Xuan cun 槑?*

- 鍖呭惈worker銆乼ask銆乴ease銆乷utput/blob chunk 鎽樿銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_store_control_plane_snapshot(&store, &snapshot);
```

**What is the API?*

- `xwork_file_persistence_load_control_plane_snapshot`

---

### xwork_file_persistence_load_control_plane_snapshot

What is the control plane snapshot?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_control_plane_snapshot(
    const xwork_file_persistence *pStore,
    const char *sPlaneId,
    xwork_control_plane_snapshot *pSnapshot
);
```

**锷绻兘锛?*

The remote worker control plane is connected to the remote worker control plane.
**卙四暟锛?*

- `pStore` `pSnapshot`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset snapshot?
**Chen ュ Pang Xuan cun 槑?*

- Load the control plane and the snapshot control plane.
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_control_plane_snapshot(&store, "plane-1", &snapshot);
```

**What is the API?*

- `xwork_file_persistence_recover_control_plane`

---

### xwork_file_persistence_store_replay

Qiciquan replay engine cassette?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_store_replay(
    const xwork_file_persistence *pStore,
    const xwork_replay_engine *pEngine
);
```

**锷绻兘锛?*

Replay manifest, intries, vents, filesystem refs, results, etc.
**卙四暟锛?*

– `pStore`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What's the value of the replay engine?
**Chen ュ Pang Xuan cun 槑?*

- replay engine?Replay API?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_store_replay(&store, engine);
```

**What is the API?*

- `xwork_file_persistence_load_replay_engine`

---

### xwork_file_persistence_list_replays

鍒楀吭 replay id銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_list_replays(
    const xwork_file_persistence *pStore,
    xwork_string_list *pList
);
```

**锷绻兘锛?*

铓弿 file backend涓fanqi濆瓨鄄?replay cassette銆?
**卙四暟锛?*

- `pStore` is the only one that can be used for wedding purposes? - `pList`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

璋卂敤Key?reset 瀛怃涓综合枪曛ㄣ€?
**Chen ュ Pang Xuan cun 槑?*

- Replay id?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_list_replays(&store, &list);
```

**What is the API?*

- `xwork_file_persistence_load_replay_manifest`

---

### xwork_file_persistence_load_replay_manifest

Why replay manifest?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_replay_manifest(
    const xwork_file_persistence *pStore,
    const char *sReplayId,
    xwork_replay_manifest *pManifest
);
```

**锷绻兘锛?*

锷纺殹 replay 锄?manifest 鍏冩暟酹€?
**卙四暟锛?*

- `pStore` `pManifest`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset manifest?
**Chen ュ Pang Xuan cun 槑?*

- 涓嶅姞杞?entry 鍒楄〃銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_replay_manifest(&store, "replay-1", &manifest);
```

**What is the API?*

- `xwork_replay_manifest_reset`

---

### xwork_file_persistence_load_replay_entries

Replay entry summaries銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_replay_entries(
    const xwork_file_persistence *pStore,
    const char *sReplayId,
    xwork_replay_entry_summary_list *pList
);
```

**锷绻兘锛?*

锷纺irrigate replay cassette 锄?entry summary 鍒楄〃銆?
**卙四暟锛?*

- `pStore` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

Reset entry summary list?
**Chen ュ Pang Xuan cun 槑?*

- What is the summary of the payload?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_replay_entries(&store, "replay-1", &entries);
```

**What is the API?*

- `xwork_replay_entry_summary_list_reset`

---

### xwork_file_persistence_load_replay_result

Replay result?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_replay_result(
    const xwork_file_persistence *pStore,
    const char *sReplayId,
    xwork_replay_result *pResult
);
```

**锷绻兘锛?*

锷纺irrigate replay 铓ц缁撴灴灉鍜岄Juan?divergence銆?
**卙四暟锛?*

- `pStore` `pResult`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset replay result?
**Chen ュ Pang Xuan cun 槑?*

- 鐢ㄤ簬 replay 铡嗗彶UI 鴴?CI gate銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_replay_result(&store, "replay-1", &result);
```

**What is the API?*

- `xwork_replay_result_reset`

---

### xwork_file_persistence_load_replay_engine

How about replay engine?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_replay_engine(
    const xwork_file_persistence *pStore,
    const char *sReplayId,
    const xwork_replay_options *pOptions,
    xwork_replay_engine **ppEngine
);
```

**锷绻兘锛?*

浠庝iao瀛樼殑 replay cassette 鋋勫狠 live replay engine銆?
**卙四暟锛?*

- `pStore` `pOptions` `ppEngine`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

`*ppEngine` `xwork_replay_engine_destroy` `xwork_replay_engine_destroy`
**Chen ュ Pang Xuan cun 槑?*

- live replay engine
**锣冧緥締ｇ爜锛?*

```c
xwork_replay_engine *engine = NULL;
xwork_file_persistence_load_replay_engine(&store, "replay-1", NULL, &engine);
xwork_replay_engine_destroy(engine);
```

**What is the API?*

- `xwork_file_persistence_store_replay`

---

### xwork_file_persistence_recover_task_graph

鍭㈠agent pool鍜?task graph銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_recover_task_graph(
    const xwork_file_persistence *pStore,
    xwork_runtime *pRuntime,
    const char *sPoolId,
    const char *sGraphId,
    const xwork_task_graph_options *pExecutionOptions,
    xwork_agent_pool **ppPool,
    xwork_task_graph **ppGraph
);
```

**锷绻兘锛?*

浠庢寔涔呭寲 agent pool snapshot 涓?task graph snapshot 鍒涘leu live 瀵 silicon thin 銆?
**卙四暟锛?*

-`pStore` `NULL`抆?- XWORKPLACEHOLDER 4TOKEN `ppPool`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence/multi-agent persistence/multi-agent ?
**璧勬簮褰掎睘锛?*

掴愬姛钖?pool鍜?graph 鐢锟鍢ㄨ€嫮嫢夛纴鍒嗗埆鐢ㄥ搴?destroy 鍑 must隟Read嫃斁銆?
**Chen ュ Pang Xuan cun 槑?*

- runtime installation and installation of workspace and workspace and service installation of xllm and workspace.
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_recover_task_graph(&store, runtime, "pool-1", "graph-1", NULL, &pool, &graph);
```

**What is the API?*

- `xwork_task_graph_destroy`
- `xwork_agent_pool_destroy`

---

### xwork_file_persistence_recover_control_plane

What is the remote control plane?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_recover_control_plane(
    const xwork_file_persistence *pStore,
    xwork_runtime *pRuntime,
    const char *sPlaneId,
    const xwork_control_plane_options *pOptions,
    xwork_control_plane **ppPlane
);
```

**锷绻兘锛?*

control plane snapshot live control plane
**卙四暟锛?*

-`pStore` `NULL`抆?- -
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence/remote ?
**璧勬簮褰掎睘锛?*

`*ppPlane` `xwork_control_plane_destroy` `xwork_control_plane_destroy`
**Chen ュ Pang Xuan cun 槑?*

- Worker
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_recover_control_plane(&store, runtime, "plane-1", NULL, &plane);
```

**What is the API?*

- `xwork_control_plane_destroy`

---

### xwork_file_persistence_load_last_approval_request

锷纺溴鈶?approval request銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_last_approval_request(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_approval_request *pRequest
);
```

**锷绻兘锛?*

璇氲彇鸸囧畾 run chain€钖庤褰ukuang殑瀹℃壒璇风簰銆?
**卙四暟锛?*

- `pStore` `pRequest`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset request?
**Chen ュ Pang Xuan cun 槑?*

- `XWORK_ERROR_NOT_FOUND`?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_last_approval_request(&store, "run-1", &request);
```

**What is the API?*

- `xwork_runtime_load_persisted_last_approval_request`

---

### xwork_file_persistence_load_run_summary

run summary?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_run_summary(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_run_summary *pSummary
);
```

**锷绻兘锛?*

蒇谲彇鸸囧畾run 镄?summary銆?
**卙四暟锛?*

- `pStore` `pSummary`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset summary?
**Chen ュ Pang Xuan cun 槑?*

- summary 阃effect掎鍒楄〃椤碉纴纓嶅set钖畲鏁?run snapshot銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_run_summary(&store, "run-1", &summary);
```

**What is the API?*

- `xwork_file_persistence_load_run_snapshot`

---

### xwork_file_persistence_load_checkpoint

Checkpoint?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_checkpoint(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const char *sCheckpointId,
    xwork_checkpoint *pCheckpoint
);
```

**锷绻兘锛?*

Checkpoint metadata?
**卙四暟锛?*

- `pStore` XWORKPLACEHOLDER2 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset checkpoint?
**Chen ュ Pang Xuan cun 槑?*

- Checkpoint snapshot? run state?checkpoint snapshot?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_checkpoint(&store, "run-1", "ckpt-1", &checkpoint);
```

**What is the API?*

- `xwork_file_persistence_load_checkpoint_snapshot`

---

### xwork_file_persistence_load_last_checkpoint

Checkpoint?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_last_checkpoint(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_checkpoint *pCheckpoint
);
```

**锷绻兘锛?*

Checkpoint checkpoint?
**卙四暟锛?*

- `pStore` `pCheckpoint`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset checkpoint?
**Chen ュ Pang Xuan cun 槑?*

- Checkpoint `XWORK_ERROR_NOT_FOUND`
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_last_checkpoint(&store, "run-1", &checkpoint);
```

**What is the API?*

- `xwork_runtime_load_persisted_last_checkpoint`

---

### xwork_file_persistence_load_artifact

What is the artifact?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_artifact(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const char *sArtifactId,
    xwork_artifact *pArtifact
);
```

**锷绻兘锛?*

璇氲彇鸸囧畾 run 鄄勬寚瀹?artifact銆?
**卙四暟锛?*

- `pStore` XWORKPLACEHOLDER2 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset artifact?
**Chen ュ Pang Xuan cun 槑?*

- content
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_artifact(&store, "run-1", "artifact-1", &artifact);
```

**What is the API?*

- `xwork_file_persistence_list_artifacts`

---

### xwork_file_persistence_load_last_artifact

锷纺溴�钖庺竴涓?artifact銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_load_last_artifact(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_artifact *pArtifact
);
```

**锷绻兘锛?*

蒇谲彇鸸囧畾 run chain€钖庤褰kuang殑 artifact銆?
**卙四暟锛?*

- `pStore` `pArtifact`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset artifact?
**Chen ュ Pang Xuan cun 槑?*

- Artifact `XWORK_ERROR_NOT_FOUND`
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_load_last_artifact(&store, "run-1", &artifact);
```

**What is the API?*

- `xwork_runtime_load_persisted_last_artifact`

---

### xwork_file_persistence_find_artifact_by_name

What is the artifact?
**What's the point?*

```c
XWORK_API xwork_status xwork_file_persistence_find_artifact_by_name(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const char *sArtifactName,
    xwork_artifact *pArtifact
);
```

**锷绻兘锛?*

鍦ㄦ寚瀹?run 鄄?artifact 涓寜 name 绮剧‘镆ユ媞骞姞枞 elder sister€?
**卙四暟锛?*

- `pStore` XWORKPLACEHOLDER2 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` persistence ?
**璧勬簮褰掎睘锛?*

What is the key?reset artifact?
**Chen ュ Pang Xuan cun 槑?*

- 涶hydrogen kettle钖屽敕 artifact 鞞 inert锲炲焄鐜板畾涔夌殑鍖Guili椤縸纴夤hong涓氩姟瞞综合鐢ㄥ殕涓€ name銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_file_persistence_find_artifact_by_name(&store, "run-1", "final.md", &artifact);
```

**What is the API?*

- `xwork_runtime_find_persisted_artifact_by_name`

---

## Runtime Facade

Runtime facade runtime facade runtime facade `xwork_persistence_backend` backend file backend file backend file backend file backend file backend file backend file backend
### xwork_runtime_list_persisted_runs

Run id?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_list_persisted_runs(
    const xwork_runtime *pRuntime,
    xwork_string_list *pList
);
```

**卙四暟锛?*

- XWORKPLACEHOLDER0 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- facade 涓嶅叧鰇?backend 鏄?file 杩樻槸镊畾涔夊焄鐜比 €?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_list_persisted_runs(runtime, &list);
```

**What is the API?*

- `xwork_file_persistence_list_runs`

---

### xwork_runtime_list_persisted_checkpoints

What is the checkpoint id?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_list_persisted_checkpoints(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_string_list *pList
);
```

**卙四暟锛?*

– `pRuntime` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- 浠呭垪鍑?id銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_list_persisted_checkpoints(runtime, "run-1", &list);
```

**What is the API?*

- `xwork_runtime_load_persisted_checkpoint`

---

### xwork_runtime_list_persisted_events

What is the event id?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_list_persisted_events(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_string_list *pList
);
```

**卙四暟锛?*

– `pRuntime` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- 浠呭垪鍑?id銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_list_persisted_events(runtime, "run-1", &list);
```

**What is the API?*

- `xwork_runtime_load_persisted_event`

---

### xwork_runtime_list_persisted_artifacts

Artifact id?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_list_persisted_artifacts(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_string_list *pList
);
```

**卙四暟锛?*

– `pRuntime` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- 浠呭垪鍑?id銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_list_persisted_artifacts(runtime, "run-1", &list);
```

**What is the API?*

- `xwork_runtime_load_persisted_artifact`

---

### xwork_runtime_list_persisted_artifact_summaries

What is the artifact summary?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_list_persisted_artifact_summaries(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_artifact_summary_list *pList
);
```

**卙四暟锛?*

– `pRuntime` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- What's the content?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_list_persisted_artifact_summaries(runtime, "run-1", &summaries);
```

**What is the API?*

- `xwork_runtime_query_persisted_artifact_summaries`

---

### xwork_runtime_query_persisted_artifact_summaries

What is the artifact summary?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_query_persisted_artifact_summaries(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    const xwork_artifact_summary_query *pQuery,
    xwork_artifact_summary_list *pList
);
```

**卙四暟锛?*

– `pRuntime` `pQuery`?`NULL`?-`pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- 捡四灉 backend 涓嶆彁渚涘师颢?query锛宺untime鍙洴阃€鍒?list 钖庤嘃狠ゃ€?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_query_persisted_artifact_summaries(runtime, "run-1", NULL, &summaries);
```

**What is the API?*

- `xwork_artifact_summary_query_init`

---

### xwork_runtime_query_persisted_run_steps

镆ヨ鎸丷箙鍖?run step銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_query_persisted_run_steps(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    const xwork_run_step_query *pQuery,
    xwork_run_step_list *pList
);
```

**卙四暟锛?*

– `pRuntime` `pQuery`?`NULL`?-`pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- 捡四灉 backend 涓嶆殮鸸丶师鐢?query锛宺untime鍙粠 event/checkpoint 娲剧敓銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_query_persisted_run_steps(runtime, "run-1", NULL, &steps);
```

**What is the API?*

- `xwork_run_step_query_init`

---

### xwork_runtime_list_persisted_run_summaries

Run summary銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_list_persisted_run_summaries(
    const xwork_runtime *pRuntime,
    xwork_run_summary_list *pList
);
```

**卙四暟锛?*

- `pRuntime`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- 阃肖掎铡嗗彶run 鍒楄〃 UI銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_list_persisted_run_summaries(runtime, &list);
```

**What is the API?*

- `xwork_runtime_list_persisted_run_index`

---

### xwork_runtime_list_persisted_run_index

Run index?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_list_persisted_run_index(
    const xwork_runtime *pRuntime,
    xwork_run_index_list *pList
);
```

**卙四暟锛?*

- `pRuntime`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- 江変环浜庢椤 query 鏉′Huan鄄?index 镆ヨ銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_list_persisted_run_index(runtime, &index);
```

**What is the API?*

- `xwork_runtime_query_persisted_run_index`

---

### xwork_runtime_query_persisted_run_index

镆ヨ鎸丷箙鍖?run index銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_query_persisted_run_index(
    const xwork_runtime *pRuntime,
    const xwork_run_index_query *pQuery,
    xwork_run_index_list *pList
);
```

**卙四暟锛?*

- `pRuntime` `pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset list?
**Chen ュ Pang Xuan cun 槑?*

- query 瀛楃涓综合瓧娈电敱咋卂椤鏂 graduate?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_query_persisted_run_index(runtime, NULL, &index);
```

**What is the API?*

- `xwork_run_index_query_init`

---

### xwork_runtime_load_persisted_run_summary

Run summary銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_load_persisted_run_summary(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_run_summary *pSummary
);
```

**卙四暟锛?*

– `pRuntime` `pSummary`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset summary?
**Chen ュ Pang Xuan cun 槑?*

- 涓嶆仮澶?live run抆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_load_persisted_run_summary(runtime, "run-1", &summary);
```

**What is the API?*

- `xwork_runtime_recover_run_from_persistence`

---

### xwork_runtime_load_persisted_last_event

What is the event?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_load_persisted_last_event(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_event *pEvent
);
```

**卙四暟锛?*

– `pRuntime` `pEvent`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the reset event key?
**Chen ュ Pang Xuan cun 槑?*

- 镞?event鞞 lazy鹑锲?not found銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_load_persisted_last_event(runtime, "run-1", &event);
```

**What is the API?*

- `xwork_runtime_load_persisted_event`

---

### xwork_runtime_load_persisted_last_approval_request

Is there an approval request?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_load_persisted_last_approval_request(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_approval_request *pRequest
);
```

**卙四暟锛?*

– `pRuntime` `pRequest`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset request?
**Chen ュ Pang Xuan cun 槑?*

- 鐢ㄤ簬鎭㈠瀹℃壒 UI銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_load_persisted_last_approval_request(runtime, "run-1", &request);
```

**What is the API?*

- `xwork_run_submit_approval`

---

### xwork_runtime_load_persisted_last_checkpoint

Checkpoint? Checkpoint?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_load_persisted_last_checkpoint(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_checkpoint *pCheckpoint
);
```

**卙四暟锛?*

– `pRuntime` `pCheckpoint`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset checkpoint?
**Chen ュ Pang Xuan cun 槑?*

- Checkpoint metadata?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_load_persisted_last_checkpoint(runtime, "run-1", &checkpoint);
```

**What is the API?*

- `xwork_runtime_load_persisted_checkpoint`

---

### xwork_runtime_load_persisted_last_artifact

What is the artifact?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_load_persisted_last_artifact(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_artifact *pArtifact
);
```

**卙四暟锛?*

– `pRuntime` `pArtifact`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset artifact?
**Chen ュ Pang Xuan cun 槑?*

- 锞?artifact锞 lazy鹑锲?not found銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_load_persisted_last_artifact(runtime, "run-1", &artifact);
```

**What is the API?*

- `xwork_runtime_load_persisted_artifact`

---

### xwork_runtime_load_persisted_event

What is the event?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_load_persisted_event(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    const char *sEventId,
    xwork_event *pEvent
);
```

**卙四暟锛?*

– `pRuntime` XWORKPLACEHOLDER2 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the reset event key?
**Chen ュ Pang Xuan cun 槑?*

- 鐢ㄤ簬瀹¤鍜?step 镆ヨ銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_load_persisted_event(runtime, "run-1", "event-1", &event);
```

**What is the API?*

- `xwork_runtime_list_persisted_events`

---

### xwork_runtime_load_persisted_checkpoint

Checkpoint? Checkpoint?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_load_persisted_checkpoint(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    const char *sCheckpointId,
    xwork_checkpoint *pCheckpoint
);
```

**卙四暟锛?*

– `pRuntime` XWORKPLACEHOLDER2 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset checkpoint?
**Chen ュ Pang Xuan cun 槑?*

- Checkpoint metadata?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_load_persisted_checkpoint(runtime, "run-1", "ckpt-1", &checkpoint);
```

**What is the API?*

- `xwork_runtime_load_persisted_last_checkpoint`

---

### xwork_runtime_load_persisted_artifact

What is the artifact?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_load_persisted_artifact(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    const char *sArtifactId,
    xwork_artifact *pArtifact
);
```

**卙四暟锛?*

– `pRuntime` XWORKPLACEHOLDER2 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset artifact?
**Chen ュ Pang Xuan cun 槑?*

- content 銄惁鍙敤鍙栧浅浜?backend銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_load_persisted_artifact(runtime, "run-1", "artifact-1", &artifact);
```

**What is the API?*

- `xwork_runtime_find_persisted_artifact_by_name`

---

### xwork_runtime_find_persisted_artifact_by_name

What is the artifact?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_find_persisted_artifact_by_name(
    const xwork_runtime *pRuntime,
    const char *sRunId,
    const char *sArtifactName,
    xwork_artifact *pArtifact
);
```

**卙四暟锛?*

– `pRuntime` XWORKPLACEHOLDER2 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK` backend
**璧勬簮褰掎睘锛?*

What is the key?reset artifact?
**Chen ュ Pang Xuan cun 槑?*

- 寤红涓氩姟璞备琴琇?artifact name鍞竴銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_find_persisted_artifact_by_name(runtime, "run-1", "final.md", &artifact);
```

**What is the API?*

- `xwork_runtime_load_persisted_artifact`

---

### xwork_runtime_recover_run

浠?run snapshot鎭㈠live run銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_recover_run(
    xwork_runtime *pRuntime,
    const xwork_run_snapshot *pSnapshot,
    xwork_run **ppRun
);
```

**卙四暟锛?*

– `pRuntime` `pSnapshot` `NULL`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`
**璧勬簮褰掎睘锛?*

鴴愬姛钖?run 锄勭潃鍒?runtime锛倱啋咋卹椤Key呮樉寮?destroy鴴?runtime destroy銆?
**Chen ュ Pang Xuan cun 槑?*

- Workspace/tool/xllm/host service?live process?callback callback?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_recover_run(runtime, &snapshot, &run);
```

**What is the API?*

- `xwork_file_persistence_load_run_snapshot`

---

### xwork_runtime_recover_run_from_persistence

浠?persistence latest snapshot 鎭㈠ live run銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_runtime_recover_run_from_persistence(
    xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_run **ppRun
);
```

**卙四暟锛?*

– `pRuntime` XWORKPLACEHOLDER1 TOKEN
**杩斿洴 alkali fine**

杩斿洖
**璧勬簮褰掎睘锛?*

Run time
**Chen ュ Pang Xuan cun 槑?*

- 杩欐槧锷纺水 latest snapshot 钖庤皟颢?
**锣冧緥締ｇ爜锛?*

```c
xwork_runtime_recover_run_from_persistence(runtime, "run-1", &run);
```

**What is the API?*

- `xwork_runtime_recover_run`

## 鎭㈠杈Guihu

The interaction between orkspace id, ending tool, pproval decision, ast checkpoint, rtifact metadata, gent/task/worker/replay snapshot_live OS process handler_interactive terminal session_live OS process handler_interactive terminal session_live OS process handler_interactive terminal session_live OS process handler
## 绾cross▼杈爈晫

File backend file backend file backend锄勋苟鍙戝开鍏ュ簢鐢鞟鐢ㄦ南涓茶鍖栥€俽untime facade 鄄勋苟鍙戣 actually鐣荼笌洴枞眰 backend 涓€镊欰€?
## The manuscript is 叧鏂囨.

- [Run API](api-run.md)
- [Artifact API](api-artifacts.md)
- [Replay API](api-replay.md)
- [鎸佷箙鍖栥€乧heckpoint 涓?replay](../guide/persistence-replay-intro.md)
- [鍐呴儴 persistence format](../../dev/docs/PERSISTENCE_FORMAT.md)
