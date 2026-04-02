#include "../xwork_core/xwork_internal.h"
#include "../../lib/xllm-session.h"
#include "../../lib/xllm-memory.h"

static void xwork__init_text_part(xllm_content_part *pPart, const char *sText)
{
    memset(pPart, 0, sizeof(*pPart));
    pPart->eKind = XLLM_PART_TEXT;
    pPart->as.tSource.eKind = XLLM_SOURCE_INLINE_TEXT;
    pPart->as.tSource.as.sText = sText ? sText : "";
}

static void xwork__init_user_message(
    xllm_message *pMessage,
    xllm_content_part *pPart,
    const char *sText
)
{
    memset(pMessage, 0, sizeof(*pMessage));
    xwork__init_text_part(pPart, sText);
    pMessage->eRole = XLLM_ROLE_USER;
    pMessage->pParts = pPart;
    pMessage->iPartCount = 1u;
}

static void xwork__init_system_message(
    xllm_message *pMessage,
    xllm_content_part *pPart,
    const char *sText
)
{
    memset(pMessage, 0, sizeof(*pMessage));
    xwork__init_text_part(pPart, sText);
    pMessage->eRole = XLLM_ROLE_SYSTEM;
    pMessage->pParts = pPart;
    pMessage->iPartCount = 1u;
}

static void xwork__init_memory_context_block(
    xllm_context_block *pBlock,
    xllm_message *pMessage,
    xllm_content_part *pPart,
    const char *sText
)
{
    memset(pBlock, 0, sizeof(*pBlock));
    xwork__init_system_message(pMessage, pPart, sText);
    pBlock->eKind = XLLM_CONTEXT_MEMORY;
    pBlock->iPriority = 0;
    pBlock->bPinned = false;
    pBlock->pMessages = pMessage;
    pBlock->iMessageCount = 1u;
}

static void xwork__init_tool_result_message(
    xllm_message *pMessage,
    xllm_content_part *pPart,
    const xwork_run *pRun
)
{
    memset(pMessage, 0, sizeof(*pMessage));
    xwork__init_text_part(pPart, pRun->sLastToolResultText);
    pMessage->eRole = XLLM_ROLE_TOOL;
    pMessage->sToolCallId = pRun->sLastToolCallId;
    pMessage->sToolName = pRun->sLastToolId;
    pMessage->pParts = pPart;
    pMessage->iPartCount = 1u;
}

static xwork_status xwork__build_request_tools(
    const xwork_runtime *pRuntime,
    xllm_tool_def **ppTools,
    size_t *piToolCount
)
{
    xwork_tool_record *pCursor;
    xllm_tool_def *pTools;
    size_t iCount = 0u;
    size_t i = 0u;

    if ( !pRuntime || !ppTools || !piToolCount ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppTools = NULL;
    *piToolCount = 0u;

    for ( pCursor = pRuntime->pTools; pCursor; pCursor = pCursor->pNext ) {
        ++iCount;
    }
    if ( iCount == 0u ) {
        return XWORK_OK;
    }

    pTools = (xllm_tool_def *)calloc(iCount, sizeof(*pTools));
    if ( !pTools ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    for ( pCursor = pRuntime->pTools; pCursor; pCursor = pCursor->pNext, ++i ) {
        pTools[i].sToolId = pCursor->tDef.sToolId;
        pTools[i].sWireName = pCursor->tDef.sToolId;
        pTools[i].sDescription = pCursor->tDef.sDescription;
        pTools[i].eKind = XLLM_TOOL_CLIENT;
    }

    *ppTools = pTools;
    *piToolCount = iCount;
    return XWORK_OK;
}

static const char *xwork__tool_call_id(const xllm_output_tool_call *pToolCall)
{
    if ( !pToolCall ) {
        return NULL;
    }
    if ( pToolCall->sToolId && pToolCall->sToolId[0] ) {
        return pToolCall->sToolId;
    }
    return pToolCall->sToolName;
}

static const char *xwork__run_session_profile_id(const xwork_run *pRun)
{
    return pRun ? pRun->sLlmProfileId : NULL;
}

void xwork__run_discard_session(xwork_run *pRun)
{
    if ( !pRun ) {
        return;
    }
    if ( pRun->pSession ) {
        xllm_session_destroy(pRun->pSession);
        pRun->pSession = NULL;
    }
}

void xwork__run_reset_session_state(xwork_run *pRun)
{
    if ( !pRun ) {
        return;
    }

    xwork__run_discard_session(pRun);
    xwork__free_cstr(&pRun->sSessionStateData);
}

static xwork_status xwork__serialize_session_state(
    xllm_session *pSession,
    char **psStateData
)
{
    xllm_session_state *pState = NULL;
    xvalue tStateValue = NULL;
    str sStateData = NULL;
    char *sStateCopy = NULL;
    xwork_status iStatus = XWORK_ERROR_EXTERNAL_FAILURE;

    if ( !pSession || !psStateData ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *psStateData = NULL;

    if ( xllm_session_export_state(pSession, &pState) != XRT_NET_OK || !pState ) {
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }
    if ( xllm_session_state_to_xvalue(pState, &tStateValue) != XRT_NET_OK || !tStateValue ) {
        goto cleanup;
    }

    sStateData = xrtStringifyXSON(tStateValue, 0, 0u, NULL);
    if ( !sStateData ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    sStateCopy = xwork__dup_cstr((const char *)sStateData);
    if ( !sStateCopy ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    *psStateData = sStateCopy;
    sStateCopy = NULL;
    iStatus = XWORK_OK;

cleanup:
    free(sStateCopy);
    if ( sStateData ) {
        xrtFree(sStateData);
    }
    if ( tStateValue ) {
        xvoUnref(tStateValue);
    }
    if ( pState ) {
        xllm_session_state_free(pState);
    }
    return iStatus;
}

xwork_status xwork__run_refresh_session_state(xwork_run *pRun)
{
    char *sStateData = NULL;
    xwork_status iStatus;

    if ( !pRun ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !pRun->pSession ) {
        return XWORK_OK;
    }

    iStatus = xwork__serialize_session_state(pRun->pSession, &sStateData);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    iStatus = xwork__replace_cstr(&pRun->sSessionStateData, sStateData);
    free(sStateData);
    return iStatus;
}

static xwork_status xwork__import_session_state(
    xwork_runtime *pRuntime,
    const char *sStateData,
    xllm_session **ppSession
)
{
    xvalue tStateValue = NULL;
    xllm_session_state *pState = NULL;
    xllm_session *pSession = NULL;
    xwork_status iStatus = XWORK_ERROR_EXTERNAL_FAILURE;

    if ( !pRuntime || !pRuntime->pLlmRuntime || !sStateData || !sStateData[0] || !ppSession ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppSession = NULL;
    tStateValue = xrtParseXSON((str)sStateData, strlen(sStateData));
    if ( !tStateValue || xvoType(tStateValue) != XVO_DT_TABLE ) {
        goto cleanup;
    }
    if ( xllm_session_state_from_xvalue(tStateValue, &pState) != XRT_NET_OK || !pState ) {
        goto cleanup;
    }
    if ( xllm_session_import_state(pRuntime->pLlmRuntime, pState, &pSession) != XRT_NET_OK ||
         !pSession ) {
        goto cleanup;
    }

    *ppSession = pSession;
    pSession = NULL;
    iStatus = XWORK_OK;

cleanup:
    if ( pSession ) {
        xllm_session_destroy(pSession);
    }
    if ( pState ) {
        xllm_session_state_free(pState);
    }
    if ( tStateValue ) {
        xvoUnref(tStateValue);
    }
    return iStatus;
}

xwork_status xwork__run_ensure_session(xwork_run *pRun)
{
    const char *sProfileId;
    xllm_session_options tOptions;
    xllm_session *pSession = NULL;

    if ( !pRun || !pRun->pRuntime || !pRun->pRuntime->pLlmRuntime ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( pRun->pSession ) {
        return XWORK_OK;
    }

    if ( pRun->sSessionStateData && pRun->sSessionStateData[0] ) {
        return xwork__import_session_state(
            pRun->pRuntime,
            pRun->sSessionStateData,
            &pRun->pSession
        );
    }

    sProfileId = xwork__run_session_profile_id(pRun);
    if ( !sProfileId || !sProfileId[0] ) {
        return XWORK_ERROR_UNSUPPORTED;
    }

    xllm_session_options_init(&tOptions);
    tOptions.sProfileId = sProfileId;
    tOptions.bEnableAutoCompact = pRun->tSessionPolicy.bEnableAutoCompact;
    tOptions.fCompactTriggerRatio = pRun->tSessionPolicy.fCompactTriggerRatio;
    if ( pRun->tSessionPolicy.iCompactTriggerTurns > 0xFFFFFFFFu ) {
        tOptions.uCompactTriggerTurns = 0xFFFFFFFFu;
    } else {
        tOptions.uCompactTriggerTurns = (uint32)pRun->tSessionPolicy.iCompactTriggerTurns;
    }
    if ( xllm_session_create(pRun->pRuntime->pLlmRuntime, &tOptions, &pSession) != XRT_NET_OK ||
         !pSession ) {
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }

    pRun->pSession = pSession;
    return XWORK_OK;
}

static bool xwork__run_has_memory_enabled_workspace(const xwork_run *pRun)
{
    size_t i;

    if ( !pRun || !pRun->pRuntime ) {
        return false;
    }

    for ( i = 0u; i < pRun->iWorkspaceCount; ++i ) {
        const char *sWorkspaceId = pRun->psWorkspaceIds ? pRun->psWorkspaceIds[i] : NULL;
        const xwork_workspace *pWorkspace;

        if ( !sWorkspaceId ) {
            continue;
        }
        pWorkspace = xwork_runtime_find_workspace(pRun->pRuntime, sWorkspaceId);
        if ( pWorkspace && xwork_workspace_is_memory_enabled(pWorkspace) ) {
            return true;
        }
    }

    return false;
}

static bool xwork__workspace_has_searchable_memory(const xwork_workspace *pWorkspace)
{
    return pWorkspace &&
           xwork_workspace_is_memory_enabled(pWorkspace) &&
           xwork_workspace_get_memory(pWorkspace) != NULL;
}

static const char *xwork__context_block_first_text(const xllm_context_block *pBlock)
{
    const xllm_message *pMessage;

    if ( !pBlock || !pBlock->pMessages || pBlock->iMessageCount == 0u ) {
        return NULL;
    }

    pMessage = &pBlock->pMessages[0];
    if ( pMessage->eRole != XLLM_ROLE_SYSTEM ) {
        return NULL;
    }
    if ( !pMessage->pParts || pMessage->iPartCount == 0u ) {
        return NULL;
    }
    if ( pMessage->pParts[0].eKind != XLLM_PART_TEXT ||
         pMessage->pParts[0].as.tSource.eKind != XLLM_SOURCE_INLINE_TEXT ) {
        return NULL;
    }
    return pMessage->pParts[0].as.tSource.as.sText;
}

static xwork_status xwork__append_memory_context_text(
    xwork_memory_context *pContext,
    const char *sText
)
{
    char *sCombined;

    if ( !pContext ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !sText || !sText[0] ) {
        return XWORK_OK;
    }
    if ( !pContext->sText || !pContext->sText[0] ) {
        return xwork__replace_cstr((char **)&pContext->sText, sText);
    }

    sCombined = xwork__dup_printf("%s\n\n%s", pContext->sText, sText);
    if ( !sCombined ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    xwork__free_cstr((char **)&pContext->sText);
    pContext->sText = sCombined;
    return XWORK_OK;
}

static xwork_status xwork__apply_workspace_memory_context(
    const xwork_run *pRun,
    xllm_request *pRequest,
    xwork_memory_context *pContext,
    xllm_error *pError
)
{
    size_t i;
    size_t iWorkspaceCount = 0u;

    if ( !pRun || !pRun->pRuntime || !pRequest || !pContext ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    xwork_memory_context_reset(pContext);

    for ( i = 0u; i < pRun->iWorkspaceCount; ++i ) {
        const char *sWorkspaceId = pRun->psWorkspaceIds ? pRun->psWorkspaceIds[i] : NULL;
        const xwork_workspace *pWorkspace;
        xllm_memory *pMemory;
        xllm_memory_search_options tSearchOptions;
        xllm_memory_search_result tSearchResult;
        xllm_memory_context_options tContextOptions;
        char sLabel[128];
        size_t iExistingBlockCount;
        const char *sBlockText;
        xwork_status iStatus;

        if ( !sWorkspaceId ) {
            continue;
        }

        pWorkspace = xwork_runtime_find_workspace(pRun->pRuntime, sWorkspaceId);
        if ( !xwork__workspace_has_searchable_memory(pWorkspace) ) {
            continue;
        }

        pMemory = xwork_workspace_get_memory(pWorkspace);
        xllm_memory_search_options_init(&tSearchOptions);
        memset(&tSearchResult, 0, sizeof(tSearchResult));
        xllm_memory_context_options_init(&tContextOptions);

        tSearchOptions.sQuery = pRun->sInstruction;
        tContextOptions.sLabel = sLabel;
        (void)snprintf(sLabel, sizeof(sLabel), "Workspace memory: %s", sWorkspaceId);

        iExistingBlockCount = pRequest->iContextBlockCount;
        if ( xllm_memory_search(pMemory, &tSearchOptions, &tSearchResult, pError) != XRT_NET_OK ) {
            xllm_memory_search_result_reset(&tSearchResult);
            return XWORK_ERROR_EXTERNAL_FAILURE;
        }
        if ( tSearchResult.iHitCount == 0u ) {
            xllm_memory_search_result_reset(&tSearchResult);
            continue;
        }

        if ( xllm_memory_apply_search_to_request(
                pRequest,
                &tSearchResult,
                &tContextOptions,
                pError
             ) != XRT_NET_OK ) {
            xllm_memory_search_result_reset(&tSearchResult);
            return XWORK_ERROR_EXTERNAL_FAILURE;
        }
        xllm_memory_search_result_reset(&tSearchResult);

        if ( pRequest->iContextBlockCount <= iExistingBlockCount ) {
            return XWORK_ERROR_EXTERNAL_FAILURE;
        }

        sBlockText = xwork__context_block_first_text(
            &pRequest->pContextBlocks[pRequest->iContextBlockCount - 1u]
        );
        iStatus = xwork__append_memory_context_text(pContext, sBlockText);
        if ( iStatus != XWORK_OK ) {
            return iStatus;
        }

        ++iWorkspaceCount;
    }

    pContext->iWorkspaceCount = iWorkspaceCount;
    return XWORK_OK;
}

static xwork_status xwork__ingest_tool_result_to_memory(
    xwork_run *pRun,
    const xwork_tool_call *pToolCall,
    const xwork_tool_result *pToolResult
)
{
    size_t i;

    if ( !pRun || !pRun->pRuntime || !pToolCall || !pToolResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    for ( i = 0u; i < pRun->iWorkspaceCount; ++i ) {
        const char *sWorkspaceId = pRun->psWorkspaceIds ? pRun->psWorkspaceIds[i] : NULL;
        const xwork_workspace *pWorkspace;
        xllm_memory *pMemory;
        xllm_memory_ingest_options tIngestOptions;
        xllm_error tError;
        char *sRecordId = NULL;
        char *sSourceUri = NULL;
        char *sTitle = NULL;
        char *sText = NULL;
        char *sSummary = NULL;
        const char *sToolId;
        xwork_status iStatus;

        if ( !sWorkspaceId ) {
            continue;
        }

        pWorkspace = xwork_runtime_find_workspace(pRun->pRuntime, sWorkspaceId);
        if ( !xwork__workspace_has_searchable_memory(pWorkspace) ) {
            continue;
        }

        sToolId = (pToolCall->sToolId && pToolCall->sToolId[0]) ? pToolCall->sToolId : "tool";
        if ( pToolResult->sVisibleSummary && pToolResult->sVisibleSummary[0] &&
             pToolResult->sOutputText && pToolResult->sOutputText[0] ) {
            sText = xwork__dup_printf(
                "Tool: %s\nSummary: %s\nOutput:\n%s",
                sToolId,
                pToolResult->sVisibleSummary,
                pToolResult->sOutputText
            );
        } else if ( pToolResult->sVisibleSummary && pToolResult->sVisibleSummary[0] ) {
            sText = xwork__dup_printf("Tool: %s\nSummary: %s", sToolId, pToolResult->sVisibleSummary);
        } else if ( pToolResult->sOutputText && pToolResult->sOutputText[0] ) {
            sText = xwork__dup_printf("Tool: %s\nOutput:\n%s", sToolId, pToolResult->sOutputText);
        }
        if ( !sText || !sText[0] ) {
            free(sText);
            continue;
        }

        sRecordId = xwork__dup_printf(
            "run:%s:workspace:%s:tool:%s",
            pRun->sRunId ? pRun->sRunId : "run",
            sWorkspaceId,
            pToolCall->sCallId ? pToolCall->sCallId : sToolId
        );
        sSourceUri = xwork__dup_printf(
            "xwork://run/%s/workspace/%s/tool/%s",
            pRun->sRunId ? pRun->sRunId : "run",
            sWorkspaceId,
            pToolCall->sCallId ? pToolCall->sCallId : sToolId
        );
        sTitle = xwork__dup_printf("Tool result: %s", sToolId);
        if ( !sRecordId || !sSourceUri || !sTitle ) {
            free(sTitle);
            free(sSourceUri);
            free(sRecordId);
            free(sText);
            return XWORK_ERROR_NO_MEMORY;
        }

        xllm_memory_ingest_options_init(&tIngestOptions);
        xllm_error_init(&tError);
        pMemory = xwork_workspace_get_memory(pWorkspace);
        tIngestOptions.eScope = XLLM_MEMORY_SCOPE_MEMORY;
        tIngestOptions.sRecordId = sRecordId;
        tIngestOptions.sTitle = sTitle;
        tIngestOptions.sSourceUri = sSourceUri;
        tIngestOptions.sText = sText;
        tIngestOptions.bReplaceExisting = true;

        if ( xllm_memory_ingest_text(pMemory, &tIngestOptions, &tError) != XRT_NET_OK ) {
            xllm_error_free(&tError);
            free(sTitle);
            free(sSourceUri);
            free(sRecordId);
            free(sText);
            return XWORK_ERROR_EXTERNAL_FAILURE;
        }
        xllm_error_free(&tError);

        sSummary = xwork__dup_printf(
            "Tool result ingested into workspace memory: %s",
            sWorkspaceId
        );
        iStatus = xwork__run_record_event(
            pRun,
            XWORK_EVENT_MEMORY_RECORD_INGESTED,
            pRun->sLastToolId,
            pRun->sLastApprovalRequestId,
            pRun->sLastCheckpointId,
            sSummary ? sSummary : "Tool result ingested into workspace memory."
        );
        free(sSummary);
        free(sTitle);
        free(sSourceUri);
        free(sRecordId);
        free(sText);
        if ( iStatus != XWORK_OK ) {
            return iStatus;
        }
    }

    return XWORK_OK;
}

static const char *xwork__artifact_kind_name(xwork_artifact_kind eKind)
{
    switch ( eKind ) {
        case XWORK_ARTIFACT_PATCH:
            return "patch";
        case XWORK_ARTIFACT_REPORT:
            return "report";
        case XWORK_ARTIFACT_COMMAND:
            return "command";
        case XWORK_ARTIFACT_OUTPUT:
        default:
            return "output";
    }
}

static xwork_status xwork__ingest_artifact_to_memory(
    xwork_run *pRun,
    const xwork_artifact *pArtifact
)
{
    size_t i;

    if ( !pRun || !pRun->pRuntime || !pArtifact ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    for ( i = 0u; i < pRun->iWorkspaceCount; ++i ) {
        const char *sWorkspaceId = pRun->psWorkspaceIds ? pRun->psWorkspaceIds[i] : NULL;
        const xwork_workspace *pWorkspace;
        xllm_memory *pMemory;
        xllm_memory_ingest_options tIngestOptions;
        xllm_error tError;
        char *sRecordId = NULL;
        char *sSourceUri = NULL;
        char *sTitle = NULL;
        char *sText = NULL;
        char *sSummary = NULL;
        xwork_status iStatus;
        const char *sArtifactId;
        const char *sArtifactName;

        if ( !sWorkspaceId ) {
            continue;
        }

        pWorkspace = xwork_runtime_find_workspace(pRun->pRuntime, sWorkspaceId);
        if ( !xwork__workspace_has_searchable_memory(pWorkspace) ) {
            continue;
        }

        sArtifactId = (pArtifact->sArtifactId && pArtifact->sArtifactId[0])
            ? pArtifact->sArtifactId
            : "artifact";
        sArtifactName = (pArtifact->sName && pArtifact->sName[0])
            ? pArtifact->sName
            : sArtifactId;

        sText = xwork__dup_printf(
            "Artifact kind: %s\nName: %s\nMime type: %s\nStorage ref: %s\nSummary: %s",
            xwork__artifact_kind_name(pArtifact->eKind),
            sArtifactName,
            pArtifact->sMimeType ? pArtifact->sMimeType : "",
            pArtifact->sStorageRef ? pArtifact->sStorageRef : "",
            pArtifact->sSummary ? pArtifact->sSummary : ""
        );
        if ( !sText ) {
            return XWORK_ERROR_NO_MEMORY;
        }

        sRecordId = xwork__dup_printf(
            "run:%s:workspace:%s:artifact:%s",
            pRun->sRunId ? pRun->sRunId : "run",
            sWorkspaceId,
            sArtifactId
        );
        sSourceUri = xwork__dup_printf(
            "xwork://run/%s/workspace/%s/artifact/%s",
            pRun->sRunId ? pRun->sRunId : "run",
            sWorkspaceId,
            sArtifactId
        );
        sTitle = xwork__dup_printf("Artifact: %s", sArtifactName);
        if ( !sRecordId || !sSourceUri || !sTitle ) {
            free(sTitle);
            free(sSourceUri);
            free(sRecordId);
            free(sText);
            return XWORK_ERROR_NO_MEMORY;
        }

        xllm_memory_ingest_options_init(&tIngestOptions);
        xllm_error_init(&tError);
        pMemory = xwork_workspace_get_memory(pWorkspace);
        tIngestOptions.eScope = XLLM_MEMORY_SCOPE_MEMORY;
        tIngestOptions.sRecordId = sRecordId;
        tIngestOptions.sTitle = sTitle;
        tIngestOptions.sSourceUri = sSourceUri;
        tIngestOptions.sText = sText;
        tIngestOptions.bReplaceExisting = true;

        if ( xllm_memory_ingest_text(pMemory, &tIngestOptions, &tError) != XRT_NET_OK ) {
            xllm_error_free(&tError);
            free(sTitle);
            free(sSourceUri);
            free(sRecordId);
            free(sText);
            return XWORK_ERROR_EXTERNAL_FAILURE;
        }
        xllm_error_free(&tError);

        sSummary = xwork__dup_printf(
            "Artifact ingested into workspace memory: %s",
            sWorkspaceId
        );
        iStatus = xwork__run_record_event(
            pRun,
            XWORK_EVENT_MEMORY_RECORD_INGESTED,
            pRun->sLastToolId,
            pRun->sLastApprovalRequestId,
            pRun->sLastCheckpointId,
            sSummary ? sSummary : "Artifact ingested into workspace memory."
        );
        free(sSummary);
        free(sTitle);
        free(sSourceUri);
        free(sRecordId);
        free(sText);
        if ( iStatus != XWORK_OK ) {
            return iStatus;
        }
    }

    return XWORK_OK;
}

static xwork_status xwork__ingest_artifacts_to_memory_range(
    xwork_run *pRun,
    size_t iStartIndex
)
{
    size_t i;

    if ( !pRun ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    for ( i = iStartIndex; i < pRun->iArtifactCount; ++i ) {
        xwork_status iStatus = xwork__ingest_artifact_to_memory(
            pRun,
            &pRun->pArtifactLog[i].tArtifact
        );
        if ( iStatus != XWORK_OK ) {
            return iStatus;
        }
    }

    return XWORK_OK;
}

static xwork_status xwork__evaluate_tool_approval(
    const xwork_run *pRun,
    const xwork_tool_def *pTool,
    const xwork_orchestrator_options *pOptions,
    xwork_approval_decision *pDecision
)
{
    xwork_approval_eval_input tInput;

    if ( !pRun || !pRun->pRuntime || !pTool || !pOptions || !pDecision ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    xwork_approval_eval_input_init(&tInput);
    tInput.eAutonomy = pRun->eAutonomy;
    tInput.eApprovalMode = pTool->eApprovalMode;
    tInput.eSideEffect = pTool->eSideEffect;
    tInput.bAutoApproveRequested = pOptions->bAutoApprove;
    return xwork_policy_evaluate_approval(&pRun->pRuntime->tPolicy, &tInput, pDecision);
}

static xwork_status xwork__record_run_state_event(
    xwork_run *pRun,
    xwork_run_state eState,
    xwork_event_kind eKind,
    const char *sSummary
)
{
    if ( !pRun ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    pRun->eState = eState;
    return xwork__run_record_event(pRun, eKind, NULL, NULL, NULL, sSummary);
}

static xwork_status xwork__store_tool_result(
    xwork_run *pRun,
    const xwork_tool_result *pResult
);

static xwork_status xwork__fail_run_with_summary(xwork_run *pRun, const char *sSummary)
{
    xwork_status iStatus;
    const char *sEffectiveSummary = sSummary ? sSummary : "Run failed.";

    iStatus = xwork__replace_cstr(&pRun->sLastOutputText, sEffectiveSummary);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    return xwork__record_run_state_event(
        pRun,
        XWORK_RUN_FAILED,
        XWORK_EVENT_RUN_FAILED,
        sEffectiveSummary
    );
}

static void xwork__fail_run_after_tool_error(
    xwork_run *pRun,
    const xwork_tool_result *pToolResult,
    const char *sFallbackSummary
)
{
    const char *sSummary = sFallbackSummary ? sFallbackSummary : "Tool execution failed.";

    if ( !pRun ) {
        return;
    }

    if ( pToolResult &&
         ((pToolResult->sOutputText && pToolResult->sOutputText[0]) ||
          (pToolResult->sVisibleSummary && pToolResult->sVisibleSummary[0])) ) {
        (void)xwork__store_tool_result(pRun, pToolResult);
    }
    if ( pToolResult &&
         pToolResult->sVisibleSummary &&
         pToolResult->sVisibleSummary[0] ) {
        sSummary = pToolResult->sVisibleSummary;
    }

    (void)xwork__fail_run_with_summary(pRun, sSummary);
}

static xwork_status xwork__save_checkpoint(
    xwork_run *pRun,
    xwork_checkpoint_kind eKind,
    const char *sPendingStep,
    const char *sToolOutputsRef,
    const char *sSummary
)
{
    char *sArtifactRefs;
    char *sSessionStateRef = NULL;
    char *sCheckpointId = NULL;
    xwork_status iStatus;

    iStatus = xwork__run_refresh_session_state(pRun);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    if ( pRun->sSessionStateData && pRun->sSessionStateData[0] ) {
        sCheckpointId = xwork__dup_scoped_id(
            pRun->sRunId,
            "checkpoint",
            pRun->iNextCheckpointSequence + 1u
        );
        if ( !sCheckpointId ) {
            return XWORK_ERROR_NO_MEMORY;
        }

        sSessionStateRef = xwork__dup_printf(
            "xwork://run/%s/checkpoint/%s/session",
            pRun->sRunId,
            sCheckpointId
        );
        free(sCheckpointId);
        if ( !sSessionStateRef ) {
            return XWORK_ERROR_NO_MEMORY;
        }
    }

    sArtifactRefs = xwork__run_build_artifact_refs(pRun);
    iStatus = xwork__run_record_checkpoint(
        pRun,
        eKind,
        sPendingStep,
        sSessionStateRef,
        sToolOutputsRef,
        NULL,
        sArtifactRefs
    );
    free(sSessionStateRef);
    free(sArtifactRefs);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    return xwork__run_record_event(
        pRun,
        XWORK_EVENT_CHECKPOINT_SAVED,
        pRun->sLastToolId,
        pRun->sLastApprovalRequestId,
        pRun->sLastCheckpointId,
        sSummary
    );
}

static xwork_status xwork__store_tool_call(
    xwork_run *pRun,
    const xllm_output_tool_call *pToolCall
)
{
    const char *sToolId = xwork__tool_call_id(pToolCall);
    xwork_status iStatus;

    iStatus = xwork__replace_cstr(&pRun->sLastToolCallId, pToolCall ? pToolCall->sCallId : NULL);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    iStatus = xwork__replace_cstr(&pRun->sLastToolId, sToolId);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    iStatus = xwork__replace_cstr(
        &pRun->sLastToolArgumentsJson,
        pToolCall ? pToolCall->sArgumentsJson : NULL
    );
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    pRun->bHasLastToolCall = sToolId != NULL && sToolId[0] != '\0';
    pRun->bHasLastToolResult = false;
    iStatus = xwork__replace_cstr(&pRun->sLastToolResultText, NULL);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    return xwork__replace_cstr(&pRun->sLastToolVisibleSummary, NULL);
}

static xwork_status xwork__store_tool_result(
    xwork_run *pRun,
    const xwork_tool_result *pResult
)
{
    xwork_status iStatus;

    iStatus = xwork__replace_cstr(&pRun->sLastToolResultText, pResult ? pResult->sOutputText : NULL);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    iStatus = xwork__replace_cstr(
        &pRun->sLastToolVisibleSummary,
        pResult ? pResult->sVisibleSummary : NULL
    );
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    pRun->bHasLastToolResult = (pResult != NULL);
    return XWORK_OK;
}

static xwork_status xwork__store_final_output(xwork_run *pRun, const char *sOutputText)
{
    return xwork__replace_cstr(&pRun->sLastOutputText, sOutputText);
}

static bool xwork__orchestrator_has_pending_tool_call(const xwork_run *pRun)
{
    return pRun &&
           pRun->bHasLastToolCall &&
           !pRun->bHasLastToolResult &&
           pRun->sLastToolId &&
           pRun->sLastToolId[0];
}

static bool xwork__parse_json_table(const char *sJson, xvalue *ptTable)
{
    xvalue tValue;

    if ( !ptTable ) {
        return false;
    }

    *ptTable = NULL;
    if ( !sJson || !sJson[0] ) {
        return true;
    }

    tValue = xrtParseJSON((str)sJson, strlen(sJson));
    if ( !tValue || xvoType(tValue) != XVO_DT_TABLE ) {
        if ( tValue ) {
            xvoUnref(tValue);
        }
        return false;
    }

    *ptTable = tValue;
    return true;
}

static const char *xwork__json_table_get_text(xvalue tTable, const char *sKey)
{
    xvalue tValue;

    if ( !tTable || !sKey ) {
        return NULL;
    }

    tValue = xvoTableGetValue(tTable, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) != XVO_DT_TEXT ) {
        return NULL;
    }

    return (const char *)xvoGetText(tValue);
}

static bool xwork__json_table_get_int(xvalue tTable, const char *sKey, int *piValue)
{
    xvalue tValue;

    if ( piValue ) {
        *piValue = 0;
    }
    if ( !tTable || !sKey || !piValue ) {
        return false;
    }

    tValue = xvoTableGetValue(tTable, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) != XVO_DT_INT ) {
        return false;
    }

    *piValue = (int)xvoGetInt(tValue);
    return true;
}

static xvalue xwork__json_table_get_array(xvalue tTable, const char *sKey)
{
    xvalue tValue;

    if ( !tTable || !sKey ) {
        return NULL;
    }

    tValue = xvoTableGetValue(tTable, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) != XVO_DT_ARRAY ) {
        return NULL;
    }

    return tValue;
}

static xwork_status xwork__append_text(char **psTarget, size_t *piLength, const char *sText)
{
    char *sNext;
    size_t iAppendLength;

    if ( !psTarget || !piLength ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !sText || !sText[0] ) {
        return XWORK_OK;
    }

    iAppendLength = strlen(sText);
    sNext = (char *)realloc(*psTarget, *piLength + iAppendLength + 1u);
    if ( !sNext ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    memcpy(sNext + *piLength, sText, iAppendLength);
    *piLength += iAppendLength;
    sNext[*piLength] = '\0';
    *psTarget = sNext;
    return XWORK_OK;
}

static xwork_status xwork__build_terminal_event_output(
    xvalue tResult,
    char **psOutputText
)
{
    xvalue tEvents;
    char *sOutputText = NULL;
    size_t iOutputLength = 0u;
    uint32 i;
    xwork_status iStatus = XWORK_OK;

    if ( !psOutputText ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *psOutputText = NULL;
    if ( !tResult ) {
        return XWORK_OK;
    }

    tEvents = xwork__json_table_get_array(tResult, "events");
    if ( !tEvents ) {
        return XWORK_OK;
    }

    for ( i = 0u; i < xvoArrayItemCount(tEvents); ++i ) {
        xvalue tEvent = xvoArrayGetValue(tEvents, i);
        const char *sKind;
        const char *sText;

        if ( !tEvent || xvoType(tEvent) != XVO_DT_TABLE ) {
            continue;
        }

        sKind = xwork__json_table_get_text(tEvent, "kind");
        if ( !sKind || strcmp(sKind, "output") != 0 ) {
            continue;
        }

        sText = xwork__json_table_get_text(tEvent, "text");
        iStatus = xwork__append_text(&sOutputText, &iOutputLength, sText);
        if ( iStatus != XWORK_OK ) {
            goto cleanup;
        }
    }

    *psOutputText = sOutputText;
    sOutputText = NULL;

cleanup:
    free(sOutputText);
    return iStatus;
}

static const char *xwork__path_leaf_name(const char *sPath)
{
    const char *sLeaf;

    if ( !sPath || !sPath[0] ) {
        return NULL;
    }

    sLeaf = strrchr(sPath, '/');
    if ( !sLeaf ) {
        sLeaf = strrchr(sPath, '\\');
    }
    return (sLeaf && sLeaf[1]) ? (sLeaf + 1) : sPath;
}

static xwork_status xwork__emit_builtin_tool_artifact(
    xwork_run *pRun,
    const xwork_tool_def *pToolDef,
    const xwork_tool_call *pToolCall,
    const xwork_tool_result *pToolResult
)
{
    xvalue tArguments = NULL;
    xvalue tResult = NULL;
    char *sCombinedOutput = NULL;
    char *sTerminalOutput = NULL;
    xwork_status iStatus = XWORK_OK;

    if ( !pRun || !pToolDef || !pToolCall || !pToolResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( !xwork__parse_json_table(pToolCall->sArgumentsJson, &tArguments) ||
         !xwork__parse_json_table(pToolResult->sOutputText, &tResult) ) {
        return XWORK_OK;
    }

    if ( strcmp(pToolDef->sToolId, XWORK_TOOL_FILESYSTEM_READ_TEXT) == 0 ) {
        xwork_output_artifact_options tOptions;
        const char *sText = xwork__json_table_get_text(tResult, "text");
        const char *sPath = xwork__json_table_get_text(tResult, "resolved_path");

        if ( !sPath ) {
            sPath = xwork__json_table_get_text(tArguments, "path");
        }
        if ( sText ) {
            xwork_output_artifact_options_init(&tOptions);
            tOptions.sName = xwork__path_leaf_name(sPath);
            tOptions.sStorageRef = sPath;
            tOptions.sSummary = pToolResult->sVisibleSummary;
            tOptions.sOutputText = sText;
            iStatus = xwork_run_emit_output_artifact(pRun, &tOptions, NULL);
        }
    } else if ( strcmp(pToolDef->sToolId, XWORK_TOOL_FILESYSTEM_WRITE_TEXT) == 0 ) {
        xwork_output_artifact_options tOptions;
        const char *sText = xwork__json_table_get_text(tArguments, "text");
        const char *sPath = xwork__json_table_get_text(tResult, "resolved_path");

        if ( !sPath ) {
            sPath = xwork__json_table_get_text(tArguments, "path");
        }
        if ( sText ) {
            xwork_output_artifact_options_init(&tOptions);
            tOptions.sName = xwork__path_leaf_name(sPath);
            tOptions.sStorageRef = sPath;
            tOptions.sSummary = pToolResult->sVisibleSummary;
            tOptions.sOutputText = sText;
            iStatus = xwork_run_emit_output_artifact(pRun, &tOptions, NULL);
        }
    } else if ( strcmp(pToolDef->sToolId, XWORK_TOOL_PROCESS_EXEC) == 0 ) {
        xwork_command_artifact_options tOptions;
        const char *sCommand = xwork__json_table_get_text(tArguments, "command");
        const char *sStdout = xwork__json_table_get_text(tResult, "stdout");
        const char *sStderr = xwork__json_table_get_text(tResult, "stderr");
        const char *sOutput = sStdout ? sStdout : "";
        const char *sCwd = xwork__json_table_get_text(tResult, "cwd");
        int iExitCode = 0;
        bool bHasExitCode = xwork__json_table_get_int(tResult, "exit_code", &iExitCode);

        if ( sStderr && sStderr[0] ) {
            if ( sStdout && sStdout[0] ) {
                sCombinedOutput = xwork__dup_printf(
                    "STDOUT:\n%s\n\nSTDERR:\n%s",
                    sStdout,
                    sStderr
                );
            } else {
                sCombinedOutput = xwork__dup_printf("STDERR:\n%s", sStderr);
            }
            if ( !sCombinedOutput ) {
                iStatus = XWORK_ERROR_NO_MEMORY;
                goto cleanup;
            }
            sOutput = sCombinedOutput;
        }
        if ( sCommand ) {
            xwork_command_artifact_options_init(&tOptions);
            tOptions.sName = "process.exec.txt";
            tOptions.sStorageRef = sCwd;
            tOptions.sSummary = pToolResult->sVisibleSummary;
            tOptions.sCommandText = sCommand;
            tOptions.sOutputText = sOutput;
            tOptions.bHasExitCode = bHasExitCode;
            tOptions.iExitCode = iExitCode;
            iStatus = xwork_run_emit_command_artifact(pRun, &tOptions, NULL);
        }
    } else if ( strcmp(pToolDef->sToolId, XWORK_TOOL_PROCESS_START_TERMINAL) == 0 ) {
        xwork_command_artifact_options tOptions;
        const char *sCommand = xwork__json_table_get_text(tResult, "command");
        const char *sSessionId = xwork__json_table_get_text(tResult, "session_id");

        if ( !sCommand ) {
            sCommand = xwork__json_table_get_text(tArguments, "command");
        }
        if ( sCommand ) {
            xwork_command_artifact_options_init(&tOptions);
            tOptions.sName = "process.start_terminal.txt";
            tOptions.sStorageRef = sSessionId;
            tOptions.sSummary = pToolResult->sVisibleSummary;
            tOptions.sCommandText = sCommand;
            tOptions.sOutputText = "";
            iStatus = xwork_run_emit_command_artifact(pRun, &tOptions, NULL);
        }
    } else if ( strcmp(pToolDef->sToolId, XWORK_TOOL_PROCESS_TERMINAL_WRITE) == 0 ) {
        xwork_command_artifact_options tOptions;
        const char *sInputText = xwork__json_table_get_text(tArguments, "input_text");
        const char *sSessionId = xwork__json_table_get_text(tResult, "session_id");

        if ( !sSessionId ) {
            sSessionId = xwork__json_table_get_text(tArguments, "session_id");
        }
        if ( sInputText ) {
            xwork_command_artifact_options_init(&tOptions);
            tOptions.sName = "process.terminal_write.txt";
            tOptions.sStorageRef = sSessionId;
            tOptions.sSummary = pToolResult->sVisibleSummary;
            tOptions.sCommandText = sInputText;
            tOptions.sOutputText = "";
            iStatus = xwork_run_emit_command_artifact(pRun, &tOptions, NULL);
        }
    } else if ( strcmp(pToolDef->sToolId, XWORK_TOOL_PROCESS_TERMINAL_STOP) == 0 ) {
        xwork_output_artifact_options tOptions;
        const char *sSessionId = xwork__json_table_get_text(tResult, "session_id");
        const char *sOutputText = xwork__json_table_get_text(tResult, "output_text");

        if ( !sOutputText ) {
            iStatus = xwork__build_terminal_event_output(tResult, &sTerminalOutput);
            if ( iStatus != XWORK_OK ) {
                goto cleanup;
            }
            sOutputText = sTerminalOutput;
        }
        if ( !sSessionId ) {
            sSessionId = xwork__json_table_get_text(tArguments, "session_id");
        }
        if ( sOutputText && sOutputText[0] ) {
            xwork_output_artifact_options_init(&tOptions);
            tOptions.sName = "process.terminal_session.txt";
            tOptions.sStorageRef = sSessionId;
            tOptions.sSummary = pToolResult->sVisibleSummary;
            tOptions.sOutputText = sOutputText;
            iStatus = xwork_run_emit_output_artifact(pRun, &tOptions, NULL);
        }
    } else if ( strcmp(pToolDef->sToolId, XWORK_TOOL_VCS_STATUS) == 0 ) {
        xwork_command_artifact_options tOptions;
        const char *sStatusText = xwork__json_table_get_text(tResult, "status");
        const char *sPath = xwork__json_table_get_text(tResult, "resolved_path");

        if ( !sPath ) {
            sPath = xwork__json_table_get_text(tArguments, "path");
        }
        if ( sStatusText ) {
            xwork_command_artifact_options_init(&tOptions);
            tOptions.sName = "git-status.txt";
            tOptions.sStorageRef = sPath;
            tOptions.sSummary = pToolResult->sVisibleSummary;
            tOptions.sCommandText = "git status --short --branch";
            tOptions.sOutputText = sStatusText;
            iStatus = xwork_run_emit_command_artifact(pRun, &tOptions, NULL);
        }
    }

cleanup:
    free(sCombinedOutput);
    free(sTerminalOutput);
    if ( tArguments ) {
        xvoUnref(tArguments);
    }
    if ( tResult ) {
        xvoUnref(tResult);
    }
    return iStatus;
}

static xwork_status xwork__execute_tool(
    xwork_run *pRun,
    const xwork_tool_def *pToolDef,
    const xwork_tool_call *pToolCall,
    xwork_tool_result *pToolResult,
    const xwork_orchestrator_options *pOptions
)
{
    if ( !pRun || !pToolDef || !pToolCall || !pToolResult || !pOptions ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( pOptions->pfnToolExec ) {
        return pOptions->pfnToolExec(pRun, pToolCall, pToolResult, pOptions->pUserData);
    }

    if ( pToolDef->eKind == XWORK_TOOL_HOST_SERVICE &&
         pToolDef->eHostService != XWORK_HOST_NONE &&
         pToolDef->sOperationId &&
         pToolDef->sOperationId[0] ) {
        return xwork_runtime_invoke_host_service(
            pRun->pRuntime,
            pToolDef->eHostService,
            pToolDef->sOperationId,
            pToolCall->sArgumentsJson,
            pToolResult
        );
    }

    return XWORK_ERROR_UNSUPPORTED;
}

static xwork_status xwork__resume_pending_tool(
    xwork_run *pRun,
    const xwork_orchestrator_options *pOptions
)
{
    const xwork_tool_def *pToolDef;
    xwork_tool_call tToolCall;
    xwork_tool_result tToolResult;
    xwork_status iStatus;
    bool bPendingToolApprovalLinked = false;
    size_t iArtifactCountBefore = 0u;

    if ( !pRun || !pOptions ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !xwork__orchestrator_has_pending_tool_call(pRun) ) {
        return XWORK_OK;
    }

    bPendingToolApprovalLinked =
        pRun->bHasLastApprovalRequest &&
        pRun->sLastApprovalRequestId &&
        pRun->sLastApprovalToolId &&
        strcmp(pRun->sLastApprovalToolId, pRun->sLastToolId) == 0;

    if ( bPendingToolApprovalLinked ) {
        if ( pRun->eLastApprovalState == XWORK_APPROVAL_PENDING ) {
            return XWORK_ERROR_INVALID_STATE;
        }
        if ( pRun->eLastApprovalState != XWORK_APPROVAL_APPROVED ) {
            return XWORK_ERROR_INVALID_STATE;
        }
    }

    pToolDef = xwork_runtime_find_tool(pRun->pRuntime, pRun->sLastToolId);
    if ( !pToolDef ) {
        return xwork__fail_run_with_summary(
            pRun,
            "Resumed run references an unknown tool."
        );
    }

    memset(&tToolCall, 0, sizeof(tToolCall));
    xwork_tool_result_init(&tToolResult);
    tToolCall.sCallId = pRun->sLastToolCallId;
    tToolCall.sToolId = pRun->sLastToolId;
    tToolCall.sArgumentsJson = pRun->sLastToolArgumentsJson;

    pRun->eState = XWORK_RUN_WAITING_TOOL;
    iStatus = xwork__run_record_event(
        pRun,
        XWORK_EVENT_TOOL_EXEC_STARTED,
        pRun->sLastToolId,
        pRun->sLastApprovalRequestId,
        pRun->sLastCheckpointId,
        "Resuming pending tool execution."
    );
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    pRun->eState = XWORK_RUN_RUNNING;
    iArtifactCountBefore = pRun->iArtifactCount;
    iStatus = xwork__execute_tool(
        pRun,
        pToolDef,
        &tToolCall,
        &tToolResult,
        pOptions
    );
    if ( iStatus != XWORK_OK ) {
        xwork__fail_run_after_tool_error(
            pRun,
            &tToolResult,
            "Tool execution failed after resume."
        );
        return iStatus;
    }

    iStatus = xwork__store_tool_result(pRun, &tToolResult);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    iStatus = xwork__emit_builtin_tool_artifact(
        pRun,
        pToolDef,
        &tToolCall,
        &tToolResult
    );
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    if ( pOptions->bIngestToolResultsToMemory ) {
        iStatus = xwork__ingest_tool_result_to_memory(pRun, &tToolCall, &tToolResult);
        if ( iStatus != XWORK_OK ) {
            return iStatus;
        }
    }

    if ( pOptions->bIngestArtifactsToMemory && pRun->iArtifactCount > iArtifactCountBefore ) {
        iStatus = xwork__ingest_artifacts_to_memory_range(pRun, iArtifactCountBefore);
        if ( iStatus != XWORK_OK ) {
            return iStatus;
        }
    }

    iStatus = xwork__run_record_event(
        pRun,
        XWORK_EVENT_TOOL_EXEC_COMPLETED,
        pRun->sLastToolId,
        pRun->sLastApprovalRequestId,
        pRun->sLastCheckpointId,
        tToolResult.sVisibleSummary ? tToolResult.sVisibleSummary : "Tool execution completed."
    );
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    return xwork__save_checkpoint(
        pRun,
        XWORK_CHECKPOINT_AFTER_TOOL,
        "model_turn",
        pRun->sLastToolResultText,
        "Checkpoint saved after resumed tool execution."
    );
}

xwork_status xwork_run_execute(
    xwork_run *pRun,
    const xwork_orchestrator_options *pOptions
)
{
    xwork_orchestrator_options tDefaultOptions;
    const xwork_orchestrator_options *pExecOptions = pOptions;
    size_t iTurn;

    if ( !pRun || !pRun->pRuntime ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( !pExecOptions ) {
        xwork_orchestrator_options_init(&tDefaultOptions);
        pExecOptions = &tDefaultOptions;
    }

    if ( pExecOptions->iMaxTurns == 0u ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !pRun->pRuntime->pLlmRuntime ||
         !xwork__run_session_profile_id(pRun) ||
         !xwork__run_session_profile_id(pRun)[0] ) {
        return XWORK_ERROR_UNSUPPORTED;
    }
    if ( pRun->eState == XWORK_RUN_CREATED ||
         pRun->eState == XWORK_RUN_READY ||
         pRun->eState == XWORK_RUN_PAUSED ) {
        xwork_status iStartStatus = xwork_run_start(pRun);
        if ( iStartStatus != XWORK_OK ) {
            return iStartStatus;
        }
    } else if ( pRun->eState != XWORK_RUN_RUNNING ) {
        return XWORK_ERROR_INVALID_STATE;
    }

    if ( xwork__orchestrator_has_pending_tool_call(pRun) ) {
        xwork_status iResumeStatus = xwork__resume_pending_tool(pRun, pExecOptions);
        if ( iResumeStatus != XWORK_OK ) {
            return iResumeStatus;
        }
    }

    for ( iTurn = 0u; iTurn < pExecOptions->iMaxTurns; ++iTurn ) {
        xllm_request tMemoryRequest;
        xllm_turn tTurn;
        xllm_error tError;
        xllm_response *pResponse = NULL;
        xllm_message atMessages[2];
        xllm_content_part atParts[2];
        xllm_context_block atContextBlocks[1];
        xllm_message atContextMessages[1];
        xllm_content_part atContextParts[1];
        xwork_memory_context tMemoryContext;
        xllm_tool_def *pTools = NULL;
        size_t iToolCount = 0u;
        size_t iMessageCount = 0u;
        size_t iPartCount = 0u;
        size_t iContextBlockCount = 0u;
        xwork_status iStatus;
        bool bCanAttachMemoryContext;
        bool bUseDefaultWorkspaceMemory;
        bool bHasToolFollowup;
        const xllm_output_tool_call *pToolCall = NULL;
        const char *sVisibleText = NULL;

        xllm_request_init(&tMemoryRequest);
        xllm_error_init(&tError);
        xwork_memory_context_init(&tMemoryContext);
        memset(&tTurn, 0, sizeof(tTurn));
        memset(atMessages, 0, sizeof(atMessages));
        memset(atParts, 0, sizeof(atParts));
        memset(atContextBlocks, 0, sizeof(atContextBlocks));
        memset(atContextMessages, 0, sizeof(atContextMessages));
        memset(atContextParts, 0, sizeof(atContextParts));

        iStatus = xwork__run_ensure_session(pRun);
        if ( iStatus != XWORK_OK ) {
            xllm_request_reset(&tMemoryRequest);
            xllm_error_free(&tError);
            xwork_memory_context_reset(&tMemoryContext);
            return iStatus;
        }

        iStatus = xwork__build_request_tools(pRun->pRuntime, &pTools, &iToolCount);
        if ( iStatus != XWORK_OK ) {
            xllm_request_reset(&tMemoryRequest);
            xllm_error_free(&tError);
            xwork_memory_context_reset(&tMemoryContext);
            return iStatus;
        }

        bCanAttachMemoryContext = pExecOptions->pfnResolveMemoryContext &&
                                  xwork__run_has_memory_enabled_workspace(pRun);
        bUseDefaultWorkspaceMemory = !pExecOptions->pfnResolveMemoryContext;
        bHasToolFollowup = pRun->bHasLastToolCall && pRun->bHasLastToolResult;
        iStatus = xwork__run_set_last_memory_context(pRun, NULL);
        if ( iStatus != XWORK_OK ) {
            free(pTools);
            xllm_request_reset(&tMemoryRequest);
            xllm_error_free(&tError);
            xwork_memory_context_reset(&tMemoryContext);
            return iStatus;
        }

        if ( bCanAttachMemoryContext ) {
            iStatus = pExecOptions->pfnResolveMemoryContext(
                pRun,
                &tMemoryContext,
                pExecOptions->pMemoryUserData
            );
            if ( iStatus != XWORK_OK ) {
                free(pTools);
                xllm_request_reset(&tMemoryRequest);
                xllm_error_free(&tError);
                xwork_memory_context_reset(&tMemoryContext);
                return xwork__fail_run_with_summary(
                    pRun,
                    "Failed to resolve memory context."
                );
            }
            if ( tMemoryContext.sText && tMemoryContext.sText[0] ) {
                iStatus = xwork__run_set_last_memory_context(pRun, &tMemoryContext);
                if ( iStatus != XWORK_OK ) {
                    free(pTools);
                    xllm_request_reset(&tMemoryRequest);
                    xllm_error_free(&tError);
                    xwork_memory_context_reset(&tMemoryContext);
                    return iStatus;
                }
                iStatus = xwork__run_record_event(
                    pRun,
                    XWORK_EVENT_MEMORY_CONTEXT_ATTACHED,
                    pRun->sLastToolId,
                    pRun->sLastApprovalRequestId,
                    pRun->sLastCheckpointId,
                    "Memory context attached."
                );
                if ( iStatus != XWORK_OK ) {
                    free(pTools);
                    xllm_request_reset(&tMemoryRequest);
                    xllm_error_free(&tError);
                    xwork_memory_context_reset(&tMemoryContext);
                    return iStatus;
                }
            }
        }

        if ( bUseDefaultWorkspaceMemory ) {
            iStatus = xwork__apply_workspace_memory_context(
                pRun,
                &tMemoryRequest,
                &tMemoryContext,
                &tError
            );
            if ( iStatus != XWORK_OK ) {
                char *sErrorSummary = xwork__dup_printf(
                    "Workspace memory search failed: %s",
                    tError.sMessage ? tError.sMessage : "unknown error"
                );
                free(pTools);
                xllm_request_reset(&tMemoryRequest);
                xllm_error_free(&tError);
                xwork_memory_context_reset(&tMemoryContext);
                iStatus = xwork__fail_run_with_summary(
                    pRun,
                    sErrorSummary ? sErrorSummary : "Workspace memory search failed."
                );
                free(sErrorSummary);
                return iStatus;
            }
            if ( tMemoryContext.sText && tMemoryContext.sText[0] ) {
                iStatus = xwork__run_set_last_memory_context(pRun, &tMemoryContext);
                if ( iStatus != XWORK_OK ) {
                    free(pTools);
                    xllm_request_reset(&tMemoryRequest);
                    xllm_error_free(&tError);
                    xwork_memory_context_reset(&tMemoryContext);
                    return iStatus;
                }
                iStatus = xwork__run_record_event(
                    pRun,
                    XWORK_EVENT_MEMORY_CONTEXT_ATTACHED,
                    pRun->sLastToolId,
                    pRun->sLastApprovalRequestId,
                    pRun->sLastCheckpointId,
                    "Memory context attached."
                );
                if ( iStatus != XWORK_OK ) {
                    free(pTools);
                    xllm_request_reset(&tMemoryRequest);
                    xllm_error_free(&tError);
                    xwork_memory_context_reset(&tMemoryContext);
                    return iStatus;
                }
            }
        }

        if ( tMemoryContext.sText && tMemoryContext.sText[0] ) {
            xwork__init_memory_context_block(
                &atContextBlocks[0],
                &atContextMessages[0],
                &atContextParts[0],
                tMemoryContext.sText
            );
            iContextBlockCount = 1u;
        }

        if ( bHasToolFollowup ) {
            xwork__init_tool_result_message(
                &atMessages[iMessageCount],
                &atParts[iPartCount],
                pRun
            );
            ++iMessageCount;
            ++iPartCount;
        } else {
            xwork__init_user_message(
                &atMessages[iMessageCount],
                &atParts[iPartCount],
                pRun->sInstruction
            );
            ++iMessageCount;
            ++iPartCount;
        }

        tTurn.pMessages = atMessages;
        tTurn.iMessageCount = iMessageCount;
        tTurn.pContextBlocks = iContextBlockCount ? atContextBlocks : NULL;
        tTurn.iContextBlockCount = iContextBlockCount;
        tTurn.pTools = pTools;
        tTurn.iToolCount = iToolCount;
        tTurn.tToolPolicy.eMode = XLLM_TOOL_CHOICE_AUTO;

        iStatus = xwork__run_record_event(
            pRun,
            XWORK_EVENT_MODEL_TURN_STARTED,
            pRun->sLastToolId,
            pRun->sLastApprovalRequestId,
            pRun->sLastCheckpointId,
            "Starting model turn."
        );
        if ( iStatus != XWORK_OK ) {
            free(pTools);
            xllm_request_reset(&tMemoryRequest);
            xllm_error_free(&tError);
            xwork_memory_context_reset(&tMemoryContext);
            return iStatus;
        }

        if ( xllm_session_chat_ex(pRun->pSession, &tTurn, NULL, &pResponse, &tError) != XRT_NET_OK ||
             !pResponse ) {
            char *sErrorSummary = xwork__dup_printf(
                "xllm session chat failed: %s",
                tError.sMessage ? tError.sMessage : "unknown error"
            );

            free(pTools);
            xllm_request_reset(&tMemoryRequest);
            xllm_error_free(&tError);
            xwork_memory_context_reset(&tMemoryContext);
            iStatus = xwork__fail_run_with_summary(
                pRun,
                sErrorSummary ? sErrorSummary : "xllm session chat failed."
            );
            free(sErrorSummary);
            return iStatus == XWORK_OK ? XWORK_ERROR_EXTERNAL_FAILURE : iStatus;
        }

        free(pTools);
        xllm_request_reset(&tMemoryRequest);
        xllm_error_free(&tError);
        xwork_memory_context_reset(&tMemoryContext);

        sVisibleText = xllm_response_get_text(pResponse);
        if ( xllm_response_get_tool_call_count(pResponse) > 0u ) {
            pToolCall = xllm_response_get_tool_call(pResponse, 0u);
        }

        if ( pToolCall ) {
            const char *sToolId = xwork__tool_call_id(pToolCall);
            const xwork_tool_def *pToolDef = xwork_runtime_find_tool(pRun->pRuntime, sToolId);
            xwork_tool_call tToolCall;
            xwork_tool_result tToolResult;
            xwork_approval_decision tApprovalDecision;
            size_t iArtifactCountBefore = 0u;

            if ( !sToolId || !pToolDef ) {
                xllm_response_free(pResponse);
                return xwork__fail_run_with_summary(
                    pRun,
                    "Model requested an unknown tool."
                );
            }

            iStatus = xwork__store_tool_call(pRun, pToolCall);
            if ( iStatus != XWORK_OK ) {
                xllm_response_free(pResponse);
                return iStatus;
            }

            iStatus = xwork__run_record_event(
                pRun,
                XWORK_EVENT_MODEL_TURN_COMPLETED,
                sToolId,
                pRun->sLastApprovalRequestId,
                pRun->sLastCheckpointId,
                "Model requested a tool call."
            );
            if ( iStatus != XWORK_OK ) {
                xllm_response_free(pResponse);
                return iStatus;
            }

            iStatus = xwork__run_record_event(
                pRun,
                XWORK_EVENT_TOOL_CALL_REQUESTED,
                sToolId,
                pRun->sLastApprovalRequestId,
                pRun->sLastCheckpointId,
                "Tool call requested."
            );
            if ( iStatus != XWORK_OK ) {
                xllm_response_free(pResponse);
                return iStatus;
            }

            xwork_approval_decision_init(&tApprovalDecision);
            iStatus = xwork__evaluate_tool_approval(
                pRun,
                pToolDef,
                pExecOptions,
                &tApprovalDecision
            );
            if ( iStatus != XWORK_OK ) {
                xllm_response_free(pResponse);
                return iStatus;
            }

            if ( tApprovalDecision.bRequiresApproval ) {
                pRun->eState = XWORK_RUN_WAITING_APPROVAL;
                iStatus = xwork__run_record_approval_request(
                    pRun,
                    NULL,
                    sToolId,
                    tApprovalDecision.sReason,
                    tApprovalDecision.eRiskLevel,
                    tApprovalDecision.sScope,
                    pToolDef->sDescription ? pToolDef->sDescription : sToolId,
                    XWORK_APPROVAL_PENDING
                );
                if ( iStatus != XWORK_OK ) {
                    xllm_response_free(pResponse);
                    return iStatus;
                }

                iStatus = xwork__run_record_event(
                    pRun,
                    XWORK_EVENT_APPROVAL_REQUESTED,
                    sToolId,
                    pRun->sLastApprovalRequestId,
                    pRun->sLastCheckpointId,
                    "Approval requested for tool execution."
                );
                if ( iStatus != XWORK_OK ) {
                    xllm_response_free(pResponse);
                    return iStatus;
                }

                if ( !tApprovalDecision.bAutoApproved ) {
                    xllm_response_free(pResponse);
                    return xwork__save_checkpoint(
                        pRun,
                        XWORK_CHECKPOINT_BEFORE_TOOL,
                        "approval",
                        NULL,
                        "Checkpoint saved while waiting for approval."
                    );
                }

                pRun->eState = XWORK_RUN_RUNNING;
                iStatus = xwork__run_record_approval_request(
                    pRun,
                    pRun->sLastApprovalRequestId,
                    sToolId,
                    tApprovalDecision.sReason,
                    tApprovalDecision.eRiskLevel,
                    tApprovalDecision.sScope,
                    pToolDef->sDescription ? pToolDef->sDescription : sToolId,
                    XWORK_APPROVAL_APPROVED
                );
                if ( iStatus != XWORK_OK ) {
                    xllm_response_free(pResponse);
                    return iStatus;
                }

                iStatus = xwork__run_record_event(
                    pRun,
                    XWORK_EVENT_APPROVAL_RESOLVED,
                    sToolId,
                    pRun->sLastApprovalRequestId,
                    pRun->sLastCheckpointId,
                    "Approval automatically granted."
                );
                if ( iStatus != XWORK_OK ) {
                    xllm_response_free(pResponse);
                    return iStatus;
                }
            }

            memset(&tToolCall, 0, sizeof(tToolCall));
            xwork_tool_result_init(&tToolResult);
            tToolCall.sCallId = pToolCall->sCallId;
            tToolCall.sToolId = sToolId;
            tToolCall.sArgumentsJson = pToolCall->sArgumentsJson;

            pRun->eState = XWORK_RUN_WAITING_TOOL;
            iStatus = xwork__run_record_event(
                pRun,
                XWORK_EVENT_TOOL_EXEC_STARTED,
                sToolId,
                pRun->sLastApprovalRequestId,
                pRun->sLastCheckpointId,
                "Starting tool execution."
            );
            if ( iStatus != XWORK_OK ) {
                xllm_response_free(pResponse);
                return iStatus;
            }

            pRun->eState = XWORK_RUN_RUNNING;
            iArtifactCountBefore = pRun->iArtifactCount;
            iStatus = xwork__execute_tool(
                pRun,
                pToolDef,
                &tToolCall,
                &tToolResult,
                pExecOptions
            );
            if ( iStatus != XWORK_OK ) {
                xllm_response_free(pResponse);
                xwork__fail_run_after_tool_error(
                    pRun,
                    &tToolResult,
                    "Tool execution failed."
                );
                return iStatus;
            }

            iStatus = xwork__store_tool_result(pRun, &tToolResult);
            if ( iStatus != XWORK_OK ) {
                xllm_response_free(pResponse);
                return iStatus;
            }

            iStatus = xwork__emit_builtin_tool_artifact(
                pRun,
                pToolDef,
                &tToolCall,
                &tToolResult
            );
            if ( iStatus != XWORK_OK ) {
                xllm_response_free(pResponse);
                return iStatus;
            }

            if ( pExecOptions->bIngestToolResultsToMemory ) {
                iStatus = xwork__ingest_tool_result_to_memory(pRun, &tToolCall, &tToolResult);
                if ( iStatus != XWORK_OK ) {
                    xllm_response_free(pResponse);
                    return iStatus;
                }
            }

            if ( pExecOptions->bIngestArtifactsToMemory &&
                 pRun->iArtifactCount > iArtifactCountBefore ) {
                iStatus = xwork__ingest_artifacts_to_memory_range(pRun, iArtifactCountBefore);
                if ( iStatus != XWORK_OK ) {
                    xllm_response_free(pResponse);
                    return iStatus;
                }
            }

            iStatus = xwork__run_record_event(
                pRun,
                XWORK_EVENT_TOOL_EXEC_COMPLETED,
                sToolId,
                pRun->sLastApprovalRequestId,
                pRun->sLastCheckpointId,
                tToolResult.sVisibleSummary ? tToolResult.sVisibleSummary : "Tool execution completed."
            );
            if ( iStatus != XWORK_OK ) {
                xllm_response_free(pResponse);
                return iStatus;
            }

            iStatus = xwork__save_checkpoint(
                pRun,
                XWORK_CHECKPOINT_AFTER_TOOL,
                "model_turn",
                pRun->sLastToolResultText,
                "Checkpoint saved after tool execution."
            );
            xllm_response_free(pResponse);
            if ( iStatus != XWORK_OK ) {
                return iStatus;
            }
            continue;
        }

        iStatus = xwork__run_record_event(
            pRun,
            XWORK_EVENT_MODEL_TURN_COMPLETED,
            pRun->sLastToolId,
            pRun->sLastApprovalRequestId,
            pRun->sLastCheckpointId,
            sVisibleText && sVisibleText[0] ? sVisibleText : "Model turn completed."
        );
        if ( iStatus != XWORK_OK ) {
            xllm_response_free(pResponse);
            return iStatus;
        }

        iStatus = xwork__store_final_output(pRun, sVisibleText);
        if ( iStatus != XWORK_OK ) {
            xllm_response_free(pResponse);
            return iStatus;
        }

        pRun->eState = XWORK_RUN_COMPLETED;
        iStatus = xwork__save_checkpoint(
            pRun,
            XWORK_CHECKPOINT_COMPLETION,
            "done",
            pRun->sLastToolResultText,
            "Checkpoint saved at completion."
        );
        xllm_response_free(pResponse);
        if ( iStatus != XWORK_OK ) {
            return iStatus;
        }

        return xwork__run_record_event(
            pRun,
            XWORK_EVENT_RUN_COMPLETED,
            pRun->sLastToolId,
            pRun->sLastApprovalRequestId,
            pRun->sLastCheckpointId,
            "Run completed."
        );
    }

    return xwork__record_run_state_event(
        pRun,
        XWORK_RUN_PAUSED,
        XWORK_EVENT_RUN_PAUSED,
        "Run paused after reaching the max turn budget."
    );
}
