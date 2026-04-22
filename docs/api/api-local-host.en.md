# Local Host API

> What is the role of the clock?
Local Host API 鄄? ?`xwork_host_services`?
## 鐩 manuscript 婧澹版槑

- `xwork_local_host_options`
- `xwork_local_host`
- `xwork_local_host_options_init()`
- `xwork_local_host_init()`
- `xwork_local_host_configure_services()`
- `xwork_local_host_reset()`
- `xwork_host_services`
- `xwork_host_services_init()`

##妯″潡瀹hydrogen綅

local host 銄湰鍦mix鹘绋嫔唴 helper鈆 Effect畠阃 Effect掎 examples鈆ﹻmoke銆丄I IDE/claw Chain management, host services, JSON contract, JSON contract鍜?artifact璇箟銆?
##閰玖江瀛楁

| Yingqi | Xuan Cunmu |
| --- | --- |
|
|
| `psFilesystemAllowPathPrefixes` | allow prefix?|
| `psFilesystemDenyPathPrefixes` | Deny prefix?|
| `psCommandAllowPatterns` | forged ring guard allow pattern?|
| `psCommandDenyPatterns` | Forged Ring Deny pattern?|
|
| `iMaxReadBytes` |
|
|
| `iMaxProcessOutputBytes` | process output 涓婇檺銆?|

The file system read/write process executes the CS status/diff/log/branch editor buffers.
## The chain €恏濛利烃?
```c
xwork_local_host_options tOptions;
xwork_local_host tHost;
xwork_host_services tServices;

xwork_local_host_options_init(&tOptions);
xwork_local_host_init(&tHost);
xwork_host_services_init(&tServices);

tOptions.sDefaultWorkingDirectory = "D:/git/project";
tOptions.bEnforceFilesystemRoot = true;
tOptions.bEnableFilesystemReadText = true;
tOptions.bEnableFilesystemWriteText = true;
tOptions.bEnableProcessExec = true;
tOptions.bEnableVcsStatus = true;

status = xwork_local_host_configure_services(&tHost, &tOptions, &tServices);
```

`xwork_runtime_options::pHostServices` `tServices` `tHost` callback user data runtime is the host service?
## 阃愬嚱鏁mix鄄?
### xwork_host_service_init

What is the host service?
**锷绻兘锛?*

`xwork_host_service`
**What's the point?*

```c
XWORK_API void xwork_host_service_init(xwork_host_service *pService);
```

**卙四暟锛?*

- `pService`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰嶈祫婧愶禂 callback鍜?user data鍧囩啱咋卂椤鏂gui鐞嗐€?
**Chen ュ Pang Xuan cun 槑?*

鐢綶瀹瀹 boast rich 鍙 mutual trickle rugged local host锛叀濿掺ュ～鍏呰嚜宸箑 `xwork_host_service`銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_host_service service;
xwork_host_service_init(&service);
```

**What is the API?*

- `xwork_host_services_init`

---

### xwork_host_services_init

What is the host service?
**锷绻兘锛?*

`xwork_host_services` host service
**What's the point?*

```c
XWORK_API void xwork_host_services_init(xwork_host_services *pServices);
```

**卙四暟锛?*

- `pServices`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰嶈祫婧愶绂 services 缁撴瀯寯撶敱咋卂敤揂guigul嫢夈€?
**Chen ュ Pang Xuan cun 槑?*

浼犵粰 runtime options 钖庯紴 鎸夊€ fried鍒?services锛朜allback user data 鄄勭铓forge borrowed the chain 熶粛颐 graduate皟鐢ㄦnanqi濊抆銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_host_services services;
xwork_host_services_init(&services);
```

**What is the API?*

- `xwork_local_host_configure_services`

---

### xwork_local_host_options_init

What are the local host options?
**锷绻兘锛?*

What is the host service helper?
**What's the point?*

```c
XWORK_API void xwork_local_host_options_init(xwork_local_host_options *pOptions);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧橩€?
**Chen ュ Pang Xuan cun 槑?*

`xwork_local_host_configure_services` filesystem filesystem filesystem editor-buffer
**锣冧緥締ｇ爜锛?*

```c
xwork_local_host_options opts;
xwork_local_host_options_init(&opts);
opts.bEnableProcessExec = true;
```

**What is the API?*

- `xwork_local_host_configure_services`

---

### xwork_local_host_init

What is the local host helper?
**锷绻兘锛?*

鍑嗗涓€涓皟鐢ㄦ南鎸丹湹湁鄄?
**What's the point?*

```c
XWORK_API void xwork_local_host_init(xwork_local_host *pHost);
```

**卙四暟锛?*

- `pHost`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

The host is the host.
**Chen ュ Pang Xuan cun 槑?*

閰浯獒?runtime閖庯纴`pHost`鈇呴　娲沲匌 runtime银嶅啀咋卂椤host service銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_local_host host;
xwork_local_host_init(&host);
```

**What is the API?*

- `xwork_local_host_reset`

---

### xwork_local_host_reset

Read the local host helper?
**锷绻兘锛?*

荐婃斁 local host 卐呴儴澶嶅埗镄啄勮羰寰勩乤llow/deny 鍒楄〃銆乼erminal/session 鍍呴璧勬簮銆?
**What's the point?*

```c
XWORK_API void xwork_local_host_reset(xwork_local_host *pHost);
```

**卙四暟锛?*

-`pHost`?`NULL`?host?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read婃斁捍呴儴璧勬簮锛屼笉Read僃斁缁撴瀯钴湰韬€?
**Chen ュ Pang Xuan cun 槑?*

What is the runtime value of the host service?reset?
**锣冧緥締ｇ爜锛?*

```c
xwork_local_host_reset(&host);
```

**What is the API?*

- `xwork_local_host_init`

---

### xwork_local_host_configure_services

鰰浰江 local host services銆?
**锷绻兘锛?*

What is the local host options?host helper?filesystem/process/terminal/vcs/editor-buffer callback?`xwork_host_services`?
**What's the point?*

```c
XWORK_API xwork_status xwork_local_host_configure_services(
    xwork_local_host *pHost,
    const xwork_local_host_options *pOptions,
    xwork_host_services *pServices
);
```

**卙四暟锛?*

- `pServices`?services?init?
**杩斿洴 alkali fine**

`XWORK_OK` `XWORK_OK`?
**璧勬簮褰掎睘锛?*

XWorkPLACEHOLDER0TOKEN
**Chen ュ Pang Xuan cun 槑?*

local
**锣冧緥締ｇ爜锛?*

```c
xwork_local_host_configure_services(&host, &opts, &services);
```

**What is the API?*

- `xwork_runtime_create`

---

### xwork_host_invoke_context_should_cancel

What’s the host service?
**锷绻兘锛?*

The host service is the host service. The host service is the host service. Cancel token is the runtime.
**What's the point?*

```c
XWORK_API bool xwork_host_invoke_context_should_cancel(
    const xwork_host_invoke_context *pContext,
    const char *sPhase
);
```

**卙四暟锛?*

- `pContext` is the most invoke context.
**杩斿洴 alkali fine**

`false`?`false`?
**璧勬簮褰掎睘锛?*

`sPhase`?
**Chen ュ Pang Xuan cun 槑?*

host service 搴斿湪 spawn 铓嶃€両/O 寰円銆云瓑寰哙杩涚▼銆䷷笂浼?chunk 绛夐Stack娈典富锷ㄦ镆ャ€?
**锣冧緥締ｇ爜锛?*

```c
if (xwork_host_invoke_context_should_cancel(ctx, "before-spawn")) {
    return XWORK_ERROR_CANCELLED;
}
```

**What is the API?*

- `xwork_tool_exec_context_should_cancel`

---

## process.exec

local `process.exec` 璧?
-cwd override
- stdin_text
- env list
- timeout_ms
- timeout_stopinterrupt/terminate/kill/kill_tree
- allow_nonzero_exit
- merge_stderr
-include_events
- max_output_bytes
-terminal mode

缁撴灉鍖卭惈 stdout/stderr銆乪xit code鈹韅鏂鈣灁璁両/O 缁絻銆乼imeout/cancel/stop reason鍜屽彲阃?ordered events銆?
## interactive terminal

local host 退寔锛?
-`process.start_terminal`
-`process.list_terminals`
-`process.terminal_read`
-`process.terminal_write`
-`process.terminal_resize`
-`process.terminal_stop`

terminal session 鄄?live local resource锛屼笉浼氲法杩涚▼ restart 镊姩鎭㈠銆四寔涔呭寲 artifact 鍙瀬瀛?transcript/state/inventory 璁 Board綍銆?
## 瀹夊叏夤hong

- `bEnforceFilesystemRoot` - `bEnforceFilesystemRoot` - `bEnforceFilesystemRoot` - `bEnforceFilesystemRoot` pattern?- 瀵?destructive command?
## The manuscript is 叧鏂囨.

- [Host Tools API](api-host-tools.md)
- [Policy / Approval API](api-policy-approval.md)
- [claw 鑷富 Agent 鑼冧緥](../case/claw-autonomous-agent.md)
- [鍐呴儴 host tool contract](../../dev/docs/HOST_TOOL_CONTRACTS.md)
