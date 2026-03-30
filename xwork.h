#ifndef XWORK_H
#define XWORK_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XWORK_API
#define XWORK_API
#endif

#define XWORK_VERSION_MAJOR 0
#define XWORK_VERSION_MINOR 1
#define XWORK_VERSION_PATCH 0

typedef struct xllm_runtime xllm_runtime;
typedef struct xllm_session xllm_session;
typedef struct xllm_memory xllm_memory;

typedef struct xwork_runtime xwork_runtime;
typedef struct xwork_workspace xwork_workspace;
typedef struct xwork_run xwork_run;

typedef enum {
    XWORK_OK = 0,
    XWORK_ERROR_INVALID_ARGUMENT,
    XWORK_ERROR_NO_MEMORY,
    XWORK_ERROR_ALREADY_EXISTS,
    XWORK_ERROR_NOT_FOUND,
    XWORK_ERROR_INVALID_STATE,
    XWORK_ERROR_UNSUPPORTED
} xwork_status;

typedef enum {
    XWORK_AUTONOMY_MANUAL = 0,
    XWORK_AUTONOMY_SEMI_AUTO,
    XWORK_AUTONOMY_AUTO
} xwork_autonomy_mode;

typedef enum {
    XWORK_RUN_CREATED = 0,
    XWORK_RUN_READY,
    XWORK_RUN_RUNNING,
    XWORK_RUN_WAITING_APPROVAL,
    XWORK_RUN_WAITING_TOOL,
    XWORK_RUN_CHECKPOINTING,
    XWORK_RUN_PAUSED,
    XWORK_RUN_COMPLETED,
    XWORK_RUN_CANCELLED,
    XWORK_RUN_FAILED
} xwork_run_state;

typedef enum {
    XWORK_TOOL_HOST_SERVICE = 0,
    XWORK_TOOL_VIRTUAL,
    XWORK_TOOL_SUBTASK
} xwork_tool_kind;

typedef enum {
    XWORK_SIDE_EFFECT_READ_ONLY = 0,
    XWORK_SIDE_EFFECT_WORKSPACE_WRITE,
    XWORK_SIDE_EFFECT_PROCESS_EXEC,
    XWORK_SIDE_EFFECT_NETWORK_ACCESS,
    XWORK_SIDE_EFFECT_EXTERNAL_MUTATION
} xwork_tool_side_effect;

typedef enum {
    XWORK_APPROVAL_DEFAULT = 0,
    XWORK_APPROVAL_NEVER,
    XWORK_APPROVAL_ALWAYS,
    XWORK_APPROVAL_ON_DEMAND
} xwork_approval_mode;

typedef struct {
    xllm_runtime *pLlmRuntime;
    void *pUserData;
} xwork_runtime_options;

typedef struct {
    const char *sWorkspaceId;
    const char *sRootPath;
    bool bEnableMemory;
    xllm_memory *pMemory;
} xwork_workspace_options;

typedef struct {
    const char *sToolId;
    const char *sDisplayName;
    const char *sDescription;
    xwork_tool_kind eKind;
    xwork_tool_side_effect eSideEffect;
    xwork_approval_mode eApprovalMode;
    bool bSupportsStreaming;
} xwork_tool_def;

typedef struct {
    const char *sRunId;
    const char *sParentRunId;
    const char *sInstruction;
    const char *sLlmProfileId;
    const char *sSessionProfileId;
    const char **psWorkspaceIds;
    size_t iWorkspaceCount;
    xwork_autonomy_mode eAutonomy;
} xwork_run_options;

typedef struct {
    const char *sRunId;
    const char *sParentRunId;
    const char *sInstruction;
    xwork_autonomy_mode eAutonomy;
    xwork_run_state eState;
    size_t iWorkspaceCount;
} xwork_run_summary;

XWORK_API void xwork_runtime_options_init(xwork_runtime_options *pOptions);
XWORK_API void xwork_workspace_options_init(xwork_workspace_options *pOptions);
XWORK_API void xwork_tool_def_init(xwork_tool_def *pDef);
XWORK_API void xwork_run_options_init(xwork_run_options *pOptions);

XWORK_API xwork_status xwork_runtime_create(
    const xwork_runtime_options *pOptions,
    xwork_runtime **ppRuntime
);
XWORK_API void xwork_runtime_destroy(xwork_runtime *pRuntime);

XWORK_API xllm_runtime *xwork_runtime_get_llm_runtime(const xwork_runtime *pRuntime);
XWORK_API size_t xwork_runtime_get_workspace_count(const xwork_runtime *pRuntime);
XWORK_API size_t xwork_runtime_get_tool_count(const xwork_runtime *pRuntime);
XWORK_API size_t xwork_runtime_get_run_count(const xwork_runtime *pRuntime);

XWORK_API xwork_status xwork_runtime_add_workspace(
    xwork_runtime *pRuntime,
    const xwork_workspace_options *pOptions,
    xwork_workspace **ppWorkspace
);
XWORK_API xwork_workspace *xwork_runtime_find_workspace(
    const xwork_runtime *pRuntime,
    const char *sWorkspaceId
);
XWORK_API void xwork_workspace_destroy(xwork_workspace *pWorkspace);
XWORK_API const char *xwork_workspace_get_id(const xwork_workspace *pWorkspace);
XWORK_API const char *xwork_workspace_get_root_path(const xwork_workspace *pWorkspace);
XWORK_API bool xwork_workspace_is_memory_enabled(const xwork_workspace *pWorkspace);
XWORK_API xllm_memory *xwork_workspace_get_memory(const xwork_workspace *pWorkspace);

XWORK_API xwork_status xwork_runtime_register_tool(
    xwork_runtime *pRuntime,
    const xwork_tool_def *pDef
);
XWORK_API const xwork_tool_def *xwork_runtime_find_tool(
    const xwork_runtime *pRuntime,
    const char *sToolId
);

XWORK_API xwork_status xwork_run_create(
    xwork_runtime *pRuntime,
    const xwork_run_options *pOptions,
    xwork_run **ppRun
);
XWORK_API void xwork_run_destroy(xwork_run *pRun);

XWORK_API xwork_status xwork_run_start(xwork_run *pRun);
XWORK_API xwork_status xwork_run_set_waiting_approval(xwork_run *pRun);
XWORK_API xwork_status xwork_run_set_waiting_tool(xwork_run *pRun);
XWORK_API xwork_status xwork_run_set_paused(xwork_run *pRun);
XWORK_API xwork_status xwork_run_complete(xwork_run *pRun);
XWORK_API xwork_status xwork_run_cancel(xwork_run *pRun);
XWORK_API xwork_status xwork_run_fail(xwork_run *pRun);

XWORK_API const char *xwork_run_get_id(const xwork_run *pRun);
XWORK_API const char *xwork_run_get_instruction(const xwork_run *pRun);
XWORK_API xwork_run_state xwork_run_get_state(const xwork_run *pRun);
XWORK_API xwork_autonomy_mode xwork_run_get_autonomy(const xwork_run *pRun);
XWORK_API size_t xwork_run_get_workspace_count(const xwork_run *pRun);
XWORK_API const char *xwork_run_get_workspace_id(const xwork_run *pRun, size_t iIndex);
XWORK_API xwork_status xwork_run_get_summary(const xwork_run *pRun, xwork_run_summary *pSummary);

#ifdef __cplusplus
}
#endif

#endif
