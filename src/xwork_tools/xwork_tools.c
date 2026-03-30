#include "../xwork_core/xwork_internal.h"

xwork_status xwork_runtime_register_tool(
    xwork_runtime *pRuntime,
    const xwork_tool_def *pDef
)
{
    xwork_tool_record *pTool;

    if ( !pRuntime || !pDef || !pDef->sToolId || !pDef->sToolId[0] ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( xwork_runtime_find_tool(pRuntime, pDef->sToolId) ) {
        return XWORK_ERROR_ALREADY_EXISTS;
    }

    pTool = (xwork_tool_record *)calloc(1u, sizeof(*pTool));
    if ( !pTool ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    pTool->sToolId = xwork__dup_cstr(pDef->sToolId);
    pTool->sDisplayName = xwork__dup_cstr(pDef->sDisplayName);
    pTool->sDescription = xwork__dup_cstr(pDef->sDescription);
    if ( !pTool->sToolId ) {
        free(pTool);
        return XWORK_ERROR_NO_MEMORY;
    }

    pTool->tDef = *pDef;
    pTool->tDef.sToolId = pTool->sToolId;
    pTool->tDef.sDisplayName = pTool->sDisplayName;
    pTool->tDef.sDescription = pTool->sDescription;

    pTool->pNext = pRuntime->pTools;
    pRuntime->pTools = pTool;
    return XWORK_OK;
}

const xwork_tool_def *xwork_runtime_find_tool(
    const xwork_runtime *pRuntime,
    const char *sToolId
)
{
    xwork_tool_record *pCursor;

    if ( !pRuntime || !sToolId || !sToolId[0] ) {
        return NULL;
    }

    for ( pCursor = pRuntime->pTools; pCursor; pCursor = pCursor->pNext ) {
        if ( pCursor->sToolId && strcmp(pCursor->sToolId, sToolId) == 0 ) {
            return &pCursor->tDef;
        }
    }
    return NULL;
}
