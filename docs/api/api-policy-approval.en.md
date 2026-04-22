# Policy / Approval API

> Zhong Ruo €侊 fine Juan 枃 阬嚱鏁鏁 Board 嬬Key 溴纴寰呬Hanchen ュ阒呫€?
Policy / Approval API 瀹hydrogen箟铓綔鐢ㄤ箣铓瓓殑缁綶竴瀹夊叏揈gui櫫銆?
##妯″潡瀹hydrogen綅

Policy Policy揆effect仠钖庢毚hiddenstubble粰浜уfan UI醆丆LI锴栬嚜锷ㄧ瓥鐣ョ殑瀹¤瀵 silicon thin銆抆曛湰妯″潡涓嶅疄鐜?UI銆佽处鍙鍙潈闄愩€亀orker 璁よ玴?socket What is the value of the product?
## chain

| 绫淲埆 | 澹典槑 |
| --- | --- |
| `xwork_policy_options`, `xwork_approval_eval_input`, `xwork_approval_decision`, `xwork_network_policy_eval_input`,
| `xwork_policy_options_init`, `xwork_approval_eval_input_init`, `xwork_approval_decision_init`, `xwork_network_policy_eval_input_init`, `xwork_network_policy_decision_init`, `xwork_policy_evaluate_network_access` |

## 镙 manuscript results Jiangzhiwei

| Yingqi/瀵 silicon thin | Xuan Cunmu |
| --- | --- |
| `eAutoApproveRiskLimit` |
|
|
|
| `xwork_approval_decision` |
| `xwork_approval_request` | run

## gallium€chain勋戋戁诺勫寯

- policy options runtime/control-plane options allow/deny pattern allow/deny pattern borrowed - eval input tool id銆乻cope銆乺eason銆乁RL銆乭ost 绛夊瓧绗︿cover閮 mustard槸 borrowed銆?-decision涓殑瀛楃涓fork寚閖戦run昐佹枃 chain枨 input-owned override - approval request init涓嶅垎閰制祫婧愶禂镆ヨ/锷纺水尰楀埌鄄?request 鎷ユ恁 deep-copy 瀛楁锛屼小鐢ㄥ恗湇呴』 reset銆?
---

### xwork_policy_options_init

What are the policy options?
**锷绻兘锛?*

鐢ㄤ簬鍒涘笶runtime銆乧ontrol plane 鴴揫嫭磔嬭瘎浼栠璁璁秠稿瀀夏绛栫暐抆?
**What's the point?*

```c
XWORK_API void xwork_policy_options_init(xwork_policy_options *pOptions);
```

**卙四暟锛?*

- `pOptions` is not the same as `NULL` is `NULL` is the policy?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

鍑 must掟涓嶅垎閰嶈祫婧橩€俛llow/deny 鏁衣粍瀛楁稿 ?
**Chen ュ Pang Xuan cun 槑?*

- init profile init profile Hazelnuts绛栫暐昃effect掎寮€鍙戞捩捩 Hongwai锛屼笉搴旀浛浠ｄ簁鍝丶缁矚畕鍏ㄧ瓥鐣ャ€?
**锣冧緥締ｇ爜锛?*

```c
#include "xwork.h"

int main(void) {
    xwork_policy_options policy;
    xwork_policy_options_init(&policy);
    policy.bDenyNetworkByDefault = true;
    return 0;
}
```

**What is the API?*

- `xwork_policy_evaluate_approval`
- `xwork_policy_evaluate_network_access`

---

### xwork_approval_eval_input_init

鍒濆鍖栧gallium silicon阎浼mix緷鍏ャ€?
**锷绻兘锛?*

What is the side effect?
**What's the point?*

```c
XWORK_API void xwork_approval_eval_input_init(xwork_approval_eval_input *pInput);
```

**卙四暟锛?*

- `pInput` is not the same as the original version.鍜?risk銆?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

What is the value of the product?
**Chen ュ Pang Xuan cun 槑?*

- Risk override side-effect side-effect
**锣冧緥締ｇ爜锛?*

```c
xwork_approval_eval_input input;
xwork_approval_eval_input_init(&input);
input.eSideEffect = XWORK_SIDE_EFFECT_PROCESS_EXEC;
```

**What is the API?*

- `xwork_policy_evaluate_approval`

---

### xwork_approval_decision_init

鍒濆鍖栧gallium silicon 阎浼貨鋋濿€?
**锷绻兘锛?*

鍑嗗鎺ユ湕`xwork_policy_evaluate_approval`鄄勮緭鍑heng€?
**What's the point?*

```c
XWORK_API void xwork_approval_decision_init(xwork_approval_decision *pDecision);
```

**卙四暟锛?*

- `pDecision` is not the same as `NULL` The rudder and the board are stamped with furrows and stilts?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

鍑簟暟涓嶅垎閰制祫婧橩€俤ecision 瀛楃涓铋negative borrowed銆?
**Chen ュ Pang Xuan cun 槑?*

- decision 涓嶉涶肖?reset銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_approval_decision decision;
xwork_approval_decision_init(&decision);
```

**What is the API?*

- `xwork_policy_evaluate_approval`

---

### xwork_network_policy_eval_input_init

鍒濆鍖栫embroidery缁MI瓥鐣ヨ瘎浼mix緭鍏ャ€?
**锷绻兘锛?*

鐢ㄤ簬鎻忚凯涓€娆＄Embroidery缁滆闂姹傜殑 URL/host 涓娄笅鏂囥€?
**What's the point?*

```c
XWORK_API void xwork_network_policy_eval_input_init(
    xwork_network_policy_eval_input *pInput
);
```

**卙四暟锛?*

- `pInput` is not the same as `NULL` is `NULL` is it?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

The host is the host of the host.
**Chen ュ Pang Xuan cun 槑?*

- The chain is the same as the embroidered one.
**锣冧緥締ｇ爜锛?*

```c
xwork_network_policy_eval_input input;
xwork_network_policy_eval_input_init(&input);
input.bNetworkAccessRequested = true;
input.sHost = "api.example.com";
```

**What is the API?*

- `xwork_policy_evaluate_network_access`

---

### xwork_network_policy_decision_init

鍒濆鍖栫embroidery缁簥铥鐣ヨ瘎浼貨鋋溿€?
**锷绻兘锛?*

鍑嗗鎺ユ湕`xwork_policy_evaluate_network_access`鄄勮緭鍑heng€?
**What's the point?*

```c
XWORK_API void xwork_network_policy_decision_init(
    xwork_network_policy_decision *pDecision
);
```

**卙四暟锛?*

- `pDecision` is not the same as `NULL` The rudder and the board are stamped with furrows and stilts?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

鍑簟暟涓嶅垎閰制祫婧橩€俤ecision 瀛楃涓铋negative borrowed銆?
**Chen ュ Pang Xuan cun 槑?*

- decision 涓嶉涶肖?reset銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_network_policy_decision decision;
xwork_network_policy_decision_init(&decision);
```

**What is the API?*

- `xwork_policy_evaluate_network_access`

---

### xwork_approval_request_init

Is there an approval request?
**锷绻兘锛?*

鍑嗗鎺ユ敹 run 鎴?persistence 杩洿洴鄄勫gallium silicon姹 umbrella€?
**What's the point?*

```c
XWORK_API void xwork_approval_request_init(xwork_approval_request *pRequest);
```

**卙四暟锛?*

- `pRequest` `NULL` `NULL`
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

鍑 must掟涓嶅垎閰嶈祫婧愩€ effect～鍏呭怗镄?request 鎷ユ湁 deep-copy 瀛楁锛屼蕉鐢ㄥ悗璋卂捤 `xwork_approval_request_reset`銆?
**Chen ュ Pang Xuan cun 槑?*

- request 鏄璁″璞★纴阃氩father丞弚缁?UI/CLI閖庣敱浜уfan璋卂椤 `xwork_run_submit_approval`銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_approval_request request;
xwork_approval_request_init(&request);
xwork_approval_request_reset(&request);
```

**What is the API?*

- `xwork_run_get_last_approval_request`
- `xwork_approval_request_reset`

---

### xwork_approval_request_reset

Read the request? Approval request?
**锷绻兘锛?*

Read the request 涓殑 id銆乺un id銆乼ool id銆乺eason銆乻cope銆乤ction summary 绛?deep-copy 瀛楁銆?
**What's the point?*

```c
XWORK_API void xwork_approval_request_reset(xwork_approval_request *pRequest);
```

**卙四暟锛?*

- `pRequest`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read the request to read the request.
**Chen ュ Pang Xuan cun 槑?*

- reset 钖?request 锲炲韌 init Zhong Ruo€?
**锣冧緥締ｇ爜锛?*

```c
xwork_approval_request_reset(&request);
```

**What is the API?*

- `xwork_approval_request_init`

---

### xwork_policy_evaluate_approval

What is the side effect?
**锷绻兘锛?*

The standard is the autonomy, the pproval mode, the ide effect, the isk override and the auto-approve limit.
**What's the point?*

```c
XWORK_API xwork_status xwork_policy_evaluate_approval(
    const xwork_policy_options *pPolicy,
    const xwork_approval_eval_input *pInput,
    xwork_approval_decision *pDecision
);
```

**卙四暟锛?*

- `pInput` `NULL`?
**杩斿洴 alkali fine**

-
**璧勬簮褰掎睘锛?*

decision 瀛楃涓cruci槸 borrowed锛涘嚱鏁issued笉鍒嗛狠狠在eh?
**Chen ュ Pang Xuan cun 槑?*

- `XWORK_APPROVAL_ALWAYS` - `XWORK_APPROVAL_NEVER` `XWORK_APPROVAL_DEFAULT`鍜岄闄╅槇駇瞞鏂€?
**锣冧緥締ｇ爜锛?*

```c
xwork_policy_options policy;
xwork_approval_eval_input input;
xwork_approval_decision decision;

xwork_policy_options_init(&policy);
xwork_approval_eval_input_init(&input);
xwork_approval_decision_init(&decision);
input.eAutonomy = XWORK_AUTONOMY_SEMI_AUTO;
input.eSideEffect = XWORK_SIDE_EFFECT_WORKSPACE_WRITE;

(void)xwork_policy_evaluate_approval(&policy, &input, &decision);
```

**What is the API?*

- `xwork_run_submit_approval`
- `xwork_policy_evaluate_network_access`

---

### xwork_policy_evaluate_network_access

璇勪和涓€娆＄Embroidered缁细闂槸钖﹀玑璁做€?
**锷绻兘锛?*

镙尧偁 network requested镙囧综合銆乁RL/host銆乨enylist銆乤llowlist鍜岄粯璁ゆ嫆缁濈瓥飣ョ拓掴愮embroidery缁细闂喅笛栥€?
**What's the point?*

```c
XWORK_API xwork_status xwork_policy_evaluate_network_access(
    const xwork_policy_options *pPolicy,
    const xwork_network_policy_eval_input *pInput,
    xwork_network_policy_decision *pDecision
);
```

**卙四暟锛?*

- `pInput` `NULL`?
**杩斿洴 alkali fine**

-
**璧勬簮褰掎睘锛?*

decision 瀛楃涓cruci槸 borrowed锛涘嚱鏁issued笉鍒嗛狠狠在eh?
**Chen ュ Pang Xuan cun 槑?*

- deny patterns 浼华厛浜?allow patterns銆?-閰浰江 allow patterns閖庯纴host雇呴　 forging ringfu allowlist銆?-chain利缃?allow patterns镞锺彇鍐成鰬 `bDenyNetworkByDefault`銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_network_policy_eval_input input;
xwork_network_policy_decision decision;

xwork_network_policy_eval_input_init(&input);
xwork_network_policy_decision_init(&decision);
input.bNetworkAccessRequested = true;
input.sHost = "api.example.com";

(void)xwork_policy_evaluate_network_access(NULL, &input, &decision);
```

**What is the API?*

- `xwork_policy_options_init`
- `xwork_policy_evaluate_approval`

## 阌澾澶拭把

- `XWORK_ERROR_NOT_FOUND` `XWORK_ERROR_NOT_FOUND` `XWORK_ERROR_NOT_FOUND`
## 鎭㈠杈Guihu

approval request run snapshot/persistence run snapshot/persistence callback Zhongduo€and€佤掴鴴莴璇濆拰琐﹀佛鉉冮檺涓嶅睘浜?xwork 鎭㈠锣娨洿銆?
## 绾cross▼杈爈晫

policy evaluate鍑 mustard 暓涓Brand 退 Rose 叏灞€Zhong rudder €侊绂骞彂彂瀊叏鍙栧姜浜庤皟Policy/input storage?
## The manuscript is 叧鏂囨.

- [Tool API](api-tools.md)
- [Run API](api-run.md)
- [Orchestrator API](api-orchestrator.md)
- [宸ュ叿銆佸鎵逛笌 artifact](../guide/tool-approval-artifact-intro.md)
- [鍐呴儴 policy contract](../../dev/docs/POLICY_APPROVAL.md)
