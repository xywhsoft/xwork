# Orchestrator API

> What is the role of the clock?
Orchestrator API `xwork_run` Model turn + tool loop What is the agent?
## 鐩 manuscript 婧澹版槑

- `xwork_orchestrator_options`
- `xwork_model_stream_mode`
- `xwork_model_event`
- `xwork_model_event_fn`
- `xwork_tool_choice_mode`
- `xwork_run_execute()`
- `xwork_run_execute_async()`
- `xwork_run_submit_approval()`
- `xwork_run_resume()`

##妯″潡瀹hydrogen綅

Orchestrator xllm xllm xllm xllm xllm What is the planner's policy? What's the policy?
## galliumц闂幆

```text
run_execute
  prepare model input
  call xllm model turn
  forward stream events
  receive model output/tool calls
  evaluate policy
  execute tool or pause for approval
  emit events/artifacts/checkpoints
  continue next turn until final output or budget exhausted
```

## 阃愬嚱鏁mix鄄?
### xwork_orchestrator_options_init

What are the orchestrator options?
**锷绻兘锛?*

`xwork_run_execute` / `xwork_run_execute_async` model-turn + tool-loop
**What's the point?*

```c
XWORK_API void xwork_orchestrator_options_init(xwork_orchestrator_options *pOptions);
```

**卙四暟锛?*

- `pOptions`?`NULL`?
**杩斿洴 alkali fine**

镞畮€?
**璧勬簮褰掎睘锛?*

涓嶅垎閰制祫婧愶绂profile id銆乼ool choice鈆乧allback user data 绛夎緭鍏ュ湪铓цchain熆棿游養搴?API 鈫勯寯夤銆?
**Chen ュ Pang Xuan cun 槑?*

鍒濆鍖栧怗捐刈缃ā鍨?profile銆乼urn budget銆乻stream callback銆鈼ool choice鍜屾尛珀瓥鐣ャ€四彽簛汾湡悂村搓涓€ run 涓嶅厑璁 put tons鍏ャ€?
**锣冧緥締ｇ爜锛?*

```c
xwork_orchestrator_options opts;
xwork_orchestrator_options_init(&opts);
opts.iMaxTurns = 8;
```

**What is the API?*

- `xwork_run_execute`
- `xwork_run_execute_async`

---

## Model stream event

`xwork_model_event` xllm

| Yingqi | Xuan Cunmu |
| --- | --- |
| `eType` | xllm event type?|
| `sText` |
| `sResponseId` / `sModel` |
| `sToolCallId` / `sToolId` /
|
| `sArtifactId` /

callback `false` `false` `XWORK_ERROR_CANCELLED`
The fine

- interrupt / cancel token 妫€镆卛嵜庣敤洴?event callback銆?-鐢ㄦ埛 event callback 杩斿洖
## Tool loop 涓庡铓?
褰洴 ā鍨嬭姹 effect 鍏front 椂锛宱rchestrator 浼Panxi

1. Tool tool definition? 2. Tool side effect Tool pproval mode Tool policy 3. Tool executor Host service? 4. Host service缁х画铓ц銆?
掩掎佳 AI IDE 鍙 Interactive 鍦?UI 涓铓?patch銆乧ommand 鎴?terminal 鎎鎴綔緼篃鍏佽 claw 镙gui偁 profile啊姩铓 Rose 嗩娳娨闄╁姩Huan溿€?
## Tool choice

orchestrator?
| Grandma Spinning | Xuan Cunmu |
| --- | --- |
| `XWORK_TOOL_CHOICE_AUTO` |
| `XWORK_TOOL_CHOICE_NONE` |
| `XWORK_TOOL_CHOICE_REQUIRED` |
| `XWORK_TOOL_CHOICE_NAMED` |

`xwork_orchestrator_options`
## 钖屾铓ц

```c
xwork_status status = xwork_run_execute(pRun, &tOptions);
if (status == XWORK_ERROR_PAUSED) {
    /* 鏌ヨ approval request锛屾彁浜ゅ鎵瑰悗 resume銆?*/
}
```

## 寮傛铓ц

Run API
```c
xwork_run_async *pAsync = NULL;
xwork_run_execute_async(pRun, &tOptions, &pAsync);
```

async cancel 捼氲繘鍏ュ涓涓€濂楀agangHuandengfang cancel token纺SAT锛屽法鍏front彽墛屽櫒鍜?host service 搴旀镆?context銆?
## 阌澾猤?
- `XWORK_ERROR_INVALID_ARGUMENT` The clock rudder €佷笉鍏佽铓ц锛屾娨钖屼髴 run 诶叆铓ц銆?- XWORKPLACEHOLDER2 TOKEN `XWORK_ERROR_CANCELLED`, ancel token, interrupt, allback false, async cancel, -
## 绾cross▼杈爈晫

Orchestrator铓ц鍏协彛銆effective纾姝ユ墽chen屾汾湡邂达纴璋卂椤鏂rose駧鑳 Introduction€氲tension async handle Mutation run?
## 鎭㈠杈Guihu

orchestrator run snapshot pending tool pproval decision checkpoint checkpoint Runtime, orkspace, ool registry, ost service, ersistence backend, xllm runtime/profile?
## The manuscript is 叧鏂囨.

- [Run API](api-run.md)
- [Tool API](api-tools.md)
- [xllm 缂栨帓涓庡伐鍏峰惊鐜痌(../guide/xllm-orchestrator-intro.md)
- [AI IDE Agent 鑼冧緥](../case/ai-ide-agent.md)
