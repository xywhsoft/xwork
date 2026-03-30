#include "xwork_internal.h"

char *xwork__dup_cstr(const char *sText)
{
    char *sCopy;
    size_t iLen;

    if ( !sText ) {
        return NULL;
    }

    iLen = strlen(sText);
    sCopy = (char *)calloc(iLen + 1u, sizeof(char));
    if ( !sCopy ) {
        return NULL;
    }

    memcpy(sCopy, sText, iLen);
    sCopy[iLen] = '\0';
    return sCopy;
}

void xwork__free_cstr(char **psText)
{
    if ( psText && *psText ) {
        free(*psText);
        *psText = NULL;
    }
}

void xwork__free_str_array(char ***ppsItems, size_t *piItemCount)
{
    size_t i;

    if ( !ppsItems || !*ppsItems ) {
        if ( piItemCount ) {
            *piItemCount = 0u;
        }
        return;
    }

    if ( piItemCount ) {
        for ( i = 0u; i < *piItemCount; ++i ) {
            free((*ppsItems)[i]);
        }
        *piItemCount = 0u;
    }
    free(*ppsItems);
    *ppsItems = NULL;
}

bool xwork__runtime_has_workspace(const xwork_runtime *pRuntime, const char *sWorkspaceId)
{
    return xwork_runtime_find_workspace(pRuntime, sWorkspaceId) != NULL;
}

bool xwork__run_state_is_terminal(xwork_run_state eState)
{
    return eState == XWORK_RUN_COMPLETED ||
           eState == XWORK_RUN_CANCELLED ||
           eState == XWORK_RUN_FAILED;
}

static void xwork__free_tool_record(xwork_tool_record *pTool)
{
    if ( !pTool ) {
        return;
    }

    xwork__free_cstr(&pTool->sToolId);
    xwork__free_cstr(&pTool->sDisplayName);
    xwork__free_cstr(&pTool->sDescription);
    memset(&pTool->tDef, 0, sizeof(pTool->tDef));
    free(pTool);
}

void xwork_runtime_options_init(xwork_runtime_options *pOptions)
{
    if ( pOptions ) {
        memset(pOptions, 0, sizeof(*pOptions));
    }
}

void xwork_workspace_options_init(xwork_workspace_options *pOptions)
{
    if ( pOptions ) {
        memset(pOptions, 0, sizeof(*pOptions));
    }
}

void xwork_tool_def_init(xwork_tool_def *pDef)
{
    if ( pDef ) {
        memset(pDef, 0, sizeof(*pDef));
        pDef->eKind = XWORK_TOOL_HOST_SERVICE;
        pDef->eSideEffect = XWORK_SIDE_EFFECT_READ_ONLY;
        pDef->eApprovalMode = XWORK_APPROVAL_DEFAULT;
    }
}

void xwork_run_options_init(xwork_run_options *pOptions)
{
    if ( pOptions ) {
        memset(pOptions, 0, sizeof(*pOptions));
        pOptions->eAutonomy = XWORK_AUTONOMY_SEMI_AUTO;
    }
}

xwork_status xwork_runtime_create(
    const xwork_runtime_options *pOptions,
    xwork_runtime **ppRuntime
)
{
    xwork_runtime *pRuntime;

    if ( !ppRuntime ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppRuntime = NULL;
    pRuntime = (xwork_runtime *)calloc(1u, sizeof(*pRuntime));
    if ( !pRuntime ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    if ( pOptions ) {
        pRuntime->pLlmRuntime = pOptions->pLlmRuntime;
        pRuntime->pUserData = pOptions->pUserData;
    }

    *ppRuntime = pRuntime;
    return XWORK_OK;
}

void xwork_runtime_destroy(xwork_runtime *pRuntime)
{
    xwork_workspace *pWorkspace;
    xwork_tool_record *pTool;
    xwork_run *pRun;

    if ( !pRuntime ) {
        return;
    }

    while ( pRuntime->pRuns ) {
        pRun = pRuntime->pRuns;
        pRuntime->pRuns = pRun->pNext;
        xwork_run_destroy(pRun);
    }

    while ( pRuntime->pWorkspaces ) {
        pWorkspace = pRuntime->pWorkspaces;
        pRuntime->pWorkspaces = pWorkspace->pNext;
        xwork_workspace_destroy(pWorkspace);
    }

    while ( pRuntime->pTools ) {
        pTool = pRuntime->pTools;
        pRuntime->pTools = pTool->pNext;
        xwork__free_tool_record(pTool);
    }

    free(pRuntime);
}

xllm_runtime *xwork_runtime_get_llm_runtime(const xwork_runtime *pRuntime)
{
    return pRuntime ? pRuntime->pLlmRuntime : NULL;
}

size_t xwork_runtime_get_workspace_count(const xwork_runtime *pRuntime)
{
    size_t iCount = 0u;
    const xwork_workspace *pCursor;

    if ( !pRuntime ) {
        return 0u;
    }

    for ( pCursor = pRuntime->pWorkspaces; pCursor; pCursor = pCursor->pNext ) {
        ++iCount;
    }
    return iCount;
}

size_t xwork_runtime_get_tool_count(const xwork_runtime *pRuntime)
{
    size_t iCount = 0u;
    const xwork_tool_record *pCursor;

    if ( !pRuntime ) {
        return 0u;
    }

    for ( pCursor = pRuntime->pTools; pCursor; pCursor = pCursor->pNext ) {
        ++iCount;
    }
    return iCount;
}

size_t xwork_runtime_get_run_count(const xwork_runtime *pRuntime)
{
    size_t iCount = 0u;
    const xwork_run *pCursor;

    if ( !pRuntime ) {
        return 0u;
    }

    for ( pCursor = pRuntime->pRuns; pCursor; pCursor = pCursor->pNext ) {
        ++iCount;
    }
    return iCount;
}

static xwork_status xwork__copy_workspace_ids(
    const xwork_run_options *pOptions,
    char ***ppsWorkspaceIds,
    size_t *piWorkspaceCount
)
{
    size_t i;
    char **psItems = NULL;

    if ( !ppsWorkspaceIds || !piWorkspaceCount ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppsWorkspaceIds = NULL;
    *piWorkspaceCount = 0u;

    if ( !pOptions || !pOptions->psWorkspaceIds || pOptions->iWorkspaceCount == 0u ) {
        return XWORK_OK;
    }

    psItems = (char **)calloc(pOptions->iWorkspaceCount, sizeof(char *));
    if ( !psItems ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    for ( i = 0u; i < pOptions->iWorkspaceCount; ++i ) {
        psItems[i] = xwork__dup_cstr(pOptions->psWorkspaceIds[i]);
        if ( !psItems[i] ) {
            *ppsWorkspaceIds = psItems;
            *piWorkspaceCount = i;
            xwork__free_str_array(ppsWorkspaceIds, piWorkspaceCount);
            return XWORK_ERROR_NO_MEMORY;
        }
    }

    *ppsWorkspaceIds = psItems;
    *piWorkspaceCount = pOptions->iWorkspaceCount;
    return XWORK_OK;
}

xwork_status xwork_run_create(
    xwork_runtime *pRuntime,
    const xwork_run_options *pOptions,
    xwork_run **ppRun
)
{
    xwork_run *pRun;
    size_t i;
    xwork_status iStatus;

    if ( !pRuntime || !pOptions || !ppRun ||
         !pOptions->sRunId || !pOptions->sRunId[0] ||
         !pOptions->sInstruction || !pOptions->sInstruction[0] ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppRun = NULL;

    for ( i = 0u; i < pOptions->iWorkspaceCount; ++i ) {
        const char *sWorkspaceId = pOptions->psWorkspaceIds ? pOptions->psWorkspaceIds[i] : NULL;
        if ( !sWorkspaceId || !xwork__runtime_has_workspace(pRuntime, sWorkspaceId) ) {
            return XWORK_ERROR_NOT_FOUND;
        }
    }

    for ( pRun = pRuntime->pRuns; pRun; pRun = pRun->pNext ) {
        if ( pRun->sRunId && strcmp(pRun->sRunId, pOptions->sRunId) == 0 ) {
            return XWORK_ERROR_ALREADY_EXISTS;
        }
    }

    pRun = (xwork_run *)calloc(1u, sizeof(*pRun));
    if ( !pRun ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    pRun->pRuntime = pRuntime;
    pRun->sRunId = xwork__dup_cstr(pOptions->sRunId);
    pRun->sParentRunId = xwork__dup_cstr(pOptions->sParentRunId);
    pRun->sInstruction = xwork__dup_cstr(pOptions->sInstruction);
    pRun->sLlmProfileId = xwork__dup_cstr(pOptions->sLlmProfileId);
    pRun->sSessionProfileId = xwork__dup_cstr(pOptions->sSessionProfileId);
    pRun->eAutonomy = pOptions->eAutonomy;
    pRun->eState = XWORK_RUN_CREATED;

    if ( !pRun->sRunId || !pRun->sInstruction ) {
        xwork_run_destroy(pRun);
        return XWORK_ERROR_NO_MEMORY;
    }

    iStatus = xwork__copy_workspace_ids(pOptions, &pRun->psWorkspaceIds, &pRun->iWorkspaceCount);
    if ( iStatus != XWORK_OK ) {
        xwork_run_destroy(pRun);
        return iStatus;
    }

    pRun->pNext = pRuntime->pRuns;
    pRuntime->pRuns = pRun;
    *ppRun = pRun;
    return XWORK_OK;
}

void xwork_run_destroy(xwork_run *pRun)
{
    xwork_run **ppCursor;

    if ( !pRun ) {
        return;
    }

    if ( pRun->pRuntime ) {
        ppCursor = &pRun->pRuntime->pRuns;
        while ( *ppCursor ) {
            if ( *ppCursor == pRun ) {
                *ppCursor = pRun->pNext;
                break;
            }
            ppCursor = &(*ppCursor)->pNext;
        }
    }

    xwork__free_cstr(&pRun->sRunId);
    xwork__free_cstr(&pRun->sParentRunId);
    xwork__free_cstr(&pRun->sInstruction);
    xwork__free_cstr(&pRun->sLlmProfileId);
    xwork__free_cstr(&pRun->sSessionProfileId);
    xwork__free_str_array(&pRun->psWorkspaceIds, &pRun->iWorkspaceCount);
    free(pRun);
}

static xwork_status xwork__run_transition(xwork_run *pRun, xwork_run_state eNextState)
{
    if ( !pRun ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( xwork__run_state_is_terminal(pRun->eState) ) {
        return XWORK_ERROR_INVALID_STATE;
    }
    pRun->eState = eNextState;
    return XWORK_OK;
}

xwork_status xwork_run_start(xwork_run *pRun)
{
    if ( !pRun ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( pRun->eState != XWORK_RUN_CREATED &&
         pRun->eState != XWORK_RUN_READY &&
         pRun->eState != XWORK_RUN_PAUSED ) {
        return XWORK_ERROR_INVALID_STATE;
    }
    pRun->eState = XWORK_RUN_RUNNING;
    return XWORK_OK;
}

xwork_status xwork_run_set_waiting_approval(xwork_run *pRun)
{
    return xwork__run_transition(pRun, XWORK_RUN_WAITING_APPROVAL);
}

xwork_status xwork_run_set_waiting_tool(xwork_run *pRun)
{
    return xwork__run_transition(pRun, XWORK_RUN_WAITING_TOOL);
}

xwork_status xwork_run_set_paused(xwork_run *pRun)
{
    return xwork__run_transition(pRun, XWORK_RUN_PAUSED);
}

xwork_status xwork_run_complete(xwork_run *pRun)
{
    return xwork__run_transition(pRun, XWORK_RUN_COMPLETED);
}

xwork_status xwork_run_cancel(xwork_run *pRun)
{
    return xwork__run_transition(pRun, XWORK_RUN_CANCELLED);
}

xwork_status xwork_run_fail(xwork_run *pRun)
{
    return xwork__run_transition(pRun, XWORK_RUN_FAILED);
}

const char *xwork_run_get_id(const xwork_run *pRun)
{
    return pRun ? pRun->sRunId : NULL;
}

const char *xwork_run_get_instruction(const xwork_run *pRun)
{
    return pRun ? pRun->sInstruction : NULL;
}

xwork_run_state xwork_run_get_state(const xwork_run *pRun)
{
    return pRun ? pRun->eState : XWORK_RUN_FAILED;
}

xwork_autonomy_mode xwork_run_get_autonomy(const xwork_run *pRun)
{
    return pRun ? pRun->eAutonomy : XWORK_AUTONOMY_MANUAL;
}

size_t xwork_run_get_workspace_count(const xwork_run *pRun)
{
    return pRun ? pRun->iWorkspaceCount : 0u;
}

const char *xwork_run_get_workspace_id(const xwork_run *pRun, size_t iIndex)
{
    if ( !pRun || iIndex >= pRun->iWorkspaceCount ) {
        return NULL;
    }
    return pRun->psWorkspaceIds ? pRun->psWorkspaceIds[iIndex] : NULL;
}

xwork_status xwork_run_get_summary(const xwork_run *pRun, xwork_run_summary *pSummary)
{
    if ( !pRun || !pSummary ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    memset(pSummary, 0, sizeof(*pSummary));
    pSummary->sRunId = pRun->sRunId;
    pSummary->sParentRunId = pRun->sParentRunId;
    pSummary->sInstruction = pRun->sInstruction;
    pSummary->eAutonomy = pRun->eAutonomy;
    pSummary->eState = pRun->eState;
    pSummary->iWorkspaceCount = pRun->iWorkspaceCount;
    return XWORK_OK;
}
