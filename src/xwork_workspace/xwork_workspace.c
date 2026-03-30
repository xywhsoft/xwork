#include "../xwork_core/xwork_internal.h"

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
