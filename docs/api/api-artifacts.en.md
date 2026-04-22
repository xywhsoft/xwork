# Artifact API

> Zhong Ruo €侊 fine Juan 枃 阬嚱鏁鏁 Board 嬬Key 溴纴寰呬Hanchen ュ阒呫€?
Artifact API Agent杩愯涓殑鏂囦Huan鍍呭銆丸atch銆丶浡浠よ緭鍑heng€佺粓绔姸镐和€佽鏂拰鎶ュ憡奇濆瓨谽彲镆ヨ銆丽彲鎸佷癙鍖栥€佸彲瀹¤镄勪吗┿€?
##妯″潡瀹hydrogen綅

Artifact metadata銆乻ummary銆乻torage ref 鍜屽彲阃?content text锛僃浜уfan瞞博嚜琛屽睍犀瞨钖屾銆?
## chain

| 绫淲埆 | 澹典槑 |
| --- | --- |
| `xwork_artifact_options`, `xwork_patch_artifact_options`, `xwork_report_artifact_options`, `xwork_output_artifact_options`, `xwork_artifact_summary_query` |
| `xwork_artifact_options_init`, `xwork_patch_artifact_options_init`, `xwork_report_artifact_options_init`, `xwork_output_artifact_options_init`, `xwork_command_artifact_options_init`, `xwork_artifact_summary_reset`, `xwork_artifact_summary_list_init`, `xwork_artifact_summary_list_reset`, `xwork_artifact_summary_query_init` |

## Artifact Kind

| Ling Haoyou | Xuan Cunmu |
| --- | --- |
|
| `XWORK_ARTIFACT_REPORT` |
| `XWORK_ARTIFACT_COMMAND` |
| `XWORK_ARTIFACT_OUTPUT` |

## Schema Ning Sheng

- `XWORK_REPORT_SCHEMA_V1`
- `XWORK_DIAGNOSTICS_SCHEMA_V1`
- `XWORK_PATCH_APPLY_RESULT_SCHEMA_V1`
- `XWORK_PATCH_FILE_SUMMARY_SCHEMA_V1`
- `XWORK_TERMINAL_STATE_SCHEMA_V1`
- `XWORK_TERMINAL_INVENTORY_SCHEMA_V1`

## gallium€chain勋戋戁诺勫寯

- options 涓殑瀛楃涓INSert拰鍐卭鎸拋鍧囦negative borrowed锛枦negative 锛跺鍒淺麟 run/artifact Yinghua卍銆?-`xwork_artifact` `xwork_artifact_summary` `pItems` `sStorageRef` Owned blob?
---

### xwork_artifact_options_init

What are the artifact options?
**锷绻兘锛?*

鐢ㄤ簬鐐渴璋卂椤
**What's the point?*

```c
XWORK_API void xwork_artifact_options_init(xwork_artifact_options *pOptions);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

What is the value of the product?
**Chen ュ Pang Xuan cun 槑?*

- `eKind` `XWORK_ARTIFACT_OUTPUT` `XWORK_ARTIFACT_OUTPUT` `XWORK_ARTIFACT_OUTPUT` kind銆乶ame銆乵ime/content/storage 绛夊瓧娈点€?
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact_options options;
xwork_artifact_options_init(&options);
options.sName = "output.txt";
options.sContentText = "hello";
```

**What is the API?*

- `xwork_run_emit_artifact`
- `xwork_artifact_init`

---

### xwork_patch_artifact_options_init

How can I patch artifact options?
**锷绻兘锛?*

鐢ㄤ簬鍙戝嚭 patch artifact 铓嶅婳澶?patch text銆似aget ref銆乤pply result鍜屾枃浠浠憳笶?JSON銆?
**What's the point?*

```c
XWORK_API void xwork_patch_artifact_options_init(xwork_patch_artifact_options *pOptions);
```

**卙四暟锛?*

- `pOptions` is not the same as `NULL` is `NULL` is it?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

What is the value of the product?
**Chen ュ Pang Xuan cun 槑?*

- `xwork_run_emit_patch_artifact` kind 璁jujiangjuan? `sFileSummaryJson` `XWORK_PATCH_FILE_SUMMARY_SCHEMA_V1`?
**锣冧緥締ｇ爜锛?*

```c
xwork_patch_artifact_options options;
xwork_patch_artifact_options_init(&options);
options.sPatchText = "--- a/file\n+++ b/file\n";
```

**What is the API?*

- `xwork_run_emit_patch_artifact`

---

### xwork_report_artifact_options_init

How to use report artifact options?
**锷绻兘锛?*

鐢ㄤ簬鍙捙戝嚭鎶ュ憡銆佽瘖鏂€佽鍒掋€乺eview 鎴?final 揈描嚭铓嶅婶徶囨姤综合婂瓧娈Point€?
**What's the point?*

```c
XWORK_API void xwork_report_artifact_options_init(xwork_report_artifact_options *pOptions);
```

**卙四暟锛?*

- `pOptions` is not the same as the previous version?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

What is the value of the product?
**Chen ュ Pang Xuan cun 槑?*

- `sMimeType` `text/markdown`
**锣冧緥締ｇ爜锛?*

```c
xwork_report_artifact_options options;
xwork_report_artifact_options_init(&options);
options.eReportClass = XWORK_ARTIFACT_REPORT_FINAL;
options.sReportText = "# Result\n";
```

**What is the API?*

- `xwork_run_emit_report_artifact`
- `XWORK_REPORT_SCHEMA_V1`

---

### xwork_output_artifact_options_init

What are the output artifact options?
**锷绻兘锛?*

鐢ㄤ簬鍙捙戝嚭酅€氭枃chain€丣SON銆佹枃浠淺崴瀹广€人瓓绔姸镐佽垨鍏多粬阃氱敤杈揿嚭銆?
**What's the point?*

```c
XWORK_API void xwork_output_artifact_options_init(xwork_output_artifact_options *pOptions);
```

**卙四暟锛?*

- `pOptions` is not the same as the previous version?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

What is the value of the product?
**Chen ュ Pang Xuan cun 槑?*

- `sMimeType` `text/plain` - JSON `sMimeType = "application/json"` `eOutputClass = XWORK_ARTIFACT_OUTPUT_JSON`
**锣冧緥締ｇ爜锛?*

```c
xwork_output_artifact_options options;
xwork_output_artifact_options_init(&options);
options.sOutputText = "done";
```

**What is the API?*

- `xwork_run_emit_output_artifact`

---

### xwork_command_artifact_options_init

What is the command artifact option?
**锷绻兘锛?*

鐢ㄤ簬璁璁板綍 forging ring guard 鏂囨幰銆佽緭鍑heng€乪xit code鍜?stdout/stderr缁緻銆?
**What's the point?*

```c
XWORK_API void xwork_command_artifact_options_init(xwork_command_artifact_options *pOptions);
```

**卙四暟锛?*

- `pOptions` is not the same as the previous version?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

What is the value of the product?
**Chen ュ Pang Xuan cun 槑?*

- `sMimeType` `text/plain` exit code `bHasExitCode = true` exit code
**锣冧緥締ｇ爜锛?*

```c
xwork_command_artifact_options options;
xwork_command_artifact_options_init(&options);
options.sCommandText = "git status";
options.bHasExitCode = true;
options.iExitCode = 0;
```

**What is the API?*

- `xwork_run_emit_command_artifact`

---

### xwork_artifact_init

鍒濆鍖?artifact銆?
**锷绻兘锛?*

鍑嗗鎺ユ敹 run emit銆乺un get鎴?persistence load 杩洿洴鄄?artifact銆?
**What's the point?*

```c
XWORK_API void xwork_artifact_init(xwork_artifact *pArtifact);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

鍑 must鈟涓嶅垎閰制祫婧愩€ effect～鍏呭悗鄄?artifact鎷ユ湁 deep-copy 瀛楁锛屽幀椤?reset銆?
**Chen ュ Pang Xuan cun 槑?*

- `eKind` `XWORK_ARTIFACT_OUTPUT`
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact artifact;
xwork_artifact_init(&artifact);
xwork_artifact_reset(&artifact);
```

**What is the API?*

- `xwork_artifact_reset`
- `xwork_run_get_artifact`

---

### xwork_artifact_reset

Yue僃斁骞鈞枆?artifact銆?
**锷绻兘锛?*

Read the artifact 涓殑 id銆乺un id銆乶ame銆乵ime銆乻torage ref銆乻ummary銆乧ontent鍜?typed metadata Ying楁銆?
**What's the point?*

```c
XWORK_API void xwork_artifact_reset(xwork_artifact *pArtifact);
```

**卙四暟锛?*

- `pArtifact`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

译婃斁 artifact 鎷ユ湁鄄勋瓧绗︿荓湰銆?
**Chen ュ Pang Xuan cun 槑?*

- reset 钖?artifact 锲炲韌 init Zhong Ruo€?
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact_reset(&artifact);
```

**What is the API?*

- `xwork_artifact_init`

---

### xwork_artifact_summary_init

What is the artifact summary?
**锷绻兘锛?*

鍑嗗玺ユ敹 artifact summary 镆ヨ缁撴灉銆?
**What's the point?*

```c
XWORK_API void xwork_artifact_summary_init(xwork_artifact_summary *pSummary);
```

**卙四暟锛?*

-
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

鍑 must鈟涓嶅垎閰制祫婧愩€ effect～鍏呭悗鄄?summary鎷ユ湁 deep-copy 瀛楁锛屽幀椤?reset銆?
**Chen ュ Pang Xuan cun 槑?*

- summary 涓嶅set钖粲鏁?content text锛屽彧equi戈暀鍙煡璇?metadata鍜粀粺璁°€?
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact_summary summary;
xwork_artifact_summary_init(&summary);
xwork_artifact_summary_reset(&summary);
```

**What is the API?*

- `xwork_artifact_summary_reset`
- `xwork_runtime_query_persisted_artifact_summaries`

---

### xwork_artifact_summary_reset

Read the article summary?
**锷绻兘锛?*

Read the summary 涓殑 id銆乶ame銆乵ime銆torage ref銆乻ummary銆乺ole銆乺eport subject鍜?patch JSON Ying楁銆?
**What's the point?*

```c
XWORK_API void xwork_artifact_summary_reset(xwork_artifact_summary *pSummary);
```

**卙四暟锛?*

- `pSummary`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

译婃斁 summary 鎷ユ湁鄄勋瓧绗︿荓湰銆?
**Chen ュ Pang Xuan cun 槑?*

- reset 钖?summary 锲炲韌 init Zhong Ruo€?
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact_summary_reset(&summary);
```

**What is the API?*

- `xwork_artifact_summary_init`

---

### xwork_artifact_summary_list_init

鍒濆鍖?artifact summary 鍒楄〃銆?
**锷绻兘锛?*

鍑嗗鎺ユ湕 artifact summary 镆ヨ鍒楄〃銆?
**What's the point?*

```c
XWORK_API void xwork_artifact_summary_list_init(xwork_artifact_summary_list *pList);
```

**卙四暟锛?*

- `pList` is not the same as `NULL` is `NULL` is it?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

`pItems` `pItems` `pItems`?
**Chen ュ Pang Xuan cun 槑?*

- `xwork_artifact_summary_list_reset`?
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact_summary_list list;
xwork_artifact_summary_list_init(&list);
xwork_artifact_summary_list_reset(&list);
```

**What is the API?*

- `xwork_artifact_summary_list_reset`

---

### xwork_artifact_summary_list_reset

Yue僃斁骞鈥惆?artifact summary鍒楄〃銆?
**锷绻兘锛?*

Read寃斁鍒楄〃鏁 play粍鍙僃婃涓?summary鎷ユ湁鄄?deep-copy Ying楁銆?
**What's the point?*

```c
XWORK_API void xwork_artifact_summary_list_reset(xwork_artifact_summary_list *pList);
```

**卙四暟锛?*

- `pList`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

Read `pItems`
**Chen ュ Pang Xuan cun 槑?*

- reset 钖庡彽澶敤鍒楄〃鍙橀噺銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact_summary_list_reset(&list);
```

**What is the API?*

- `xwork_artifact_summary_list_init`

---

### xwork_artifact_summary_query_init

鍒濆鍖?artifact summary 镆ヨ鏉′Huan銆?
**锷绻兘锛?*

鐢ㄤ簬鎸?kind銆乷utput class銆乺ole銆乺eport class銆乶ame銆丮IME銆乻torage ref銆乪xit code鍜?sequence锣冨洿杩囨protect artifact summary銆?
**What's the point?*

```c
XWORK_API void xwork_artifact_summary_query_init(xwork_artifact_summary_query *pQuery);
```

**卙四暟锛?*

- `pQuery` is not the same as `NULL` is `NULL` is it?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

What is the value of the product?
**Chen ュ Pang Xuan cun 槑?*

- `iLimit` `bHasMore`鍜?`iNextAfterSequence`銆?
**锣冧緥締ｇ爜锛?*

```c
xwork_artifact_summary_query query;
xwork_artifact_summary_query_init(&query);
query.bHasOutputClass = true;
query.eOutputClass = XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE;
query.iLimit = 50u;
```

**What is the API?*

- `xwork_runtime_query_persisted_artifact_summaries`

## 鍙戝皭 Artifact

[Run API](api-run.md)
-`xwork_run_emit_artifact`
-`xwork_run_emit_patch_artifact`
-`xwork_run_emit_report_artifact`
-`xwork_run_emit_output_artifact`
-`xwork_run_emit_command_artifact`

## 鎭㈠杈Guihu

artifact metadata 鍜?content text 鍙 mutual 鎸佷箙鍖栨仮侶僃€俙sStorageRef` 鎸囧悜鄄勫閮?blob掴栨枃捐浠剁敱瀹凯富绯獤粺琐绻 chu攛拋work 鍐卯江 file backend 涓涐鐞嗗垎宁娨Fang blob store銆?
## 绾cross▼杈爈晫

artifact init/reset/query 缁撴瀯揓嶈邂叏揓€锛氽un emit/get/query 鎿莴綔浼氲鍐?run 鎴?persistence backend锛屽涓涓€ run 鄄?mutation What is the meaning of the flag?
## The manuscript is 叧鏂囨.

- [Run API](api-run.md)
- [Persistence API](api-persistence.md)
- [宸ュ叿銆佸鎵逛笌 artifact](../guide/tool-approval-artifact-intro.md)
