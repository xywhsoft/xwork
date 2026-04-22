# Remote Worker API

Remote Worker API, control plane, orker registry, ease, assignment queue, emote task, rtifact blob chunk, output chunk, output chunk, and output chunk. Worker Galliumц锛屽铓铓锣椤彲瀹¤銆丸彲鎭㈠鄄勪锷锷＄姸镐 and €?
##妯″桡杈戈晫

- `XWORK_REMOTE_TRANSPORT_IN_PROCESS` blob streaming鐢卞涓毲焄鐜综合€?- control plane 涓嶆彃姝?OS 杩涚▼锛屼篃涓嶆仮澶?live terminal/process handle锛泂napshot锭㈠镞?assigned/running task Orphaned control plane - control plane worker worker mutation control unit tart/stop/register/heartbeat/enqueue/claim/complete/fail/cancel/upload/query `1`?`XWORK_REMOTE_PROTOCOL_VERSION_CURRENT`?
## gallium€chain夋戈绾﹀畾

| Silicon thin | Gallium chain |
| --- | --- |
|
|
| options/result/upload/chunk 枈揆叆 |
| summary/list/snapshot 枈揿吭 | 枈描缁洴瀯玷ユ恁 deep-copy 鍐呭锛屼婢ㄥ尮閰?
| assignment |

## 鏏毛瀷仙人▼

```text
xwork_control_plane_options_init
xwork_control_plane_create
xwork_control_plane_start
xwork_worker_options_init
xwork_control_plane_register_worker
xwork_remote_task_options_init
xwork_control_plane_enqueue_task
xwork_control_plane_claim_task
xwork_control_plane_complete_task
xwork_control_plane_get_snapshot
xwork_control_plane_destroy
```

## 鍒濆鍖栦笌笌笃惁API

### xwork_control_plane_options_init

What is the control plane options?
**锷绻兘锛?*

Control plane control plane
**What's the point?*

```c
XWORK_API void xwork_control_plane_options_init(xwork_control_plane_options *pOptions);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`sPlaneId`
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_options opts;
xwork_control_plane_options_init(&opts);
opts.sPlaneId = "plane-1";
opts.pRuntime = runtime;
```

**What is the API?*

- `xwork_control_plane_create`

---

### xwork_worker_options_init

What are the worker options?
**锷绻兘锛?*

卑嗗 worker 剉ㄥ唽卙四暟銆?
**What's the point?*

```c
XWORK_API void xwork_worker_options_init(xwork_worker_options *pOptions);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

The protocol version of the protocol version is the runtime. The plane runtime is the worker. The plane runtime is the worker.
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_options opts;
xwork_worker_options_init(&opts);
opts.sWorkerId = "worker-1";
opts.pRuntime = runtime;
```

**What is the API?*

- `xwork_control_plane_register_worker`

---

### xwork_worker_summary_init

What is the worker summary?
**锷绻兘锛?*

鍑嗗 worker 镆ヨ缁撴灉缁撴瀯銆?
**What's the point?*

```c
XWORK_API void xwork_worker_summary_init(xwork_worker_summary *pSummary);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`XWORK_WORKER_REGISTERED`?
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_summary summary;
xwork_worker_summary_init(&summary);
```

**What is the API?*

- `xwork_control_plane_list_workers`

---

### xwork_worker_summary_reset

Read the worker summary?
**锷绻兘锛?*

Read the worker id, the worker ID, the endpoint, the deep-copy file, and the worker ID.
**What's the point?*

```c
XWORK_API void xwork_worker_summary_reset(xwork_worker_summary *pSummary);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁捍呴儴璧勬簮锛屼笉Read僃斁缁撴瀯钴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

咋卂敤钖庢仮澶澶brandnegative init Zhong Ruo€and€?
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_summary_reset(&summary);
```

**What is the API?*

- `xwork_worker_summary_init`

---

### xwork_worker_summary_list_init

鍒濆鍖?worker summary 鍒楄〃銆?
**锷绻兘锛?*

Worker registry
**What's the point?*

```c
XWORK_API void xwork_worker_summary_list_init(xwork_worker_summary_list *pList);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

咋卂椤 `xwork_control_plane_list_workers` 铓嶅簲鍒捒濆鍖栥€?
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_summary_list list;
xwork_worker_summary_list_init(&list);
```

**What is the API?*

- `xwork_control_plane_list_workers`

---

### xwork_worker_summary_list_reset

荒婃恁 worker summary 鍒楄〃銆?
**锷绻兘锛?*

Read the chain?worker summary 鍜屾暟缁卩€?
**What's the point?*

```c
XWORK_API void xwork_worker_summary_list_reset(xwork_worker_summary_list *pList);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁鍒楄〃鎷ユ湁镄勫崴瀹广€?
**Chen ュ Pang Xuan cun 槑?*

Reading婃斁钖庡垪曛ㄥ彲澶敤抆?
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_summary_list_reset(&list);
```

**What is the API?*

- `xwork_worker_summary_reset`

---

### xwork_worker_snapshot_init

What is the worker snapshot?
**锷绻兘锛?*

What is the worker snapshot?worker registry?
**What's the point?*

```c
XWORK_API void xwork_worker_snapshot_init(xwork_worker_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

What is the protocol version of the protocol version?
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_snapshot snapshot;
xwork_worker_snapshot_init(&snapshot);
```

**What is the API?*

- `xwork_control_plane_get_snapshot`

---

### xwork_worker_snapshot_reset

Read the worker snapshot?
**锷绻兘锛?*

Read the worker snapshot.
**What's the point?*

```c
XWORK_API void xwork_worker_snapshot_reset(xwork_worker_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Yue僃斁鍐呴儴 deep-copy 璧勬簮銆?
**Chen ュ Pang Xuan cun 槑?*

`xwork_control_plane_create_from_snapshot`
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_worker_snapshot_init`

---

### xwork_worker_snapshot_list_init

鍒濆鍖?worker snapshot 鍒楄〃銆?
**锷绻兘锛?*

What is the worker snapshot list?
**What's the point?*

```c
XWORK_API void xwork_worker_snapshot_list_init(xwork_worker_snapshot_list *pList);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_control_plane_snapshot.tWorkers` `xwork_control_plane_snapshot.tWorkers`?
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_snapshot_list list;
xwork_worker_snapshot_list_init(&list);
```

**What is the API?*

- `xwork_worker_snapshot_list_reset`

---

### xwork_worker_snapshot_list_reset

Yue僃斁 worker snapshot 鍒楄〃銆?
**锷绻兘锛?*

Read this article
**What's the point?*

```c
XWORK_API void xwork_worker_snapshot_list_reset(xwork_worker_snapshot_list *pList);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁鍒楄〃鎷ユ湁镄勫崴瀹广€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_control_plane_snapshot_reset` 浼氶棿掺ヨ皟鐢ㄥ畠銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_snapshot_list_reset(&list);
```

**What is the API?*

- `xwork_control_plane_snapshot_reset`

---

### xwork_remote_task_options_init

What are the remote task options?
**锷绻兘锛?*

Remote task?
**What's the point?*

```c
XWORK_API void xwork_remote_task_options_init(xwork_remote_task_options *pOptions);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

Task kind task type host tool host service process process task id task id request JSON ost tool operation id?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_options opts;
xwork_remote_task_options_init(&opts);
opts.sTaskId = "task-1";
opts.sOperationId = XWORK_HOST_PROCESS_EXEC;
opts.sRequestJson = "{\"cmd\":\"echo hi\"}";
```

**What is the API?*

- `xwork_control_plane_enqueue_task`

---

### xwork_remote_task_summary_init

What is the function of remote task summary?
**锷绻兘锛?*

What is the remote task?
**What's the point?*

```c
XWORK_API void xwork_remote_task_summary_init(xwork_remote_task_summary *pSummary);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

The task is kind and the host tool is queued?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_summary summary;
xwork_remote_task_summary_init(&summary);
```

**What is the API?*

- `xwork_control_plane_get_task_summary`

---

### xwork_remote_task_summary_reset

Read the remote task summary?
**锷绻兘锛?*

Read the task summary, the task summary is the output chunk, and the output chunk is the output chunk.
**What's the point?*

```c
XWORK_API void xwork_remote_task_summary_reset(xwork_remote_task_summary *pSummary);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Yue僃恁 summary 鍍呴儴鎷ユ湁璧勬簮銆?
**Chen ュ Pang Xuan cun 槑?*

阃傜敤浜?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_summary_reset(&summary);
```

**What is the API?*

- `xwork_remote_task_summary_init`

---

### xwork_remote_task_summary_list_init

鍒濆鍖?remote task summary鍒楄〃銆?
**锷绻兘锛?*

鍑嗗绌?task summary list銆?
**What's the point?*

```c
XWORK_API void xwork_remote_task_summary_list_init(xwork_remote_task_summary_list *pList);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

咋卂椤 `xwork_control_plane_list_tasks` 铓嶅簲鍒捒濆鍖栥€?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_summary_list list;
xwork_remote_task_summary_list_init(&list);
```

**What is the API?*

- `xwork_control_plane_list_tasks`

---

### xwork_remote_task_summary_list_reset

Read the remote task summary 卒楄〃銆?
**锷绻兘锛?*

Read the link?task summary 鍜屾暟缁卩€?
**What's the point?*

```c
XWORK_API void xwork_remote_task_summary_list_reset(xwork_remote_task_summary_list *pList);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁鍒楄〃鎷ユ湁镄勫崴瀹广€?
**Chen ュ Pang Xuan cun 槑?*

Reading婃斁钖庡垪曛ㄥ彲澶敤抆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_summary_list_reset(&list);
```

**What is the API?*

- `xwork_remote_task_summary_reset`

---

### xwork_remote_task_snapshot_init

What is the problem?remote task snapshot?
**锷绻兘锛?*

鍑嗗 task snapshot锛叀敤浜?control plane鎸䷷箙鍖栧拋鎭㈠銆?
**What's the point?*

```c
XWORK_API void xwork_remote_task_snapshot_init(xwork_remote_task_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

What is the protocol version of the protocol?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_snapshot snapshot;
xwork_remote_task_snapshot_init(&snapshot);
```

**What is the API?*

- `xwork_control_plane_get_snapshot`

---

### xwork_remote_task_snapshot_reset

How to read remote task snapshot?
**锷绻兘锛?*

Read the task snapshot, the output chunk, and the output chunk.
**What's the point?*

```c
XWORK_API void xwork_remote_task_snapshot_reset(xwork_remote_task_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read the snapshot 卍呴儴鎷ユ湁璧勬簮銆?
**Chen ュ Pang Xuan cun 槑?*

What is the API API for?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_remote_task_snapshot_init`

---

### xwork_remote_task_snapshot_list_init

鍒濆鍖?remote task snapshot 鍒楄〃銆?
**锷绻兘锛?*

What is the task snapshot list?
**What's the point?*

```c
XWORK_API void xwork_remote_task_snapshot_list_init(xwork_remote_task_snapshot_list *pList);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_control_plane_snapshot.tTasks` `xwork_control_plane_snapshot.tTasks`?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_snapshot_list list;
xwork_remote_task_snapshot_list_init(&list);
```

**What is the API?*

- `xwork_remote_task_snapshot_list_reset`

---

### xwork_remote_task_snapshot_list_reset

Read remote task snapshot 卒楄〃銆?
**锷绻兘锛?*

Read the link?remote task snapshot 鍜屾暟缁卩€?
**What's the point?*

```c
XWORK_API void xwork_remote_task_snapshot_list_reset(xwork_remote_task_snapshot_list *pList);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁鍒楄〃鎷ユ湁镄勫崴瀹广€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_control_plane_snapshot_reset` 浼氶棿掺ヨ皟鐢ㄥ畠銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_snapshot_list_reset(&list);
```

**What is the API?*

- `xwork_control_plane_snapshot_reset`

---

### xwork_remote_task_assignment_init

What is the remote task assignment?
**锷绻兘锛?*

Worker claim task assignment assignment?
**What's the point?*

```c
XWORK_API void xwork_remote_task_assignment_init(xwork_remote_task_assignment *pAssignment);
```

**卙四暟锛?*

- `pAssignment`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

咋卂椤 `xwork_control_plane_claim_task` 铓嶅簲鍒捒濆鍖栥€?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_assignment assignment;
xwork_remote_task_assignment_init(&assignment);
```

**What is the API?*

- `xwork_control_plane_claim_task`

---

### xwork_remote_task_assignment_reset

Read the remote task assignment?
**锷绻兘锛?*

Read assignment assignment task task id signaturessignment id orker id request JSON request?
**What's the point?*

```c
XWORK_API void xwork_remote_task_assignment_reset(xwork_remote_task_assignment *pAssignment);
```

**卙四暟锛?*

- `pAssignment`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read assignment 卍呴儴鎷ユ湁璧勬簮銆?
**Chen ュ Pang Xuan cun 槑?*

Worker
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_assignment_reset(&assignment);
```

**What is the API?*

- `xwork_remote_task_assignment_init`

---

### xwork_remote_task_result_init

What is the problem?remote task result?
**锷绻兘锛?*

Worker
**What's the point?*

```c
XWORK_API void xwork_remote_task_result_init(xwork_remote_task_result *pResult);
```

**卙四暟锛?*

- `pResult`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

`complete_task` `complete_task`
**Chen ュ Pang Xuan cun 槑?*

`XWORK_OK`?Protocol version ?`XWORK_OK`?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_result result;
xwork_remote_task_result_init(&result);
result.sVisibleSummary = "done";
```

**What is the API?*

- `xwork_control_plane_complete_task`

---

### xwork_remote_output_chunk_init

What is the remote output chunk?
**锷绻兘锛?*

鍑嗗 stdout/stderr 邂囨湰 chunk 涓娄紶璇簇簰銆?
**What's the point?*

```c
XWORK_API void xwork_remote_output_chunk_init(xwork_remote_output_chunk *pChunk);
```

**卙四暟锛?*

- `pChunk`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Control plane control plane
**Chen ュ Pang Xuan cun 槑?*

樿樿 stream 涓?stdout銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_output_chunk chunk;
xwork_remote_output_chunk_init(&chunk);
chunk.sTaskId = "task-1";
chunk.sWorkerId = "worker-1";
chunk.sText = "line\n";
```

**What is the API?*

- `xwork_control_plane_upload_output_chunk`

---

### xwork_remote_output_chunk_summary_init

What is the output chunk summary?
**锷绻兘锛?*

What is the output chunk of the output chunk?
**What's the point?*

```c
XWORK_API void xwork_remote_output_chunk_summary_init(
    xwork_remote_output_chunk_summary *pSummary
);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

樿樿 stream 涓?stdout銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_output_chunk_summary summary;
xwork_remote_output_chunk_summary_init(&summary);
```

**What is the API?*

- `xwork_remote_output_chunk_summary_reset`

---

### xwork_remote_output_chunk_summary_reset

Read the output chunk summary?
**锷绻兘锛?*

Read content hash 鍜屾枃chain婴瀹广€?
**What's the point?*

```c
XWORK_API void xwork_remote_output_chunk_summary_reset(
    xwork_remote_output_chunk_summary *pSummary
);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read 婃斁鍐呴儴 deep-copy 瀛楃涓layer€?
**Chen ュ Pang Xuan cun 槑?*

Remote task summary/snapshot
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_output_chunk_summary_reset(&summary);
```

**What is the API?*

- `xwork_remote_output_chunk_summary_init`

---

### xwork_remote_output_chunk_summary_list_init

鍒濆鍖?output chunk summary 鍒楄〃銆?
**锷绻兘锛?*

鍑嗗绌?output chunk summary list銆?
**What's the point?*

```c
XWORK_API void xwork_remote_output_chunk_summary_list_init(
    xwork_remote_output_chunk_summary_list *pList
);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

鍒楄〃鍏卂礌阃氩father鉉ヨ嚜 task summary鴴?snapshot銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_output_chunk_summary_list list;
xwork_remote_output_chunk_summary_list_init(&list);
```

**What is the API?*

- `xwork_remote_output_chunk_summary_list_reset`

---

### xwork_remote_output_chunk_summary_list_reset

Read output chunk summary 卒楄〃銆?
**锷绻兘锛?*

Read the link?output chunk summary 鍜屾暟缁卩€?
**What's the point?*

```c
XWORK_API void xwork_remote_output_chunk_summary_list_reset(
    xwork_remote_output_chunk_summary_list *pList
);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁鍒楄〃鎷ユ湁镄勫崴瀹广€?
**Chen ュ Pang Xuan cun 槑?*

Yue僃斁钖庡彽澶涶敤銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_output_chunk_summary_list_reset(&list);
```

**What is the API?*

- `xwork_remote_output_chunk_summary_reset`

---

### xwork_remote_blob_chunk_summary_init

What is the artifact blob chunk summary?
**锷绻兘锛?*

What is the artifact blob chunk?
**What's the point?*

```c
XWORK_API void xwork_remote_blob_chunk_summary_init(
    xwork_remote_blob_chunk_summary *pSummary
);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

blob block
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_blob_chunk_summary summary;
xwork_remote_blob_chunk_summary_init(&summary);
```

**What is the API?*

- `xwork_control_plane_list_artifact_blobs`

---

### xwork_remote_blob_chunk_summary_reset

Read artifact blob chunk summary?
**锷绻兘锛?*

Read the task/assignment/worker/artifact/blob/hash function.
**What's the point?*

```c
XWORK_API void xwork_remote_blob_chunk_summary_reset(
    xwork_remote_blob_chunk_summary *pSummary
);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Yue僃斁鍐呴儴 deep-copy 璧勬簮銆?
**Chen ュ Pang Xuan cun 槑?*

咋卂敤钖?summary 锲炲韌 init Zhong Ruo€and€?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_blob_chunk_summary_reset(&summary);
```

**What is the API?*

- `xwork_remote_blob_chunk_summary_init`

---

### xwork_remote_blob_chunk_summary_list_init

鍒濆鍖?artifact blob chunk summary鍒楄〃銆?
**锷绻兘锛?*

鍑嗗绌?blob chunk summary list銆?
**What's the point?*

```c
XWORK_API void xwork_remote_blob_chunk_summary_list_init(
    xwork_remote_blob_chunk_summary_list *pList
);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

咋卂椤 `xwork_control_plane_list_artifact_blobs` 铓嶅簲鍒捒濆鍖栥€?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_blob_chunk_summary_list list;
xwork_remote_blob_chunk_summary_list_init(&list);
```

**What is the API?*

- `xwork_control_plane_list_artifact_blobs`

---

### xwork_remote_blob_chunk_summary_list_reset

译婃斁 artifact blob chunk summary 鍒楄〃銆?
**锷绻兘锛?*

Read the link? blob chunk summary 鍜屾暟缁卩€?
**What's the point?*

```c
XWORK_API void xwork_remote_blob_chunk_summary_list_reset(
    xwork_remote_blob_chunk_summary_list *pList
);
```

**卙四暟锛?*

- `pList`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁鍒楄〃鎷ユ湁镄勫崴瀹广€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_control_plane_snapshot_reset` 捼氶 spray鏀?snapshot 鍍呯殑 blob chunk 鍒楄〃銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_blob_chunk_summary_list_reset(&list);
```

**What is the API?*

- `xwork_remote_blob_chunk_summary_reset`

---

### xwork_remote_artifact_upload_init

How to use remote artifact upload?
**锷绻兘锛?*

鍑嗗 artifact summary 涓?blob chunk 涓娄綶璇簇眰銆?
**What's the point?*

```c
XWORK_API void xwork_remote_artifact_upload_init(xwork_remote_artifact_upload *pUpload);
```

**卙四暟锛?*

- `pUpload`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Control plane Artifact summary Chunk Chunk
**Chen ュ Pang Xuan cun 槑?*

蹇呴　璁魔典 task id銆亀orker id銆乤rtifact summary锛涙湁 chunk 铁版遁镞?`pChunkData`鍜?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_artifact_upload upload;
xwork_remote_artifact_upload_init(&upload);
upload.sTaskId = "task-1";
upload.sWorkerId = "worker-1";
upload.pArtifact = &artifactSummary;
```

**What is the API?*

- `xwork_control_plane_upload_artifact`

---

### xwork_control_plane_snapshot_init

What is the control plane snapshot?
**锷绻兘锛?*

Control plane snapshot control plane snapshot worker worker ask blob chunk block
**What's the point?*

```c
XWORK_API void xwork_control_plane_snapshot_init(xwork_control_plane_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

涓樿 transport 涓?in-process 锛宲rotocol version 涓涋铓瓓皓皗increasing chain€?
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_snapshot snapshot;
xwork_control_plane_snapshot_init(&snapshot);
```

**What is the API?*

- `xwork_control_plane_get_snapshot`

---

### xwork_control_plane_snapshot_reset

Read the control plane snapshot?
**锷绻兘锛?*

View plane id, orker snapshot list, emote task snapshot list, blob chunk list, etc.
**What's the point?*

```c
XWORK_API void xwork_control_plane_snapshot_reset(xwork_control_plane_snapshot *pSnapshot);
```

**卙四暟锛?*

- `pSnapshot`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read the snapshot 卍呴儴鎷ユ湁璧勬簮銆?
**Chen ュ Pang Xuan cun 槑?*

What is the API API for?
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_control_plane_create_from_snapshot`

---

## Control Plane

### xwork_control_plane_create

What is the control plane?
**锷绻兘锛?*

鍒涘 slowly 杩出▼浠氲姟玺у埗鞞枛簼值瀛?worker registry銆佷change锷￠槦鍒楀拋涓娄紶鏁版偁銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_create(
    const xwork_control_plane_options *pOptions,
    xwork_control_plane **ppPlane
);
```

**卙四暟锛?*

– `pOptions`
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

`*ppPlane` `*ppPlane` `*ppPlane`
**Chen ュ Pang Xuan cun 槑?*

Protocol version
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane *plane = NULL;
xwork_control_plane_options opts;
xwork_control_plane_options_init(&opts);
opts.sPlaneId = "plane-1";
opts.pRuntime = runtime;
xwork_control_plane_create(&opts, &plane);
```

**What is the API?*

- `xwork_control_plane_destroy`
- `xwork_control_plane_start`

---

### xwork_control_plane_create_from_snapshot

浠?snapshot 鎭㈠ control plane銆?
**锷绻兘锛?*

Worker registry, ask queue, esult, utput chunk, blob chunk?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_create_from_snapshot(
    const xwork_control_plane_options *pOptions,
    const xwork_control_plane_snapshot *pSnapshot,
    xwork_control_plane **ppPlane
);
```

**卙四暟锛?*

- `pOptions` `pSnapshot` is a snapshot file - `ppPlane` is a plane owned plane?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

鎴湬姛钖?plane 褰掕皟鐢ㄨ€呮卍chain夛绂snapshot 涓嶈玺ョ銆?
**Chen ュ Pang Xuan cun 槑?*

Assigned/running task `XWORK_REMOTE_TASK_ORPHANED`?cancelled?
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane *plane = NULL;
xwork_control_plane_create_from_snapshot(&opts, &snapshot, &plane);
```

**What is the API?*

- `xwork_control_plane_get_snapshot`

---

### xwork_control_plane_destroy

阌€姣?control plane銆?
**锷绻兘锛?*

Check the plane, the orker registry, the ask records, the lob chunks, the capability allowlist, and the query.
**What's the point?*

```c
XWORK_API void xwork_control_plane_destroy(xwork_control_plane *pPlane);
```

**卙四暟锛?*

- `pPlane`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Yue僃斁 plane 鎷ユ湁璧勬簮锛涗笉浼氶氶氀?borrowed What is the runtime like?
**Chen ュ Pang Xuan cun 槑?*

Transport?
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_destroy(plane);
```

**What is the API?*

- `xwork_control_plane_create`

---

### xwork_control_plane_start

What is the control plane?
**锷绻兘锛?*

What is the worker claim queued task?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_start(xwork_control_plane *pPlane);
```

**卙四暟锛?*

- `pPlane`?plane銆?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_ERROR_INVALID_ARGUMENT`
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

start the server
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_start(plane);
```

**What is the API?*

- `xwork_control_plane_stop`
- `xwork_control_plane_claim_task`

---

### xwork_control_plane_stop

Guocang control plane?
**锷绻兘锛?*

绂佹鏂谂讑 claim 缁х画镮峰彇浠氲姟銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_stop(xwork_control_plane *pPlane);
```

**卙四暟锛?*

- `pPlane`?plane銆?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_ERROR_INVALID_ARGUMENT`
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

stop 涓嶅彇娑埚fan缁?assigned/running 鄄勪 Change 锷★纴涔熶笉笉Guocang OS 杩涚▼銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_stop(plane);
```

**What is the API?*

- `xwork_control_plane_start`

---

### xwork_control_plane_set_time

Control plane control plane?
**锷绻兘锛?*

`nowMs` plane `nowMs`
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_set_time(
    xwork_control_plane *pPlane,
    size_t iNowMs
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

xwork
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_set_time(plane, nowMs);
```

**What is the API?*

- `xwork_control_plane_worker_heartbeat`
- `xwork_control_plane_sweep_stale`

---

## Worker Registry

### xwork_control_plane_register_worker

剉ㄥ唽worker銆?
**锷绻兘锛?*

Worker control plane registry control plane registry lease control plane registry
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_register_worker(
    xwork_control_plane *pPlane,
    const xwork_worker_options *pOptions,
    xwork_worker **ppWorker
);
```

**卙四暟锛?*

– `pPlane` `ppWorker`?borrowed worker?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

worker plane
**Chen ュ Pang Xuan cun 槑?*

worker protocol version
**锣冧緥締ｇ爜锛?*

```c
xwork_worker *worker = NULL;
xwork_worker_options opts;
xwork_worker_options_init(&opts);
opts.sWorkerId = "worker-1";
xwork_control_plane_register_worker(plane, &opts, &worker);
```

**What is the API?*

- `xwork_control_plane_worker_heartbeat`
- `xwork_control_plane_unregister_worker`

---

### xwork_control_plane_unregister_worker

What is the worker?
**锷绻兘锛?*

Worker Unregistered
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_unregister_worker(
    xwork_control_plane *pPlane,
    const char *sWorkerId
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

涓嶉谀退?plane 涓?worker 璁扉綍簛屼玎 Xuancun把中銉锟斤拷．
**Chen ュ Pang Xuan cun 槑?*

Sweep/cancel sweep/cancel sweep/cancel sweep/cancel sweep/cancel sweep/cancel sweep/cancel sweep/cancel sweep/cancel sweep/cancel sweep
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_unregister_worker(plane, "worker-1");
```

**What is the API?*

- `xwork_control_plane_sweep_stale`

---

### xwork_control_plane_worker_heartbeat

Worker heartbeat?
**锷绻兘锛?*

鍒 Feng把 worker 鄄?last heartbeat銆乴ease expires锛屽苟皏噙姸阐人江涓?online銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_worker_heartbeat(
    xwork_control_plane *pPlane,
    const char *sWorkerId,
    size_t iNowMs
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

unregistered worker 涓嶈兘 heartbeat銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_worker_heartbeat(plane, "worker-1", nowMs);
```

**What is the API?*

- `xwork_control_plane_sweep_stale`

---

### xwork_control_plane_sweep_stale

娓咯悊恩囨模 worker抆?
**锷绻兘锛?*

镙管偁 lease 杩囨湡镞枿镙拷错笇 stale worker锛屽苟灏嗗叾 assigned/running 浠淲姟枞negative orphaned銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_sweep_stale(
    xwork_control_plane *pPlane,
    size_t iNowMs,
    size_t *piOrphanedCount
);
```

**卙四暟锛?*

– `pPlane` `piOrphanedCount`?orphaned task?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

涓制父绉画卢墋勋戈抆?
**Chen ュ Pang Xuan cun 槑?*

璇?API 鄄仮澶嶅拰 worker lease 砠＄愄勄勬牳蹇冭窭琣狋牂orphaned钖庣敕瀹praised the rich?
**锣冧緥締ｇ爜锛?*

```c
size_t orphaned = 0;
xwork_control_plane_sweep_stale(plane, nowMs, &orphaned);
```

**What is the API?*

- `xwork_control_plane_worker_heartbeat`

---

### xwork_control_plane_list_workers

Worker?
**锷绻兘锛?*

Control plane control plane worker registry Worker registry
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_list_workers(
    const xwork_control_plane *pPlane,
    xwork_worker_summary_list *pList
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

鍒楄〃鎷ユ湁 deep-copy 鍍呭锛倀椤
**Chen ュ Pang Xuan cun 槑?*

What's the point?
**锣冧緥締ｇ爜锛?*

```c
xwork_worker_summary_list list;
xwork_worker_summary_list_init(&list);
xwork_control_plane_list_workers(plane, &list);
xwork_worker_summary_list_reset(&list);
```

**What is the API?*

- `xwork_worker_summary_list_reset`

---

## Remote Task 鐢熷懡综合ㄦ桡

### xwork_control_plane_enqueue_task

鍏ラ槦 remote task銆?
**锷绻兘锛?*

Remote task control plane control plane worker claim?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_enqueue_task(
    xwork_control_plane *pPlane,
    const xwork_remote_task_options *pOptions
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

plane task task
**Chen ュ Pang Xuan cun 槑?*

What is the function of the capability allowlist and the ask policy and the network policy?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_options task;
xwork_remote_task_options_init(&task);
task.sTaskId = "task-1";
task.sOperationId = XWORK_HOST_PROCESS_EXEC;
task.sRequestJson = "{}";
xwork_control_plane_enqueue_task(plane, &task);
```

**What is the API?*

- `xwork_control_plane_claim_task`

---

### xwork_control_plane_claim_task

worker?
**锷绻兘锛?*

涓?online worker 镆ユ鍖鍖综合 镄?queued task锛倀铓鴴?assignment銆鄄?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_claim_task(
    xwork_control_plane *pPlane,
    const char *sWorkerId,
    xwork_remote_task_assignment *pAssignment
);
```

**卙四暟锛?*

- `pAssignment`?assignment?init?
**杩斿洴 alkali fine**

`XWORK_OK`?XWORK_ERROR_NOT_FOUND`?
**璧勬簮褰掎睘锛?*

assignment 鎷ユ湁 deep-copy 瀛楁锛妀椤
**Chen ュ Pang Xuan cun 槑?*

plane 谇呴　　　?start锛泈orker 鑇呴‖ online锛涙椤鍙鍙栦change锷℃椂杩濿洖 not found銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_assignment assignment;
xwork_remote_task_assignment_init(&assignment);
xwork_control_plane_claim_task(plane, "worker-1", &assignment);
xwork_remote_task_assignment_reset(&assignment);
```

**What is the API?*

- `xwork_control_plane_complete_task`

---

### xwork_control_plane_complete_task

What is the remote task?
**锷绻兘锛?*

镕码嵁 assignment id 鎻愪helila浵氲姟缁洴灉锛屽苟银娄 Change锷℃爣灁狠negative completed鎴?failed銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_complete_task(
    xwork_control_plane *pPlane,
    const char *sAssignmentId,
    const xwork_remote_task_result *pResult
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

plane 澶嶅埗 output銆乻ummary銆鈪rror鍜?artifact summary銆?
**Chen ュ Pang Xuan cun 槑?*

`iStatus == XWORK_OK` protocol version
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_result result;
xwork_remote_task_result_init(&result);
result.sVisibleSummary = "completed";
xwork_control_plane_complete_task(plane, assignment.sAssignmentId, &result);
```

**What is the API?*

- `xwork_control_plane_fail_task`

---

### xwork_control_plane_fail_task

What is the remote task?
**锷绻兘锛?*

鐢ㄩ敊璇枃chain瀯阃犳爣鍑?remote task result锛屽苟鎻愪helium涓 coaxけLUョ粨鋋濿€?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_fail_task(
    xwork_control_plane *pPlane,
    const char *sAssignmentId,
    const char *sErrorText,
    bool bRetryable
);
```

**卙四暟锛?*

- `bRetryable`唛氭槸钖﹀璁璁t璇曘€?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

What is the task record?
**Chen ュ Pang Xuan cun 槑?*

`XWORK_ERROR_EXTERNAL_FAILURE`?complete result?
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_fail_task(plane, assignmentId, "tool failed", true);
```

**What is the API?*

- `xwork_control_plane_complete_task`

---

### xwork_control_plane_cancel_task

What is the remote task?
**锷绻兘锛?*

The task id is canceled?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_cancel_task(
    xwork_control_plane *pPlane,
    const char *sTaskId,
    const char *sReason
);
```

**卙四暟锛?*

- XWORKPLACEHOLDER0 TOKEN
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

plane 澶嶅埗鍙栨秷铡緷洜鍒?task 阌澾Q℃伅銆?
**Chen ュ Pang Xuan cun 槑?*

鍙栨秷涓気氻尢?worker 閖姩鄄勄閮?OS 杩涚▼锛泈orker/transport What's the point of the story?
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_cancel_task(plane, "task-1", "user cancelled");
```

**What is the API?*

- `xwork_control_plane_get_task_summary`

---

### xwork_control_plane_execute_next_local

What is the worker runtime?
**锷绻兘锛?*

What is the claim error?worker runtime host service error and complete/fail error?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_execute_next_local(
    xwork_control_plane *pPlane,
    const char *sWorkerId,
    xwork_remote_task_assignment *pAssignment
);
```

**卙四暟锛?*

- `pPlane`?plane?- `pAssignment`曰氩彲阃夎緭鍑瀄焄闄呮墽chen倛怀殑 assignment銆?
**杩斿洴 alkali fine**

杩斿洖host service 铓ц中间€佹娨 control plane 阌澾鉌鈥?
**璧勬簮褰掎睘锛?*

捈描暭 assignment 濡坝濉平曰倀敱咋啂敕敤Key?reset銆?
**Chen ュ Pang Xuan cun 槑?*

Worker function is used to implement runtime and API functions in-process worker mock and agent shell.
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_assignment assignment;
xwork_remote_task_assignment_init(&assignment);
xwork_control_plane_execute_next_local(plane, "worker-1", &assignment);
xwork_remote_task_assignment_reset(&assignment);
```

**What is the API?*

- `xwork_runtime_invoke_host_service`

---

### xwork_control_plane_get_task_summary

What is the remote task?
**锷绻兘锛?*

鎸?task id 銮峰彇浠氲槟中鰰€and€乤ssignment銆佺粨鋋溿€乤rtifact 鍜?output chunk 鎽樿銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_get_task_summary(
    const xwork_control_plane *pPlane,
    const char *sTaskId,
    xwork_remote_task_summary *pSummary
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

summary 鎷ユ湁 deep-copy 鍍呭锛倀椤
**Chen ュ Pang Xuan cun 槑?*

鍑 mustard
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_summary summary;
xwork_remote_task_summary_init(&summary);
xwork_control_plane_get_task_summary(plane, "task-1", &summary);
xwork_remote_task_summary_reset(&summary);
```

**What is the API?*

- `xwork_control_plane_list_tasks`

---

### xwork_control_plane_list_tasks

What is the remote task?
**锷绻兘锛?*

What is the control plane?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_list_tasks(
    const xwork_control_plane *pPlane,
    xwork_remote_task_summary_list *pList
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

鍒楄〃鎷ユ湁 deep-copy 鍍呭锛倀椤
**Chen ュ Pang Xuan cun 槑?*

What is the user interface for?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_task_summary_list list;
xwork_remote_task_summary_list_init(&list);
xwork_control_plane_list_tasks(plane, &list);
xwork_remote_task_summary_list_reset(&list);
```

**What is the API?*

- `xwork_control_plane_get_task_summary`

---

## Artifact涓?Output涓娄綶

### xwork_control_plane_upload_artifact

What is the remote artifact?
**锷绻兘锛?*

涓?remote task 枩 borrow姞鴴栨洿鏂?artifact summary锛屽Gouqi濆瓧鍙€?blob chunk銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_upload_artifact(
    xwork_control_plane *pPlane,
    const xwork_remote_artifact_upload *pUpload
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

plane 澶嶅埗 artifact summary銆乥lob metadata鍜?chunk 铁版偁銆?
**Chen ュ Pang Xuan cun 槑?*

浠淲槟涶嶶嶶涴尰 queued銆乧ancelled鎴?orphaned锛昘ssignment id 濡傛彁渚涳綴紇呴　鍖 Return 浠淲槟褰揿姠 assignment銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_artifact_upload upload;
xwork_remote_artifact_upload_init(&upload);
upload.sTaskId = "task-1";
upload.sWorkerId = "worker-1";
upload.pArtifact = &artifactSummary;
xwork_control_plane_upload_artifact(plane, &upload);
```

**What is the API?*

- `xwork_control_plane_list_artifact_blobs`

---

### xwork_control_plane_upload_output_chunk

What is the remote output chunk?
**锷绻兘锛?*

涓?remote task? stdout/stderr 邂囨湰chunk?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_upload_output_chunk(
    xwork_control_plane *pPlane,
    const xwork_remote_output_chunk *pChunk
);
```

**卙四暟锛?*

- `pPlane`?plane?-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

plane 澶嶅埗鏂囨湰銆乭ash鍜?chunk metadata銆?
**Chen ュ Pang Xuan cun 槑?*

浠淲槟涶嶶嶶涴尪簬 queued銆乧ancelled 鴴?orphaned锛沘ssignment id 濡傛彁涚涳纴呴　鍖Guili銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_output_chunk chunk;
xwork_remote_output_chunk_init(&chunk);
chunk.sTaskId = "task-1";
chunk.sWorkerId = "worker-1";
chunk.sText = "stdout line\n";
xwork_control_plane_upload_output_chunk(plane, &chunk);
```

**What is the API?*

- `xwork_remote_output_chunk_init`

---

### xwork_control_plane_list_artifact_blobs

What are artifact blob chunks?
**锷绻兘锛?*

Task id Artifact id blob chunk?
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_list_artifact_blobs(
    const xwork_control_plane *pPlane,
    const char *sTaskId,
    const char *sArtifactId,
    xwork_remote_blob_chunk_summary_list *pList
);
```

**卙四暟锛?*

- What is the chain?blob chunk?-`pList`?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

鍒楄〃鎷ユ湁 deep-copy 鍍呭锛倀椤
**Chen ュ Pang Xuan cun 槑?*

The API is the artifact blob?
**锣冧緥締ｇ爜锛?*

```c
xwork_remote_blob_chunk_summary_list list;
xwork_remote_blob_chunk_summary_list_init(&list);
xwork_control_plane_list_artifact_blobs(plane, "task-1", NULL, &list);
xwork_remote_blob_chunk_summary_list_reset(&list);
```

**What is the API?*

- `xwork_control_plane_upload_artifact`

---

## Snapshot

### xwork_control_plane_get_snapshot

銮峰彇control plane snapshot銆?
**锷绻兘锛?*

Control plane, worker registry, ask, control plane, worker registry, blob chunks, etc.
**What's the point?*

```c
XWORK_API xwork_status xwork_control_plane_get_snapshot(
    const xwork_control_plane *pPlane,
    xwork_control_plane_snapshot *pSnapshot
);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

snapshot 鎷ユ湁 deep-copy 鍍呭锛妀椤
**Chen ュ Pang Xuan cun 槑?*

snapshot, live network connection, thread, process, terminal handle, etc.
**锣冧緥締ｇ爜锛?*

```c
xwork_control_plane_snapshot snapshot;
xwork_control_plane_snapshot_init(&snapshot);
xwork_control_plane_get_snapshot(plane, &snapshot);
xwork_control_plane_snapshot_reset(&snapshot);
```

**What is the API?*

- `xwork_control_plane_create_from_snapshot`

---

## The manuscript is 叧鏂囨.

- [Host Tools API](api-host-tools.md)
- [Policy / Approval API](api-policy-approval.md)
- [杩滅▼ Worker 涓庢帶鍒跺钩闈(../guide/remote-worker-intro.md)
- [Remote Worker Agent 鑼冧緥](../case/remote-worker-agent.md)
- [鍐呴儴 remote worker contract](../../dev/docs/REMOTE_WORKER.md)
