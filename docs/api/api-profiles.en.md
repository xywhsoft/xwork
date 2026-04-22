# Profiles API

> Zhong Ruo €侊 fine Juan 枃 阬嚱鏁鏁 Board 嬬Key 溴纴寰呬Hanchen ュ阒呫€?
Profiles API鍖呮嫭`xcode`鍜?`xclaw`銆?
##妯″潡瀹hydrogen綅

Profile Profile涓 coax瀻绾匡纴鍍䶈鐩栨洿涓ユ牸鄄勫稨鍏ㄣ€乹ā鍨娨€乵emory銆乸lanner 鴴栧狠鍏风瓥飣ャ€?
## chain

| 绫淲埆 | 澹典槑 |
| --- | --- |
| Ning Shengnu | `XWORK_PROFILE_XCODE`, `XWORK_PROFILE_XCLAW` |
| `xwork_profile` |
| `xwork_profile_init`, `xwork_profile_get_builtin`, `xwork_profile_apply_runtime_options`, `xwork_profile_apply_xllm_profile_options`, `xwork_profile_apply_xllm_bootstrap_options`,

## Profile 瀛楁

| Yingqi | Xuan Cunmu |
| --- | --- |
|
| `sDisplayName` |
| `sDescription` | profile
|
|
| `eAutonomy` |
|
|
| `iDefaultMaxTurns` |
| `bDefaultAutoApprove` |
| `bEnableWorkspaceMemory` | Workspace memory?|
|

##卐寯江 Profile

| Profile |
| --- | --- |
| `XWORK_PROFILE_XCODE` / `"xcode"` | memory锛宲lanner boundary 鍏draw棴锛尀embroidered缼终粯璁ゆ嫆缁濄€?|
| `XWORK_PROFILE_XCLAW` / `"xclaw"` | boundary锛倀embroidery缀缀绀粛haren樿鎷掔粷逛岄櫎闱簶皟鐢ㄦ南閰瞿江 allowlist銆?|

## 鎺ㄨ嫘香噙洊椤 coax 簭

```text
options_init
xwork_profile_get_builtin
xwork_profile_apply_*_options
浜у搧灞傝鐩栨洿涓ユ牸绛栫暐
create runtime / workspace / run / orchestrator
```

杩欐牱鍙 Mutual Lang collapse calendar profile 痙洊洜уfan 灞傛樉剮忛濛赐利漃€?
---

### xwork_profile_init

鍒濆鍖?profile 缁撴瀯銆?
**锷绻兘锛?*

Profile铓嶈皟鐢ㄨ鍑 mustard隟锛屼婷婛楁涶涘叆狋畾畾樿正尊€?
**What's the point?*

```c
XWORK_API void xwork_profile_init(xwork_profile *pProfile);
```

**卙四暟锛?*

- `pProfile` is not the same `NULL` 镞多小娓娴浂骞跺开鍏ラ粯璁ょ瓥飣ャ€?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

鍑 must隟涓嶅垎閰嶅爢鍐煭稨銆俻rofile涓殑瀛楃涓insert秧娈典gliaiao鎸丸€熤畇箟銆?
**Chen ュ Pang Xuan cun 槑?*

- 樿樿把富鐐тnegative涓?
**锣冧緥締ｇ爜锛?*

```c
#include "xwork.h"

int main(void) {
    xwork_profile profile;
    xwork_profile_init(&profile);
    return profile.iDefaultMaxTurns == 4u ? 0 : 1;
}
```

**What is the API?*

- `xwork_profile_get_builtin`
- `xwork_profile_apply_run_options`

---

### xwork_profile_get_builtin

Luan Feng彇卐寯江 profile銆?
**锷绻兘锛?*

`xcode` `xclaw`镄勯粯璁ら利缃纴狀怀浜уfan卒濆鍖栭€夐‖镄勫熀狾 pants€?
**What's the point?*

```c
XWORK_API xwork_status xwork_profile_get_builtin(
    const char *sProfileId,
    xwork_profile *pProfile
);
```

**卙四暟锛?*

- `pProfile` is the only one that can be used as a profile?
**杩斿洴 alkali fine**

- `XWORK_OK` - `XWORK_ERROR_INVALID_ARGUMENT` - Juanhong┖洴栀欉銄唴缃?profile銆?
**璧勬簮褰掎睘锛?*

profile缁撴瀯鐢鐢皟鐢ㄨ€呮嫢chain夛绂鍏多酑瀛楃涓肖寚钖戦runhoe丸唴烃暟鎹纴璋卂敤Key呬笉鑳综合狠鏀 all €?
**Chen ュ Pang Xuan cun 槑?*

- 鐮峰彇 profile 涓莳緽皟銆?- 鍐卯jiang profile鏄ǔ瀹氰粯璁ゅ€緷纴娕嗕獝丸眰浠涅簲鏄鈥晙洊瀹夊叏揈皫抆?
**锣冧緥締ｇ爜锛?*

```c
#include "xwork.h"

int load_xclaw_profile(void) {
    xwork_profile profile;
    return xwork_profile_get_builtin(XWORK_PROFILE_XCLAW, &profile) == XWORK_OK ? 0 : 1;
}
```

**What is the API?*

- `xwork_profile_init`
- `xwork_profile_apply_runtime_options`

---

### xwork_profile_apply_runtime_options

Profile profile runtime options runtime options
**锷绻兘锛?*

嗲彲浠ョ敤璇ュ嚱鏁版妸profile涓殑policy 樿樿肖chain fried簲鐢ㄥ埌 `xwork_runtime_options`銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_profile_apply_runtime_options(
    const xwork_profile *pProfile,
    xwork_runtime_options *pOptions
);
```

**卙四暟锛?*

– `pProfile` `pOptions` is not the same as `NULL`
**杩斿洴 alkali fine**

– `XWORK_OK`
**璧勬簮褰掎睘锛?*

`pOptions` `pOptions`
**Chen ュ Pang Xuan cun 槑?*

- Error code for runtime policy and xllm runtime and OST services for persistence backend -濡悛灉捜簲獦ㄨ皟鐢ㄨ鍑 mustard 隟钖庡啀嗙洊抆?
**锣冧緥締ｇ爜锛?*

```c
#include "xwork.h"

int apply_runtime_profile(const xwork_profile *profile) {
    xwork_runtime_options options;
    xwork_runtime_options_init(&options);
    return xwork_profile_apply_runtime_options(profile, &options) == XWORK_OK ? 0 : 1;
}
```

**What is the API?*

- `xwork_runtime_options_init`
- `xwork_runtime_create`

---

### xwork_profile_apply_xllm_profile_options

鎶?profile 镄勯粯璁ゆā鍨?profile id 鍐椤叆 xllm profile options銆?
**锷绻兘锛?*

`xwork_xllm_profile_options` `xwork_xllm_profile_options` profile Xllm profile id
**What's the point?*

```c
XWORK_API xwork_status xwork_profile_apply_xllm_profile_options(
    const xwork_profile *pProfile,
    xwork_xllm_profile_options *pOptions
);
```

**卙四暟锛?*

– `pProfile` `pOptions`?`NULL`?
**杩斿洴 alkali fine**

– `XWORK_OK`
**璧勬簮褰掎睘锛?*

profile id 鍜?display name Profile?
**Chen ュ Pang Xuan cun 槑?*

- `pOptions->sProfileId` `pOptions->sProfileId` `pOptions->sDisplayName`宸茬粡chain夐潓绌 coax€ alkali 纴鍑 mustard 暟涓氙晙洊瀹刦€?
**锣冧緥締ｇ爜锛?*

```c
#include "xwork.h"

int apply_llm_profile(const xwork_profile *profile) {
    xwork_xllm_profile_options options;
    xwork_xllm_profile_options_init(&options);
    return xwork_profile_apply_xllm_profile_options(profile, &options) == XWORK_OK ? 0 : 1;
}
```

**What is the API?*

- `xwork_xllm_profile_options_init`
- `xwork_profile_apply_xllm_bootstrap_options`

---

### xwork_profile_apply_xllm_bootstrap_options

Profile profile
**锷绻兘锛?*

`xwork_xllm_profile_options` runtime create bootstrap bootstrap xllm runtime?
**What's the point?*

```c
XWORK_API xwork_status xwork_profile_apply_xllm_bootstrap_options(
    const xwork_profile *pProfile,
    xwork_xllm_profile_options *pProfileOptions,
    xwork_xllm_bootstrap_options *pBootstrapOptions
);
```

**卙四暟锛?*

– `pProfile` `pProfileOptions` `pBootstrapOptions`氛氲緭鍏?枈揿揭卙四暟隆 effect瀹椤Marriage骁鍒楄〃锛屽嚱鏁雳铸囧悜
**杩斿洴 alkali fine**

- `XWORK_OK`
**璧勬簮褰掎睘锛?*

Is there a problem with `pProfileOptions`?XWORKPLACEH OLDER1TOKEN?`xwork_runtime_create` What is the value of the chain?
**Chen ュ Pang Xuan cun 槑?*

- `pBootstrapOptions->pProfiles` `NULL` `iProfileCount` `0``1``1`
**锣冧緥締ｇ爜锛?*

```c
#include "xwork.h"

int apply_bootstrap_profile(const xwork_profile *profile) {
    xwork_xllm_profile_options llm_profile;
    xwork_xllm_bootstrap_options bootstrap;
    xwork_xllm_profile_options_init(&llm_profile);
    xwork_xllm_bootstrap_options_init(&bootstrap);
    return xwork_profile_apply_xllm_bootstrap_options(
        profile,
        &llm_profile,
        &bootstrap
    ) == XWORK_OK ? 0 : 1;
}
```

**What is the API?*

- `xwork_xllm_bootstrap_options_init`
- `xwork_runtime_create`

---

### xwork_profile_apply_workspace_options

鎶?profile鄄?workspace鎶樿chain fried qi鏏?workspace options銆?
**锷绻兘锛?*

Profile Workspace Workspace Memory
**What's the point?*

```c
XWORK_API xwork_status xwork_profile_apply_workspace_options(
    const xwork_profile *pProfile,
    xwork_workspace_options *pOptions
);
```

**卙四暟锛?*

– `pProfile` `pOptions` is not the same as `NULL`
**杩斿洴 alkali fine**

– `XWORK_OK`
**璧勬簮褰掎睘锛?*

`pOptions->pMemory`?
**Chen ュ Pang Xuan cun 槑?*

- 钖椤 memory 鍙缃竷総洿紑鍏鍏珂璋卂敤鏂 Visit粛繇呴　鉪淵 chain夋晥 `xllm_memory *`銆?-浜уfan鍙湪璋卂敤钖庣户缁鐩?include/exclude 绛栫暐抆?
**锣冧緥締ｇ爜锛?*

```c
#include "xwork.h"

int apply_workspace_profile(const xwork_profile *profile) {
    xwork_workspace_options options;
    xwork_workspace_options_init(&options);
    return xwork_profile_apply_workspace_options(profile, &options) == XWORK_OK ? 0 : 1;
}
```

**What is the API?*

- `xwork_workspace_options_init`
- `xwork_runtime_add_workspace`

---

### xwork_profile_apply_run_options

鎶?profile 鄄?run 翶樿Chain Jianqi鍏?run options銆?
**锷绻兘锛?*

We can run the profile and run the session policy.
**What's the point?*

```c
XWORK_API xwork_status xwork_profile_apply_run_options(
    const xwork_profile *pProfile,
    xwork_run_options *pOptions
);
```

**卙四暟锛?*

– `pProfile` `pOptions`?`NULL`?
**杩斿洴 alkali fine**

– `XWORK_OK`
**璧勬簮褰掎睘锛?*

What is the value of `sLlmProfileId` `sSessionProfileId`?
**Chen ュ Pang Xuan cun 槑?*

- 捿悛灉 run options 宸茌粡璁剧江豱炵┖ profile id锛屽嚱鏁issued笉浼氲鐩栥€?- 鍑 must拟浼氲鐩?`eAutonomy`鍜?`tSessionPolicy`銆?
**锣冧緥締ｇ爜锛?*

```c
#include "xwork.h"

int apply_run_profile(const xwork_profile *profile) {
    xwork_run_options options;
    xwork_run_options_init(&options);
    return xwork_profile_apply_run_options(profile, &options) == XWORK_OK ? 0 : 1;
}
```

**What is the API?*

- `xwork_run_options_init`
- `xwork_run_create`

---

### xwork_profile_apply_orchestrator_options

鎶?profile 鄄?orchestrator 锶樿碇 fried 凯鍏?orchestrator options銆?
**锷绻兘锛?*

orchestrator orchestrator profile 镄可粯璁?max turns銆乸lanner mode鍜?auto approve 璁剧江銆?
**What's the point?*

```c
XWORK_API xwork_status xwork_profile_apply_orchestrator_options(
    const xwork_profile *pProfile,
    xwork_orchestrator_options *pOptions
);
```

**卙四暟锛?*

– `pProfile` `pOptions`?`NULL`?
**杩斿洴 alkali fine**

– `XWORK_OK`
**璧勬簮褰掎睘锛?*

What are the options?
**Chen ュ Pang Xuan cun 槑?*

- `iDefaultMaxTurns > 0` `pOptions->iMaxTurns` `bAutoApprove` `bAutoApprove`
**锣冧緥締ｇ爜锛?*

```c
#include "xwork.h"

int apply_orchestrator_profile(const xwork_profile *profile) {
    xwork_orchestrator_options options;
    xwork_orchestrator_options_init(&options);
    return xwork_profile_apply_orchestrator_options(profile, &options) == XWORK_OK ? 0 : 1;
}
```

**What is the API?*

- `xwork_orchestrator_options_init`
- `xwork_run_execute`

## 阌澾澶拭把

-
## 鎭㈠杈Guihu

profile 鏄利缃暟鎹纴涓嶆嶆湹枞?live Zhong Ruo€四仮澶?run 镞讹纴璋卂椤鏂gui簲Read嶆把搴 Flag椤涓庡师 run鏏竞技镄?profile/options锛屽啀锷纺水 snapshot 鎴?persistence 鏁版偁銆?
## 绾cross▼杈爈晫

profile apply The options are as follows: And the options are as follows:缁撴瀯銆?
## The manuscript is 叧鏂囨.

- [Runtime API](api-runtime.md)
- [Workspace API](api-workspace.md)
- [Run API](api-run.md)
- [AI IDE Agent 鑼冧緥](../case/ai-ide-agent.md)
- [claw 鑷富 Agent 鑼冧緥](../case/claw-autonomous-agent.md)
