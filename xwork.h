#ifndef XWORK_H
#define XWORK_H

/*
 * xwork v2: the agent/tool-loop boundary above xllm and xllm-session.
 *
 * xwork owns orchestration, workspace policy, tool execution, artifacts and
 * compaction scheduling. It does not own provider protocols or CLI rendering.
 */

#include "xllm.h"
#include "xllm-session.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XWORK_VERSION_MAJOR 2
#define XWORK_VERSION_MINOR 0
#define XWORK_VERSION_PATCH 0

typedef struct xwork_agent xwork_agent;

typedef enum xwork_result {
    XWORK_RESULT_OK = 0,
    XWORK_RESULT_ERROR = -1,
    XWORK_RESULT_CANCELLED = -2,
    XWORK_RESULT_LIMIT = -3,
    XWORK_RESULT_TIMEOUT = -4
} xwork_result;

typedef enum xwork_error_code {
    XWORK_ERROR_NONE = 0,
    XWORK_ERROR_INVALID_ARGUMENT,
    XWORK_ERROR_OUT_OF_MEMORY,
    XWORK_ERROR_MODEL,
    XWORK_ERROR_TOOL,
    XWORK_ERROR_POLICY,
    XWORK_ERROR_IO,
    XWORK_ERROR_CONTEXT,
    XWORK_ERROR_LOOP_GUARD,
    XWORK_ERROR_CANCELLED,
    XWORK_ERROR_TIMEOUT
} xwork_error_code;

typedef struct xwork_error {
    xwork_error_code eCode;
    char sMessage[1024];
    xllm_error tModelError;
} xwork_error;

typedef enum xwork_tool_effect {
    XWORK_TOOL_EFFECT_READ_ONLY = 0,
    XWORK_TOOL_EFFECT_WORKSPACE_WRITE,
    XWORK_TOOL_EFFECT_PROCESS
} xwork_tool_effect;

typedef enum xwork_approval_mode {
    /* Execute tools automatically inside the configured workspace sandbox. */
    XWORK_APPROVAL_AUTO = 0,
    /* Ask the host callback before workspace writes and process execution. */
    XWORK_APPROVAL_CALLBACK,
    /* Allow reads but reject all mutating tools. */
    XWORK_APPROVAL_READ_ONLY
} xwork_approval_mode;

typedef enum xwork_permission_decision {
    XWORK_PERMISSION_DEFAULT = 0,
    XWORK_PERMISSION_ALLOW,
    XWORK_PERMISSION_DENY
} xwork_permission_decision;

typedef enum xwork_resource_kind {
    XWORK_RESOURCE_NONE = 0,
    XWORK_RESOURCE_PATH,
    XWORK_RESOURCE_COMMAND,
    XWORK_RESOURCE_PROCESS
} xwork_resource_kind;

typedef enum xwork_risk_level {
    XWORK_RISK_LOW = 0,
    XWORK_RISK_MEDIUM,
    XWORK_RISK_HIGH
} xwork_risk_level;

typedef struct xwork_permission_request {
    const char* sToolName;
    xwork_tool_effect eEffect;
    xwork_risk_level eRisk;
    xwork_resource_kind eResourceKind;
    const char* sResource;
    const char* sArgumentsJson;
    const char* sWorkspaceRoot;
    uint64_t uAgentTurn;
} xwork_permission_request;

typedef enum xwork_hook_phase {
    XWORK_HOOK_BEFORE_TOOL = 0,
    XWORK_HOOK_AFTER_TOOL
} xwork_hook_phase;

typedef enum xwork_hook_action {
    XWORK_HOOK_CONTINUE = 0,
    XWORK_HOOK_DENY,
    XWORK_HOOK_CANCEL
} xwork_hook_action;

typedef struct xwork_hook_event {
    xwork_hook_phase ePhase;
    uint64_t uAgentTurn;
    const char* sToolName;
    xwork_tool_effect eEffect;
    const char* sArgumentsJson;
    const char* sOutput;
    bool bSuccess;
} xwork_hook_event;

typedef struct xwork_tool_context {
    xwork_agent* pAgent;
    const char* sWorkspaceRoot;
    const char* sToolCallId;
    uint64_t uAgentTurn;
} xwork_tool_context;

typedef struct xwork_tool_output {
    char* sContent;
    bool bSuccess;
} xwork_tool_output;

typedef xwork_result (*xwork_tool_execute_fn)(
    void* pUserData,
    const xwork_tool_context* pContext,
    const char* sArgumentsJson,
    xwork_tool_output* pOutput,
    xwork_error* pError
);

typedef struct xwork_tool_definition {
    const char* sName;
    const char* sDescription;
    const char* sParametersJson;
    bool bStrict;
    xwork_tool_effect eEffect;
    xwork_tool_execute_fn OnExecute;
    void* pUserData;
} xwork_tool_definition;

typedef enum xwork_event_kind {
    XWORK_EVENT_AGENT_START = 0,
    XWORK_EVENT_MODEL_START,
    XWORK_EVENT_MODEL_TEXT_DELTA,
    XWORK_EVENT_MODEL_REASONING_DELTA,
    XWORK_EVENT_MODEL_DONE,
    XWORK_EVENT_TOOL_START,
    XWORK_EVENT_TOOL_DONE,
    XWORK_EVENT_COMPACTION_START,
    XWORK_EVENT_COMPACTION_DONE,
    XWORK_EVENT_AGENT_DONE,
    XWORK_EVENT_ERROR
} xwork_event_kind;

typedef struct xwork_event {
    xwork_event_kind eKind;
    uint64_t uAgentTurn;
    const char* sText;
    size_t iTextLength;
    const char* sToolName;
    const char* sToolCallId;
    const char* sArtifactPath;
    const char* sModel;
    const char* sProviderRequestId;
    const char* sFinishReason;
    const char* sRequestFingerprint;
    size_t iMessageCount;
    size_t iToolDefinitionCount;
    size_t iResponseToolCallCount;
    uint32_t uMaxOutputTokens;
    uint32_t uHttpStatus;
    bool bSuccess;
    xllm_usage tUsage;
    xllm_diagnostics tDiagnostics;
    xllm_session_stats tSessionStats;
} xwork_event;

/* Return false to request cooperative cancellation. */
typedef bool (*xwork_event_fn)(void* pUserData, const xwork_event* pEvent);

/* Return true to approve the requested side effect. */
typedef bool (*xwork_approval_fn)(
    void* pUserData,
    const char* sToolName,
    xwork_tool_effect eEffect,
    const char* sArgumentsJson
);

/* Structured per-call policy. DEFAULT falls back to eApprovalMode/OnApproval. */
typedef xwork_permission_decision (*xwork_permission_fn)(
    void* pUserData,
    const xwork_permission_request* pRequest
);

/* DENY is a tool-level rejection before execution. After execution it marks
 * the tool result failed because an already-completed side effect cannot be undone. */
typedef xwork_hook_action (*xwork_hook_fn)(void* pUserData, const xwork_hook_event* pEvent);

/* Injectable model boundary used by tests and offline hosts. */
typedef xllm_result (*xwork_model_complete_fn)(
    void* pUserData,
    const xllm_request* pRequest,
    const xllm_stream_callbacks* pCallbacks,
    xllm_response** ppResponse,
    xllm_error* pError
);

typedef struct xwork_agent_config {
    /* Borrowed dependencies; they must outlive the agent. */
    xllm_client* pClient;
    xllm_session* pSession;

    const char* sWorkspaceRoot;
    const char* sSystemPrompt;
    const char* sSessionPath;
    const char* sArtifactDirectory;
    const char* sModel;
    const char* sReasoningEffort;
    xctx* pContext;

    xwork_approval_mode eApprovalMode;
    xwork_approval_fn OnApproval;
    void* pApprovalUserData;
    xwork_permission_fn OnPermission;
    void* pPermissionUserData;
    xwork_hook_fn OnHook;
    void* pHookUserData;

    xwork_event_fn OnEvent;
    void* pEventUserData;

    xwork_model_complete_fn OnModelComplete;
    void* pModelUserData;

    uint32_t uCommandTimeoutMs;
    uint32_t uMaxAgentTurns;          /* 0 means unlimited. */
    uint32_t uRepeatedToolBatchLimit;
    uint32_t uConsecutiveFailureLimit;
    uint32_t uMaxManagedProcesses;
    uint32_t uCompletionVerificationRetries; /* Premature final answers after a write; default 2. */
    size_t iMaxInlineToolBytes;
    size_t iMaxCapturedCommandBytes;
    bool bRegisterBuiltinTools;
    bool bAutoSaveSession;
    bool bRequireVerificationAfterWrite;      /* Require successful exec_command after latest write. */
} xwork_agent_config;

typedef struct xwork_run_result {
    char* sFinalText;
    uint64_t uAgentTurns;
    uint64_t uModelCalls;
    uint64_t uToolCalls;
    uint64_t uCompactions;
    xllm_usage tLastUsage;
    xllm_session_stats tFinalSessionStats;
} xwork_run_result;

void xworkErrorInit(xwork_error* pError);
const char* xworkErrorCodeName(xwork_error_code eCode);
void xworkToolOutputInit(xwork_tool_output* pOutput);
void xworkToolOutputUnit(xwork_tool_output* pOutput);
bool xworkToolOutputSet(xwork_tool_output* pOutput, bool bSuccess, const char* sContent);

void xworkAgentConfigInit(xwork_agent_config* pConfig);
xwork_agent* xworkAgentCreate(const xwork_agent_config* pConfig, xwork_error* pError);
void xworkAgentDestroy(xwork_agent* pAgent);
bool xworkAgentRegisterTool(xwork_agent* pAgent, const xwork_tool_definition* pDefinition, xwork_error* pError);
size_t xworkAgentToolCount(const xwork_agent* pAgent);
bool xworkAgentCancel(xwork_agent* pAgent);
const char* xworkAgentWorkspaceRoot(const xwork_agent* pAgent);

xwork_result xworkAgentRun(xwork_agent* pAgent, const char* sPrompt, xwork_run_result* pResult, xwork_error* pError);
/* Resume an interrupted run without appending another user prompt. Pending
 * tool calls are completed first; an interrupted model call is retried from
 * the durable session tail. */
xwork_result xworkAgentResume(xwork_agent* pAgent, xwork_run_result* pResult, xwork_error* pError);
/* Force one safe-prefix summary compaction and persist the committed session. */
xwork_result xworkAgentCompact(xwork_agent* pAgent, xwork_error* pError);
void xworkRunResultUnit(xwork_run_result* pResult);

/* Registers filesystem, transactional edit, synchronous command, and managed process tools. */
bool xworkAgentRegisterBuiltinTools(xwork_agent* pAgent, xwork_error* pError);

#ifdef __cplusplus
}
#endif

#endif
