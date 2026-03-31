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

#define XWORK_PROFILE_XCODE "xcode"
#define XWORK_PROFILE_XCLAW "xclaw"

typedef struct xllm_runtime xllm_runtime;
typedef struct xllm_session xllm_session;
typedef struct xllm_memory xllm_memory;

typedef struct xwork_runtime xwork_runtime;
typedef struct xwork_workspace xwork_workspace;
typedef struct xwork_run xwork_run;
typedef struct xwork_event xwork_event;
typedef struct xwork_approval_request xwork_approval_request;
typedef struct xwork_checkpoint xwork_checkpoint;
typedef struct xwork_artifact xwork_artifact;
typedef struct xwork_profile xwork_profile;
typedef struct xwork_run_snapshot xwork_run_snapshot;
typedef struct xwork_persistence_backend xwork_persistence_backend;
typedef struct xwork_run_index_list xwork_run_index_list;
typedef struct xwork_run_index_query xwork_run_index_query;

typedef enum {
    XWORK_OK = 0,
    XWORK_ERROR_INVALID_ARGUMENT,
    XWORK_ERROR_NO_MEMORY,
    XWORK_ERROR_ALREADY_EXISTS,
    XWORK_ERROR_NOT_FOUND,
    XWORK_ERROR_INVALID_STATE,
    XWORK_ERROR_EXTERNAL_FAILURE,
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

typedef enum {
    XWORK_RISK_LOW = 0,
    XWORK_RISK_MEDIUM,
    XWORK_RISK_HIGH,
    XWORK_RISK_CRITICAL
} xwork_risk_level;

typedef enum {
    XWORK_APPROVAL_PENDING = 0,
    XWORK_APPROVAL_APPROVED,
    XWORK_APPROVAL_REJECTED,
    XWORK_APPROVAL_CANCELLED
} xwork_approval_state;

typedef enum {
    XWORK_CHECKPOINT_MANUAL = 0,
    XWORK_CHECKPOINT_MODEL_TURN,
    XWORK_CHECKPOINT_BEFORE_TOOL,
    XWORK_CHECKPOINT_AFTER_TOOL,
    XWORK_CHECKPOINT_COMPLETION
} xwork_checkpoint_kind;

typedef enum {
    XWORK_ARTIFACT_PATCH = 0,
    XWORK_ARTIFACT_REPORT,
    XWORK_ARTIFACT_COMMAND,
    XWORK_ARTIFACT_OUTPUT
} xwork_artifact_kind;

typedef enum {
    XWORK_EVENT_NONE = 0,
    XWORK_EVENT_RUN_CREATED,
    XWORK_EVENT_RUN_STARTED,
    XWORK_EVENT_MEMORY_CONTEXT_ATTACHED,
    XWORK_EVENT_MEMORY_RECORD_INGESTED,
    XWORK_EVENT_MODEL_TURN_STARTED,
    XWORK_EVENT_MODEL_TURN_COMPLETED,
    XWORK_EVENT_TOOL_CALL_REQUESTED,
    XWORK_EVENT_APPROVAL_REQUESTED,
    XWORK_EVENT_APPROVAL_RESOLVED,
    XWORK_EVENT_TOOL_EXEC_STARTED,
    XWORK_EVENT_TOOL_EXEC_COMPLETED,
    XWORK_EVENT_ARTIFACT_EMITTED,
    XWORK_EVENT_CHECKPOINT_SAVED,
    XWORK_EVENT_CHECKPOINT_LOADED,
    XWORK_EVENT_RUN_PAUSED,
    XWORK_EVENT_RUN_COMPLETED,
    XWORK_EVENT_RUN_CANCELLED,
    XWORK_EVENT_RUN_FAILED
} xwork_event_kind;

typedef struct {
    const char *sCallId;
    const char *sToolId;
    const char *sArgumentsJson;
} xwork_tool_call;

typedef struct {
    const char *sOutputText;
    const char *sVisibleSummary;
    bool bRetryable;
} xwork_tool_result;

typedef enum {
    XWORK_HOST_NONE = 0,
    XWORK_HOST_FILESYSTEM,
    XWORK_HOST_PROCESS,
    XWORK_HOST_VCS,
    XWORK_HOST_DIAGNOSTICS,
    XWORK_HOST_EDITOR
} xwork_host_service_kind;

typedef xwork_status (*xwork_host_invoke_fn)(
    const char *sOperationId,
    const char *sRequestJson,
    xwork_tool_result *pResult,
    void *pUserData
);

typedef struct {
    xwork_host_invoke_fn pfnInvoke;
    void *pUserData;
} xwork_host_service;

typedef struct {
    xwork_host_service tFilesystem;
    xwork_host_service tProcess;
    xwork_host_service tVcs;
    xwork_host_service tDiagnostics;
    xwork_host_service tEditor;
} xwork_host_services;

typedef struct {
    xwork_risk_level eAutoApproveRiskLimit;
} xwork_policy_options;

typedef struct {
    xwork_autonomy_mode eAutonomy;
    xwork_approval_mode eApprovalMode;
    xwork_tool_side_effect eSideEffect;
    bool bAutoApproveRequested;
} xwork_approval_eval_input;

typedef struct {
    bool bRequiresApproval;
    bool bAutoApproved;
    xwork_risk_level eRiskLevel;
    const char *sScope;
    const char *sReason;
} xwork_approval_decision;

typedef struct {
    const char *sText;
    size_t iWorkspaceCount;
} xwork_memory_context;

typedef struct {
    bool bEnableAutoCompact;
    double fCompactTriggerRatio;
    size_t iCompactTriggerTurns;
} xwork_session_policy;

struct xwork_profile {
    const char *sProfileId;
    const char *sDisplayName;
    const char *sDescription;
    const char *sDefaultLlmProfileId;
    const char *sDefaultSessionProfileId;
    xwork_autonomy_mode eAutonomy;
    xwork_policy_options tPolicy;
    xwork_session_policy tSessionPolicy;
    size_t iDefaultMaxTurns;
    bool bDefaultAutoApprove;
    bool bEnableWorkspaceMemory;
};

typedef struct {
    xllm_runtime *pLlmRuntime;
    const xwork_host_services *pHostServices;
    const xwork_persistence_backend *pPersistenceBackend;
    xwork_policy_options tPolicy;
    void *pUserData;
} xwork_runtime_options;

typedef xwork_status (*xwork_tool_exec_fn)(
    xwork_run *pRun,
    const xwork_tool_call *pCall,
    xwork_tool_result *pResult,
    void *pUserData
);

typedef xwork_status (*xwork_memory_resolve_fn)(
    const xwork_run *pRun,
    xwork_memory_context *pContext,
    void *pUserData
);

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
    xwork_host_service_kind eHostService;
    const char *sOperationId;
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
    xwork_session_policy tSessionPolicy;
} xwork_run_options;

typedef struct {
    const char *sArtifactId;
    xwork_artifact_kind eKind;
    const char *sName;
    const char *sMimeType;
    const char *sStorageRef;
    const char *sSummary;
} xwork_artifact_options;

typedef struct {
    const char *sRunId;
    const char *sParentRunId;
    const char *sInstruction;
    xwork_autonomy_mode eAutonomy;
    xwork_run_state eState;
    size_t iWorkspaceCount;
} xwork_run_summary;

typedef struct {
    const xwork_run_summary *pItems;
    size_t iCount;
} xwork_run_summary_list;

struct xwork_run_snapshot {
    const char *sRunId;
    const char *sParentRunId;
    const char *sInstruction;
    const char *sLlmProfileId;
    const char *sSessionProfileId;
    const char *sSessionStateData;
    const char **psWorkspaceIds;
    size_t iWorkspaceCount;
    xwork_autonomy_mode eAutonomy;
    xwork_session_policy tSessionPolicy;
    xwork_run_state eState;
    const char *sLastOutputText;
    bool bHasMemoryContext;
    const char *sLastMemoryContextText;
    size_t iLastMemoryWorkspaceCount;
    bool bHasToolCall;
    const char *sLastToolCallId;
    const char *sLastToolId;
    const char *sLastToolArgumentsJson;
    bool bHasToolResult;
    const char *sLastToolResultText;
    const char *sLastToolVisibleSummary;
    bool bHasApprovalRequest;
    const char *sLastApprovalRequestId;
    const char *sLastApprovalToolId;
    const char *sLastApprovalReason;
    const char *sLastApprovalScope;
    const char *sLastApprovalActionSummary;
    xwork_risk_level eLastApprovalRiskLevel;
    xwork_approval_state eLastApprovalState;
    size_t iLastApprovalSequence;
    bool bHasCheckpoint;
    const char *sLastCheckpointId;
    const char *sLastCheckpointPendingStep;
    const char *sLastCheckpointSessionStateRef;
    const char *sLastCheckpointToolOutputsRef;
    const char *sLastCheckpointWorkspaceSnapshotRef;
    const char *sLastCheckpointArtifactRefs;
    xwork_checkpoint_kind eLastCheckpointKind;
    xwork_run_state eLastCheckpointRunState;
    size_t iLastCheckpointSequence;
    const xwork_artifact *pArtifacts;
    size_t iArtifactCount;
    size_t iNextEventSequence;
    size_t iNextArtifactSequence;
    size_t iNextApprovalSequence;
    size_t iNextCheckpointSequence;
};

typedef struct {
    const char *sRootPath;
} xwork_file_persistence_options;

typedef struct {
    char *sRootPath;
} xwork_file_persistence;

typedef struct {
    const char **psItems;
    size_t iCount;
} xwork_string_list;

typedef xwork_status (*xwork_persistence_store_event_fn)(
    const xwork_event *pEvent,
    void *pUserData
);

typedef xwork_status (*xwork_persistence_store_checkpoint_fn)(
    const xwork_checkpoint *pCheckpoint,
    const xwork_run_snapshot *pSnapshot,
    void *pUserData
);

typedef xwork_status (*xwork_persistence_store_artifact_fn)(
    const xwork_artifact *pArtifact,
    void *pUserData
);

typedef xwork_status (*xwork_persistence_load_run_snapshot_fn)(
    const char *sRunId,
    xwork_run_snapshot *pSnapshot,
    void *pUserData
);

typedef xwork_status (*xwork_persistence_load_checkpoint_snapshot_fn)(
    const char *sRunId,
    const char *sCheckpointId,
    xwork_run_snapshot *pSnapshot,
    void *pUserData
);

typedef xwork_status (*xwork_persistence_list_runs_fn)(
    xwork_string_list *pList,
    void *pUserData
);

typedef xwork_status (*xwork_persistence_query_run_index_fn)(
    const xwork_run_index_query *pQuery,
    xwork_run_index_list *pList,
    void *pUserData
);

struct xwork_persistence_backend {
    xwork_persistence_store_event_fn pfnStoreEvent;
    xwork_persistence_store_checkpoint_fn pfnStoreCheckpoint;
    xwork_persistence_store_artifact_fn pfnStoreArtifact;
    xwork_persistence_load_run_snapshot_fn pfnLoadRunSnapshot;
    xwork_persistence_load_checkpoint_snapshot_fn pfnLoadCheckpointSnapshot;
    xwork_persistence_list_runs_fn pfnListRuns;
    xwork_persistence_query_run_index_fn pfnQueryRunIndex;
    void *pUserData;
};

struct xwork_event {
    const char *sEventId;
    const char *sRunId;
    const char *sToolId;
    const char *sApprovalRequestId;
    const char *sCheckpointId;
    const char *sSummary;
    xwork_event_kind eKind;
    xwork_run_state eRunState;
    size_t iSequence;
};

struct xwork_approval_request {
    const char *sRequestId;
    const char *sRunId;
    const char *sToolId;
    const char *sReason;
    const char *sScope;
    const char *sActionSummary;
    xwork_risk_level eRiskLevel;
    xwork_approval_state eState;
};

struct xwork_checkpoint {
    const char *sCheckpointId;
    const char *sRunId;
    const char *sPendingStep;
    const char *sSessionStateRef;
    const char *sToolOutputsRef;
    const char *sWorkspaceSnapshotRef;
    const char *sArtifactRefs;
    xwork_checkpoint_kind eKind;
    xwork_run_state eRunState;
    size_t iSequence;
};

struct xwork_artifact {
    const char *sArtifactId;
    const char *sRunId;
    xwork_artifact_kind eKind;
    const char *sName;
    const char *sMimeType;
    const char *sStorageRef;
    const char *sSummary;
    size_t iSequence;
};

typedef struct xwork_run_index_entry {
    xwork_run_summary tSummary;
    bool bHasLastApprovalRequest;
    xwork_approval_request tLastApprovalRequest;
    bool bHasLastEvent;
    xwork_event tLastEvent;
    bool bHasLastCheckpoint;
    xwork_checkpoint tLastCheckpoint;
    size_t iArtifactCount;
    bool bHasLastArtifact;
    xwork_artifact tLastArtifact;
} xwork_run_index_entry;

struct xwork_run_index_list {
    const xwork_run_index_entry *pItems;
    size_t iCount;
};

typedef enum {
    XWORK_RUN_INDEX_SORT_RUN_ID_ASC = 0,
    XWORK_RUN_INDEX_SORT_RUN_ID_DESC,
    XWORK_RUN_INDEX_SORT_LAST_EVENT_SEQUENCE_DESC,
    XWORK_RUN_INDEX_SORT_LAST_CHECKPOINT_SEQUENCE_DESC
} xwork_run_index_sort;

struct xwork_run_index_query {
    bool bFilterState;
    xwork_run_state eState;
    bool bFilterAutonomy;
    xwork_autonomy_mode eAutonomy;
    bool bFilterLastApprovalState;
    xwork_approval_state eLastApprovalState;
    bool bRequireLastApprovalRequest;
    bool bRequireLastCheckpoint;
    bool bRequireArtifacts;
    xwork_run_index_sort eSort;
};

typedef struct {
    xwork_tool_exec_fn pfnToolExec;
    void *pUserData;
    xwork_memory_resolve_fn pfnResolveMemoryContext;
    void *pMemoryUserData;
    bool bIngestToolResultsToMemory;
    bool bIngestArtifactsToMemory;
    size_t iMaxTurns;
    bool bAutoApprove;
} xwork_orchestrator_options;

XWORK_API void xwork_runtime_options_init(xwork_runtime_options *pOptions);
XWORK_API void xwork_workspace_options_init(xwork_workspace_options *pOptions);
XWORK_API void xwork_tool_def_init(xwork_tool_def *pDef);
XWORK_API void xwork_run_options_init(xwork_run_options *pOptions);
XWORK_API void xwork_run_summary_init(xwork_run_summary *pSummary);
XWORK_API void xwork_run_summary_reset(xwork_run_summary *pSummary);
XWORK_API void xwork_run_summary_list_init(xwork_run_summary_list *pList);
XWORK_API void xwork_run_summary_list_reset(xwork_run_summary_list *pList);
XWORK_API void xwork_run_index_entry_init(xwork_run_index_entry *pEntry);
XWORK_API void xwork_run_index_entry_reset(xwork_run_index_entry *pEntry);
XWORK_API void xwork_run_index_list_init(xwork_run_index_list *pList);
XWORK_API void xwork_run_index_list_reset(xwork_run_index_list *pList);
XWORK_API void xwork_run_index_query_init(xwork_run_index_query *pQuery);
XWORK_API void xwork_host_service_init(xwork_host_service *pService);
XWORK_API void xwork_host_services_init(xwork_host_services *pServices);
XWORK_API void xwork_policy_options_init(xwork_policy_options *pOptions);
XWORK_API void xwork_approval_eval_input_init(xwork_approval_eval_input *pInput);
XWORK_API void xwork_approval_decision_init(xwork_approval_decision *pDecision);
XWORK_API void xwork_memory_context_init(xwork_memory_context *pContext);
XWORK_API void xwork_memory_context_reset(xwork_memory_context *pContext);
XWORK_API void xwork_session_policy_init(xwork_session_policy *pPolicy);
XWORK_API void xwork_profile_init(xwork_profile *pProfile);
XWORK_API void xwork_persistence_backend_init(xwork_persistence_backend *pBackend);
XWORK_API void xwork_file_persistence_options_init(
    xwork_file_persistence_options *pOptions
);
XWORK_API void xwork_file_persistence_init(xwork_file_persistence *pStore);
XWORK_API void xwork_file_persistence_reset(xwork_file_persistence *pStore);
XWORK_API void xwork_string_list_init(xwork_string_list *pList);
XWORK_API void xwork_string_list_reset(xwork_string_list *pList);
XWORK_API void xwork_event_init(xwork_event *pEvent);
XWORK_API void xwork_event_reset(xwork_event *pEvent);
XWORK_API void xwork_approval_request_init(xwork_approval_request *pRequest);
XWORK_API void xwork_approval_request_reset(xwork_approval_request *pRequest);
XWORK_API void xwork_checkpoint_init(xwork_checkpoint *pCheckpoint);
XWORK_API void xwork_checkpoint_reset(xwork_checkpoint *pCheckpoint);
XWORK_API void xwork_artifact_options_init(xwork_artifact_options *pOptions);
XWORK_API void xwork_artifact_init(xwork_artifact *pArtifact);
XWORK_API void xwork_artifact_reset(xwork_artifact *pArtifact);
XWORK_API void xwork_run_snapshot_init(xwork_run_snapshot *pSnapshot);
XWORK_API void xwork_run_snapshot_reset(xwork_run_snapshot *pSnapshot);
XWORK_API void xwork_tool_result_init(xwork_tool_result *pResult);
XWORK_API void xwork_orchestrator_options_init(xwork_orchestrator_options *pOptions);
XWORK_API xwork_status xwork_profile_get_builtin(
    const char *sProfileId,
    xwork_profile *pProfile
);
XWORK_API xwork_status xwork_profile_apply_runtime_options(
    const xwork_profile *pProfile,
    xwork_runtime_options *pOptions
);
XWORK_API xwork_status xwork_profile_apply_workspace_options(
    const xwork_profile *pProfile,
    xwork_workspace_options *pOptions
);
XWORK_API xwork_status xwork_profile_apply_run_options(
    const xwork_profile *pProfile,
    xwork_run_options *pOptions
);
XWORK_API xwork_status xwork_profile_apply_orchestrator_options(
    const xwork_profile *pProfile,
    xwork_orchestrator_options *pOptions
);
XWORK_API xwork_status xwork_file_persistence_configure_backend(
    xwork_file_persistence *pStore,
    const xwork_file_persistence_options *pOptions,
    xwork_persistence_backend *pBackend
);
XWORK_API xwork_status xwork_file_persistence_list_runs(
    const xwork_file_persistence *pStore,
    xwork_string_list *pList
);
XWORK_API xwork_status xwork_file_persistence_list_run_summaries(
    const xwork_file_persistence *pStore,
    xwork_run_summary_list *pList
);
XWORK_API xwork_status xwork_file_persistence_list_run_index(
    const xwork_file_persistence *pStore,
    xwork_run_index_list *pList
);
XWORK_API xwork_status xwork_file_persistence_query_run_index(
    const xwork_file_persistence *pStore,
    const xwork_run_index_query *pQuery,
    xwork_run_index_list *pList
);
XWORK_API xwork_status xwork_file_persistence_list_checkpoints(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_string_list *pList
);
XWORK_API xwork_status xwork_file_persistence_list_events(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_string_list *pList
);
XWORK_API xwork_status xwork_file_persistence_list_artifacts(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_string_list *pList
);
XWORK_API xwork_status xwork_file_persistence_load_event(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const char *sEventId,
    xwork_event *pEvent
);
XWORK_API xwork_status xwork_file_persistence_load_last_event(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_event *pEvent
);
XWORK_API xwork_status xwork_file_persistence_load_run_snapshot(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_run_snapshot *pSnapshot
);
XWORK_API xwork_status xwork_file_persistence_load_checkpoint_snapshot(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const char *sCheckpointId,
    xwork_run_snapshot *pSnapshot
);
XWORK_API xwork_status xwork_file_persistence_load_last_approval_request(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_approval_request *pRequest
);
XWORK_API xwork_status xwork_file_persistence_load_run_summary(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_run_summary *pSummary
);
XWORK_API xwork_status xwork_file_persistence_load_checkpoint(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const char *sCheckpointId,
    xwork_checkpoint *pCheckpoint
);
XWORK_API xwork_status xwork_file_persistence_load_last_checkpoint(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_checkpoint *pCheckpoint
);
XWORK_API xwork_status xwork_file_persistence_load_artifact(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    const char *sArtifactId,
    xwork_artifact *pArtifact
);
XWORK_API xwork_status xwork_file_persistence_load_last_artifact(
    const xwork_file_persistence *pStore,
    const char *sRunId,
    xwork_artifact *pArtifact
);

XWORK_API xwork_status xwork_runtime_create(
    const xwork_runtime_options *pOptions,
    xwork_runtime **ppRuntime
);
XWORK_API void xwork_runtime_destroy(xwork_runtime *pRuntime);

XWORK_API xllm_runtime *xwork_runtime_get_llm_runtime(const xwork_runtime *pRuntime);
XWORK_API const xwork_host_services *xwork_runtime_get_host_services(const xwork_runtime *pRuntime);
XWORK_API const xwork_persistence_backend *xwork_runtime_get_persistence_backend(
    const xwork_runtime *pRuntime
);
XWORK_API xwork_status xwork_runtime_list_persisted_runs(
    const xwork_runtime *pRuntime,
    xwork_string_list *pList
);
XWORK_API xwork_status xwork_runtime_list_persisted_run_index(
    const xwork_runtime *pRuntime,
    xwork_run_index_list *pList
);
XWORK_API xwork_status xwork_runtime_query_persisted_run_index(
    const xwork_runtime *pRuntime,
    const xwork_run_index_query *pQuery,
    xwork_run_index_list *pList
);
XWORK_API xwork_status xwork_runtime_get_policy_options(
    const xwork_runtime *pRuntime,
    xwork_policy_options *pOptions
);
XWORK_API size_t xwork_runtime_get_workspace_count(const xwork_runtime *pRuntime);
XWORK_API size_t xwork_runtime_get_tool_count(const xwork_runtime *pRuntime);
XWORK_API size_t xwork_runtime_get_run_count(const xwork_runtime *pRuntime);
XWORK_API xwork_status xwork_runtime_invoke_host_service(
    const xwork_runtime *pRuntime,
    xwork_host_service_kind eKind,
    const char *sOperationId,
    const char *sRequestJson,
    xwork_tool_result *pResult
);

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
XWORK_API xwork_status xwork_runtime_recover_run(
    xwork_runtime *pRuntime,
    const xwork_run_snapshot *pSnapshot,
    xwork_run **ppRun
);
XWORK_API xwork_status xwork_runtime_recover_run_from_persistence(
    xwork_runtime *pRuntime,
    const char *sRunId,
    xwork_run **ppRun
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
XWORK_API xwork_status xwork_run_submit_approval(
    xwork_run *pRun,
    xwork_approval_state eDecision
);
XWORK_API xwork_status xwork_run_load_checkpoint(
    xwork_run *pRun,
    const char *sCheckpointId
);
XWORK_API xwork_status xwork_run_resume(xwork_run *pRun);
XWORK_API xwork_status xwork_run_emit_artifact(
    xwork_run *pRun,
    const xwork_artifact_options *pOptions,
    xwork_artifact *pArtifact
);
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
XWORK_API xwork_status xwork_run_get_snapshot(
    const xwork_run *pRun,
    xwork_run_snapshot *pSnapshot
);
XWORK_API xwork_status xwork_run_get_last_event(const xwork_run *pRun, xwork_event *pEvent);
XWORK_API xwork_status xwork_run_get_last_approval_request(
    const xwork_run *pRun,
    xwork_approval_request *pRequest
);
XWORK_API xwork_status xwork_run_get_last_checkpoint(
    const xwork_run *pRun,
    xwork_checkpoint *pCheckpoint
);
XWORK_API xwork_status xwork_run_get_last_memory_context(
    const xwork_run *pRun,
    xwork_memory_context *pContext
);
XWORK_API size_t xwork_run_get_event_count(const xwork_run *pRun);
XWORK_API xwork_status xwork_run_get_event(
    const xwork_run *pRun,
    size_t iIndex,
    xwork_event *pEvent
);
XWORK_API size_t xwork_run_get_checkpoint_count(const xwork_run *pRun);
XWORK_API xwork_status xwork_run_get_checkpoint(
    const xwork_run *pRun,
    size_t iIndex,
    xwork_checkpoint *pCheckpoint
);
XWORK_API size_t xwork_run_get_artifact_count(const xwork_run *pRun);
XWORK_API xwork_status xwork_run_get_artifact(
    const xwork_run *pRun,
    size_t iIndex,
    xwork_artifact *pArtifact
);
XWORK_API const char *xwork_run_get_last_output_text(const xwork_run *pRun);
XWORK_API xwork_status xwork_policy_evaluate_approval(
    const xwork_policy_options *pPolicy,
    const xwork_approval_eval_input *pInput,
    xwork_approval_decision *pDecision
);
XWORK_API xwork_status xwork_run_execute(
    xwork_run *pRun,
    const xwork_orchestrator_options *pOptions
);

#ifdef __cplusplus
}
#endif

#endif
