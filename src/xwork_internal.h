#ifndef XWORK_INTERNAL_H
#define XWORK_INTERNAL_H

#include "../xwork.h"
#include "xrt.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#else
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#endif

typedef struct xwork_buf {
    char* pData;
    size_t iLen;
    size_t iCap;
} xwork_buf;

typedef struct xwork_tool_entry {
    char* sName;
    char* sDescription;
    char* sParametersJson;
    char* sSource;
    bool bStrict;
    xwork_tool_effect eEffect;
    xwork_tool_execute_fn OnExecute;
    void* pUserData;
} xwork_tool_entry;

typedef struct xwork_process_entry {
    uint64_t uId;
    xprocess* pProcess;
    char* sCommand;
    uint64_t uStdoutOffset;
    uint64_t uStderrOffset;
    bool bStdinClosed;
} xwork_process_entry;

struct xwork_agent {
    xllm_client* pClient;
    xllm_session* pSession;
    xllm_memory* pMemory;
    char* sWorkspaceRoot;
    char* sSystemPrompt;
    char* sSessionPath;
    char* sArtifactDirectory;
    char* sModel;
    char* sReasoningEffort;
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
    uint32_t uMaxAgentTurns;
    uint32_t uRepeatedToolBatchLimit;
    uint32_t uConsecutiveFailureLimit;
    uint32_t uMaxManagedProcesses;
    uint32_t uCompletionVerificationRetries;
    uint32_t uCompactionQualityRetries;
    size_t iMaxInlineToolBytes;
    size_t iMaxCapturedCommandBytes;
    uint32_t uMemoryMaxHitsPerLayer;
    size_t iMemoryMaxContextBytesPerLayer;
    xllm_memory_sensitivity eMemoryMaximumSensitivity;
    bool bAutoSaveSession;
    bool bRequireVerificationAfterWrite;
    bool bRetrieveMemory;
    volatile long iCancelled;
    bool bRunning;

    xwork_tool_entry* pTools;
    size_t iToolCount;
    size_t iToolCap;
    uint64_t uToolRegistryGeneration;
    xwork_process_entry* pProcesses;
    size_t iProcessCount;
    size_t iProcessCap;
    uint64_t uNextProcessId;
    uint64_t uArtifactSequence;
    uint64_t uRunSequence;
};

char* xwork__strdup(const char* sText);
bool xwork__replace(char** ppDst, const char* sText);
void xwork__set_error(xwork_error* pError, xwork_error_code eCode, const char* sMessage);
void xwork__copy_model_error(xwork_error* pError, const xllm_error* pModelError);
bool xwork__buf_reserve(xwork_buf* pBuf, size_t iNeed);
bool xwork__buf_append(xwork_buf* pBuf, const void* pData, size_t iLen);
bool xwork__buf_append_cstr(xwork_buf* pBuf, const char* sText);
bool xwork__buf_append_char(xwork_buf* pBuf, char ch);
bool xwork__buf_appendf(xwork_buf* pBuf, const char* sFormat, ...);
char* xwork__buf_detach(xwork_buf* pBuf);
void xwork__buf_unit(xwork_buf* pBuf);
bool xwork__json_string(xwork_buf* pBuf, const char* sText);

xvalue xwork__json_parse_object(const char* sJson);
xvalue xwork__json_get(xvalue tObject, const char* sKey);
const char* xwork__json_text(xvalue tObject, const char* sKey);
bool xwork__json_bool(xvalue tObject, const char* sKey, bool bDefault, bool* pValid);
uint64_t xwork__json_u64(xvalue tObject, const char* sKey, uint64_t uDefault, bool* pValid);

char* xwork__resolve_path(const xwork_agent* pAgent, const char* sPath, xwork_error* pError);
char* xwork__relative_path(const xwork_agent* pAgent, const char* sPath);
bool xwork__ensure_parent(const char* sPath);
bool xwork__emit(xwork_agent* pAgent, const xwork_event* pEvent);
bool xwork__save(xwork_agent* pAgent, xwork_error* pError);
const xwork_tool_entry* xwork__find_tool(const xwork_agent* pAgent, const char* sName);
void xwork__processes_unit(xwork_agent* pAgent);

xwork_result xwork__execute_tool(
    xwork_agent* pAgent,
    const xllm_tool_call* pCall,
    uint64_t uTurn,
    char** ppSessionContent,
    bool* pbSuccess,
    bool* pbEffectApplied,
    xwork_error* pError
);

xllm_result xwork__model_complete(
    xwork_agent* pAgent,
    const xllm_request* pRequest,
    const xllm_stream_callbacks* pCallbacks,
    xllm_response** ppResponse,
    xllm_error* pError
);

static inline long xwork__atomic_load(volatile long* pValue)
{
#if defined(_MSC_VER)
    return _InterlockedCompareExchange(pValue, 0, 0);
#elif defined(__GNUC__) || defined(__clang__)
    return __atomic_load_n(pValue, __ATOMIC_SEQ_CST);
#else
    return *pValue;
#endif
}

static inline void xwork__atomic_store(volatile long* pValue, long iValue)
{
#if defined(_MSC_VER)
    (void)_InterlockedExchange(pValue, iValue);
#elif defined(__GNUC__) || defined(__clang__)
    __atomic_store_n(pValue, iValue, __ATOMIC_SEQ_CST);
#else
    *pValue = iValue;
#endif
}

static inline bool xwork__is_cancelled(xwork_agent* pAgent)
{
    return pAgent && (xwork__atomic_load(&pAgent->iCancelled) != 0 ||
        (pAgent->pContext && xrtContextDone(pAgent->pContext)));
}

static inline xctx_status xwork__context_status(xwork_agent* pAgent)
{
    return pAgent && pAgent->pContext ? xrtContextStatus(pAgent->pContext) : XCTX_ACTIVE;
}

#endif
