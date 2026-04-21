#include "../xwork_core/xwork_internal.h"
#include "../../lib/xllm-memory.h"

void xwork_workspace_memory_sync_summary_init(xwork_workspace_memory_sync_summary *pSummary)
{
    if ( pSummary ) {
        memset(pSummary, 0, sizeof(*pSummary));
    }
}

void xwork_workspace_memory_file_sync_summary_init(
    xwork_workspace_memory_file_sync_summary *pSummary
)
{
    if ( pSummary ) {
        memset(pSummary, 0, sizeof(*pSummary));
    }
}

xwork_status xwork_runtime_add_workspace(
    xwork_runtime *pRuntime,
    const xwork_workspace_options *pOptions,
    xwork_workspace **ppWorkspace
)
{
    xwork_workspace *pWorkspace;

    if ( !pRuntime || !pOptions || !ppWorkspace ||
         !pOptions->sWorkspaceId || !pOptions->sWorkspaceId[0] ||
         !pOptions->sRootPath || !pOptions->sRootPath[0] ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppWorkspace = NULL;

    if ( xwork_runtime_find_workspace(pRuntime, pOptions->sWorkspaceId) ) {
        return XWORK_ERROR_ALREADY_EXISTS;
    }

    pWorkspace = (xwork_workspace *)calloc(1u, sizeof(*pWorkspace));
    if ( !pWorkspace ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    pWorkspace->pRuntime = pRuntime;
    pWorkspace->sWorkspaceId = xwork__dup_cstr(pOptions->sWorkspaceId);
    pWorkspace->sRootPath = xwork__dup_cstr(pOptions->sRootPath);
    pWorkspace->bEnableMemory = pOptions->bEnableMemory;
    pWorkspace->pMemory = pOptions->pMemory;

    if ( !pWorkspace->sWorkspaceId || !pWorkspace->sRootPath ) {
        xwork_workspace_destroy(pWorkspace);
        return XWORK_ERROR_NO_MEMORY;
    }

    pWorkspace->pNext = pRuntime->pWorkspaces;
    pRuntime->pWorkspaces = pWorkspace;
    *ppWorkspace = pWorkspace;
    return XWORK_OK;
}

xwork_workspace *xwork_runtime_find_workspace(
    const xwork_runtime *pRuntime,
    const char *sWorkspaceId
)
{
    xwork_workspace *pCursor;

    if ( !pRuntime || !sWorkspaceId || !sWorkspaceId[0] ) {
        return NULL;
    }

    for ( pCursor = pRuntime->pWorkspaces; pCursor; pCursor = pCursor->pNext ) {
        if ( pCursor->sWorkspaceId && strcmp(pCursor->sWorkspaceId, sWorkspaceId) == 0 ) {
            return pCursor;
        }
    }
    return NULL;
}

void xwork_workspace_destroy(xwork_workspace *pWorkspace)
{
    xwork_workspace **ppCursor;

    if ( !pWorkspace ) {
        return;
    }

    if ( pWorkspace->pRuntime ) {
        ppCursor = &pWorkspace->pRuntime->pWorkspaces;
        while ( *ppCursor ) {
            if ( *ppCursor == pWorkspace ) {
                *ppCursor = pWorkspace->pNext;
                break;
            }
            ppCursor = &(*ppCursor)->pNext;
        }
    }

    xwork__free_cstr(&pWorkspace->sWorkspaceId);
    xwork__free_cstr(&pWorkspace->sRootPath);
    free(pWorkspace);
}

const char *xwork_workspace_get_id(const xwork_workspace *pWorkspace)
{
    return pWorkspace ? pWorkspace->sWorkspaceId : NULL;
}

const char *xwork_workspace_get_root_path(const xwork_workspace *pWorkspace)
{
    return pWorkspace ? pWorkspace->sRootPath : NULL;
}

bool xwork_workspace_is_memory_enabled(const xwork_workspace *pWorkspace)
{
    return pWorkspace ? pWorkspace->bEnableMemory : false;
}

xllm_memory *xwork_workspace_get_memory(const xwork_workspace *pWorkspace)
{
    return pWorkspace ? pWorkspace->pMemory : NULL;
}

xwork_status xwork_workspace_sync_memory(
    xwork_workspace *pWorkspace,
    xwork_workspace_memory_sync_summary *pSummary
)
{
    xllm_memory_ingest_workspace_options tOptions;
    xllm_memory_sync_workspace_result tResult;
    xllm_error tError;
    int iStatus;

    if ( !pWorkspace || !pWorkspace->bEnableMemory || !pWorkspace->pMemory ||
         !pWorkspace->sRootPath || !pWorkspace->sRootPath[0] ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( pSummary ) {
        xwork_workspace_memory_sync_summary_init(pSummary);
    }

    xllm_memory_ingest_workspace_options_init(&tOptions);
    xllm_memory_sync_workspace_result_init(&tResult);
    xllm_error_init(&tError);

    tOptions.eScope = XLLM_MEMORY_SCOPE_MEMORY;
    tOptions.sPath = pWorkspace->sRootPath;
    tOptions.bRecursive = true;
    tOptions.bReplaceExisting = true;
    tOptions.bSkipHidden = true;
    tOptions.bSkipUnchanged = true;
    tOptions.bLoadGitIgnore = true;
    tOptions.sSourceUriPrefix = "workspace://";

    iStatus = xllm_memory_sync_workspace(pWorkspace->pMemory, &tOptions, &tResult, &tError);
    if ( iStatus != XRT_NET_OK ) {
        xllm_error_free(&tError);
        xllm_memory_sync_workspace_result_reset(&tResult);
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }

    if ( pSummary ) {
        pSummary->iVisitedFileCount = (size_t)tResult.tIngest.uVisitedFileCount;
        pSummary->iIngestedFileCount = (size_t)tResult.tIngest.uIngestedFileCount;
        pSummary->iCreatedRecordCount = (size_t)tResult.tIngest.uCreatedRecordCount;
        pSummary->iUpdatedRecordCount = (size_t)tResult.tIngest.uUpdatedRecordCount;
        pSummary->iSkippedFileCount = (size_t)tResult.tIngest.uSkippedFileCount;
        pSummary->iFailedFileCount = (size_t)tResult.tIngest.uFailedFileCount;
        pSummary->iExaminedRecordCount = (size_t)tResult.uExaminedRecordCount;
        pSummary->iRemovedRecordCount = (size_t)tResult.uRemovedRecordCount;
    }

    xllm_error_free(&tError);
    xllm_memory_sync_workspace_result_reset(&tResult);
    return XWORK_OK;
}

xwork_status xwork_workspace_sync_memory_file(
    xwork_workspace *pWorkspace,
    const char *sPath,
    xwork_workspace_memory_file_sync_summary *pSummary
)
{
    xllm_memory_sync_file_options tOptions;
    xllm_memory_change_set tChanges;
    xllm_error tError;
    size_t i;
    int iStatus;

    if ( !pWorkspace || !pWorkspace->bEnableMemory || !pWorkspace->pMemory ||
         !pWorkspace->sRootPath || !pWorkspace->sRootPath[0] ||
         !sPath || !sPath[0] ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( pSummary ) {
        xwork_workspace_memory_file_sync_summary_init(pSummary);
    }

    xllm_memory_sync_file_options_init(&tOptions);
    xllm_memory_change_set_init(&tChanges);
    xllm_error_init(&tError);

    tOptions.eScope = XLLM_MEMORY_SCOPE_MEMORY;
    tOptions.sPath = sPath;
    tOptions.sRootPath = pWorkspace->sRootPath;
    tOptions.bReplaceExisting = true;
    tOptions.bUseWorkspaceDefaults = true;
    tOptions.bSkipHidden = true;
    tOptions.bSkipUnchanged = true;
    tOptions.sSourceUriPrefix = "workspace://";

    iStatus = xllm_memory_sync_file(pWorkspace->pMemory, &tOptions, &tChanges, &tError);
    if ( iStatus != XRT_NET_OK ) {
        xllm_error_free(&tError);
        xllm_memory_change_set_reset(&tChanges);
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }

    if ( pSummary ) {
        pSummary->iChangeCount = tChanges.iChangeCount;
        for ( i = 0u; i < tChanges.iChangeCount; ++i ) {
            switch ( tChanges.pChanges[i].eKind ) {
                case XLLM_MEMORY_CHANGE_CREATED:
                    ++pSummary->iCreatedCount;
                    break;
                case XLLM_MEMORY_CHANGE_UPDATED:
                    ++pSummary->iUpdatedCount;
                    break;
                case XLLM_MEMORY_CHANGE_REMOVED:
                    ++pSummary->iRemovedCount;
                    break;
                case XLLM_MEMORY_CHANGE_SKIPPED:
                    ++pSummary->iSkippedCount;
                    break;
                case XLLM_MEMORY_CHANGE_FAILED:
                    ++pSummary->iFailedCount;
                    break;
                default:
                    break;
            }
        }
    }

    xllm_error_free(&tError);
    xllm_memory_change_set_reset(&tChanges);
    return XWORK_OK;
}
