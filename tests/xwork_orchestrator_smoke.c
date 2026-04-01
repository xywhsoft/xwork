#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define XWORK_TEST_RMDIR _rmdir
#else
#include <unistd.h>
#define XWORK_TEST_RMDIR rmdir
#endif

#define XRT_IMPLEMENTATION
#include "../lib/xrt.h"

#define XLLM_SESSION_IMPLEMENTATION
#include "../lib/xllm-session.h"

#define XLLM_MEMORY_IMPLEMENTATION
#include "../lib/xllm-memory.h"

#include "../xwork.c"

typedef struct {
    int iTurnCount;
    const char *sExpectedMemoryText;
    int iObservedMemoryTurns;
} xwork_mock_adapter_ctx;

typedef struct {
    int iExecCount;
} xwork_mock_tool_exec_ctx;

typedef struct {
    int iInvokeCount;
} xwork_mock_host_ctx;

typedef struct {
    int iResolveCount;
    const char *sContextText;
} xwork_mock_memory_ctx;

typedef struct {
    int iStoreEventCount;
    int iStoreCheckpointCount;
    int iStoreArtifactCount;
    int iLoadSnapshotCount;
    bool bHasSnapshot;
    xwork_run_snapshot tSnapshot;
} xwork_mock_persistence_ctx;

#ifdef _WIN32
#define XWORK_TEST_PROCESS_ENV_COMMAND "set XWORK_TEST_PROCESS_ENV"
#define XWORK_TEST_PROCESS_ENV_EXPECTED "XWORK_TEST_PROCESS_ENV=xwork-process-env"
#define XWORK_TEST_PROCESS_STDIN_COMMAND "more"
#define XWORK_TEST_PROCESS_NONZERO_COMMAND_RAW \
    "cmd /c \"echo xwork-local-process-nonzero && exit /b 3\""
#define XWORK_TEST_PROCESS_NONZERO_COMMAND \
    "cmd /c \\\"echo xwork-local-process-nonzero && exit /b 3\\\""
#else
#define XWORK_TEST_PROCESS_ENV_COMMAND "printf %s \"$XWORK_TEST_PROCESS_ENV\""
#define XWORK_TEST_PROCESS_ENV_EXPECTED "xwork-process-env"
#define XWORK_TEST_PROCESS_STDIN_COMMAND "cat"
#define XWORK_TEST_PROCESS_NONZERO_COMMAND_RAW \
    "sh -c 'printf %s xwork-local-process-nonzero; exit 3'"
#define XWORK_TEST_PROCESS_NONZERO_COMMAND XWORK_TEST_PROCESS_NONZERO_COMMAND_RAW
#endif

#define XWORK_TEST_PROCESS_STDIN_INPUT "xwork-process-stdin\\n"
#define XWORK_TEST_PROCESS_STDIN_EXPECTED "xwork-process-stdin"
#define XWORK_TEST_PROCESS_NONZERO_EXPECTED "xwork-local-process-nonzero"
#define XWORK_TEST_PROCESS_NONZERO_EXIT_CODE 3

static char *xwork_test_dup_cstr(const char *sText)
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

static void xwork_test_remove_empty_directory(const char *sPath)
{
    if ( !sPath || !sPath[0] ) {
        return;
    }
    (void)XWORK_TEST_RMDIR(sPath);
}

static void xwork_test_init_custom_session_policy(xwork_session_policy *pPolicy)
{
    if ( !pPolicy ) {
        return;
    }

    xwork_session_policy_init(pPolicy);
    pPolicy->bEnableAutoCompact = false;
    pPolicy->fCompactTriggerRatio = 0.60;
    pPolicy->iCompactTriggerTurns = 5u;
}

static int xwork_test_create_memory(
    xllm_runtime *pRuntime,
    const char *sNamespace,
    const char *sRecordId,
    const char *sTitle,
    const char *sSourceUri,
    const char *sText,
    xllm_memory **ppMemory
)
{
    xllm_memory_options tMemoryOptions;
    xllm_memory_ingest_options tIngestOptions;
    xllm_error tError;
    int iStatus;

    if ( !pRuntime || !sNamespace || !sRecordId || !sText || !ppMemory ) {
        return XRT_NET_ERROR;
    }

    *ppMemory = NULL;
    xllm_memory_options_init(&tMemoryOptions);
    xllm_memory_ingest_options_init(&tIngestOptions);
    xllm_error_init(&tError);

    tMemoryOptions.sNamespace = sNamespace;
    tMemoryOptions.sSqlitePath = ":memory:";
    tMemoryOptions.eScheme = XLLM_MEMORY_SCHEME_BUILTIN_SPARSE;
    iStatus = xllm_memory_create(pRuntime, &tMemoryOptions, ppMemory);
    if ( iStatus != XRT_NET_OK ) {
        xllm_error_free(&tError);
        return iStatus;
    }

    tIngestOptions.sRecordId = sRecordId;
    tIngestOptions.sTitle = sTitle;
    tIngestOptions.sSourceUri = sSourceUri;
    tIngestOptions.sText = sText;
    iStatus = xllm_memory_ingest_text(*ppMemory, &tIngestOptions, &tError);
    if ( iStatus != XRT_NET_OK ) {
        xllm_memory_destroy(*ppMemory);
        *ppMemory = NULL;
        xllm_error_free(&tError);
        return iStatus;
    }
    xllm_error_free(&tError);

    return XRT_NET_OK;
}

static xwork_status xwork_test_snapshot_copy_artifacts(
    xwork_run_snapshot *pTarget,
    const xwork_run_snapshot *pSource
)
{
    xwork_artifact *pArtifacts;
    size_t i;

    if ( !pTarget || !pSource ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( pSource->iArtifactCount == 0u ) {
        pTarget->pArtifacts = NULL;
        pTarget->iArtifactCount = 0u;
        return XWORK_OK;
    }

    pArtifacts = (xwork_artifact *)calloc(pSource->iArtifactCount, sizeof(*pArtifacts));
    if ( !pArtifacts ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    pTarget->pArtifacts = pArtifacts;
    pTarget->iArtifactCount = pSource->iArtifactCount;

    for ( i = 0u; i < pSource->iArtifactCount; ++i ) {
        xwork_status iStatus;

        xwork_artifact_init(&pArtifacts[i]);
        iStatus = xwork__artifact_snapshot_replace_cstr(
            &pArtifacts[i].sArtifactId,
            pSource->pArtifacts[i].sArtifactId
        );
        if ( iStatus != XWORK_OK ) {
            xwork__run_snapshot_reset_artifacts(pTarget);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(
            &pArtifacts[i].sName,
            pSource->pArtifacts[i].sName
        );
        if ( iStatus != XWORK_OK ) {
            xwork__run_snapshot_reset_artifacts(pTarget);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(
            &pArtifacts[i].sMimeType,
            pSource->pArtifacts[i].sMimeType
        );
        if ( iStatus != XWORK_OK ) {
            xwork__run_snapshot_reset_artifacts(pTarget);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(
            &pArtifacts[i].sStorageRef,
            pSource->pArtifacts[i].sStorageRef
        );
        if ( iStatus != XWORK_OK ) {
            xwork__run_snapshot_reset_artifacts(pTarget);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(
            &pArtifacts[i].sSummary,
            pSource->pArtifacts[i].sSummary
        );
        if ( iStatus != XWORK_OK ) {
            xwork__run_snapshot_reset_artifacts(pTarget);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(
            &pArtifacts[i].sContentText,
            pSource->pArtifacts[i].sContentText
        );
        if ( iStatus != XWORK_OK ) {
            xwork__run_snapshot_reset_artifacts(pTarget);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(
            &pArtifacts[i].sCommandText,
            pSource->pArtifacts[i].sCommandText
        );
        if ( iStatus != XWORK_OK ) {
            xwork__run_snapshot_reset_artifacts(pTarget);
            return iStatus;
        }

        pArtifacts[i].sRunId = pTarget->sRunId;
        pArtifacts[i].eKind = pSource->pArtifacts[i].eKind;
        pArtifacts[i].bHasExitCode = pSource->pArtifacts[i].bHasExitCode;
        pArtifacts[i].iExitCode = pSource->pArtifacts[i].iExitCode;
        pArtifacts[i].iSequence = pSource->pArtifacts[i].iSequence;
    }

    return XWORK_OK;
}

static xwork_status xwork_test_snapshot_copy(
    xwork_run_snapshot *pTarget,
    const xwork_run_snapshot *pSource
)
{
    xwork_status iStatus;

    if ( !pTarget || !pSource ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    xwork_run_snapshot_reset(pTarget);

    iStatus = xwork__run_snapshot_replace_cstr(&pTarget->sRunId, pSource->sRunId);
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(&pTarget->sParentRunId, pSource->sParentRunId);
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(&pTarget->sInstruction, pSource->sInstruction);
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(&pTarget->sLlmProfileId, pSource->sLlmProfileId);
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sSessionProfileId,
        pSource->sSessionProfileId
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    pTarget->tSessionPolicy = pSource->tSessionPolicy;
    iStatus = xwork__run_snapshot_copy_workspace_ids(
        pTarget,
        pSource->psWorkspaceIds,
        pSource->iWorkspaceCount
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastOutputText,
        pSource->sLastOutputText
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastMemoryContextText,
        pSource->sLastMemoryContextText
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastToolCallId,
        pSource->sLastToolCallId
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(&pTarget->sLastToolId, pSource->sLastToolId);
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastToolArgumentsJson,
        pSource->sLastToolArgumentsJson
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastToolResultText,
        pSource->sLastToolResultText
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastToolVisibleSummary,
        pSource->sLastToolVisibleSummary
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastApprovalRequestId,
        pSource->sLastApprovalRequestId
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastApprovalToolId,
        pSource->sLastApprovalToolId
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastApprovalReason,
        pSource->sLastApprovalReason
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastApprovalScope,
        pSource->sLastApprovalScope
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastApprovalActionSummary,
        pSource->sLastApprovalActionSummary
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastCheckpointId,
        pSource->sLastCheckpointId
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastCheckpointPendingStep,
        pSource->sLastCheckpointPendingStep
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastCheckpointSessionStateRef,
        pSource->sLastCheckpointSessionStateRef
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastCheckpointToolOutputsRef,
        pSource->sLastCheckpointToolOutputsRef
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastCheckpointWorkspaceSnapshotRef,
        pSource->sLastCheckpointWorkspaceSnapshotRef
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sLastCheckpointArtifactRefs,
        pSource->sLastCheckpointArtifactRefs
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork__run_snapshot_replace_cstr(
        &pTarget->sSessionStateData,
        pSource->sSessionStateData
    );
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }
    iStatus = xwork_test_snapshot_copy_artifacts(pTarget, pSource);
    if ( iStatus != XWORK_OK ) {
        goto fail;
    }

    pTarget->eAutonomy = pSource->eAutonomy;
    pTarget->eState = pSource->eState;
    pTarget->bHasMemoryContext = pSource->bHasMemoryContext;
    pTarget->iLastMemoryWorkspaceCount = pSource->iLastMemoryWorkspaceCount;
    pTarget->bHasToolCall = pSource->bHasToolCall;
    pTarget->bHasToolResult = pSource->bHasToolResult;
    pTarget->bHasApprovalRequest = pSource->bHasApprovalRequest;
    pTarget->eLastApprovalRiskLevel = pSource->eLastApprovalRiskLevel;
    pTarget->eLastApprovalState = pSource->eLastApprovalState;
    pTarget->iLastApprovalSequence = pSource->iLastApprovalSequence;
    pTarget->bHasCheckpoint = pSource->bHasCheckpoint;
    pTarget->eLastCheckpointKind = pSource->eLastCheckpointKind;
    pTarget->eLastCheckpointRunState = pSource->eLastCheckpointRunState;
    pTarget->iLastCheckpointSequence = pSource->iLastCheckpointSequence;
    pTarget->iNextEventSequence = pSource->iNextEventSequence;
    pTarget->iNextArtifactSequence = pSource->iNextArtifactSequence;
    pTarget->iNextApprovalSequence = pSource->iNextApprovalSequence;
    pTarget->iNextCheckpointSequence = pSource->iNextCheckpointSequence;
    return XWORK_OK;

fail:
    xwork_run_snapshot_reset(pTarget);
    return iStatus;
}

static xwork_status xwork_mock_persistence_store_event(
    const xwork_event *pEvent,
    void *pUserData
)
{
    xwork_mock_persistence_ctx *pCtx = (xwork_mock_persistence_ctx *)pUserData;

    if ( !pEvent || !pCtx ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    ++pCtx->iStoreEventCount;
    return XWORK_OK;
}

static xwork_status xwork_mock_persistence_store_checkpoint(
    const xwork_checkpoint *pCheckpoint,
    const xwork_run_snapshot *pSnapshot,
    void *pUserData
)
{
    xwork_mock_persistence_ctx *pCtx = (xwork_mock_persistence_ctx *)pUserData;
    xwork_status iStatus;

    if ( !pCheckpoint || !pSnapshot || !pCtx ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    ++pCtx->iStoreCheckpointCount;
    iStatus = xwork_test_snapshot_copy(&pCtx->tSnapshot, pSnapshot);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    pCtx->bHasSnapshot = true;
    return XWORK_OK;
}

static xwork_status xwork_mock_persistence_store_artifact(
    const xwork_artifact *pArtifact,
    void *pUserData
)
{
    xwork_mock_persistence_ctx *pCtx = (xwork_mock_persistence_ctx *)pUserData;

    if ( !pArtifact || !pCtx ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    ++pCtx->iStoreArtifactCount;
    return XWORK_OK;
}

static xwork_status xwork_mock_persistence_load_run_snapshot(
    const char *sRunId,
    xwork_run_snapshot *pSnapshot,
    void *pUserData
)
{
    xwork_mock_persistence_ctx *pCtx = (xwork_mock_persistence_ctx *)pUserData;

    if ( !sRunId || !sRunId[0] || !pSnapshot || !pCtx ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    ++pCtx->iLoadSnapshotCount;
    if ( !pCtx->bHasSnapshot ||
         !pCtx->tSnapshot.sRunId ||
         strcmp(pCtx->tSnapshot.sRunId, sRunId) != 0 ) {
        return XWORK_ERROR_NOT_FOUND;
    }

    return xwork_test_snapshot_copy(pSnapshot, &pCtx->tSnapshot);
}

static int32 xwork_mock_count_tokens(
    void *pCtx,
    const xllm_profile *pProfile,
    const xllm_request *pRequest,
    xllm_token_count_result *pResult,
    xllm_error *pError
)
{
    (void)pCtx;
    (void)pProfile;
    (void)pRequest;
    (void)pError;

    if ( pResult ) {
        memset(pResult, 0, sizeof(*pResult));
    }
    return XRT_NET_OK;
}

static const char *xwork_mock_first_text(const xllm_message *pMessage)
{
    if ( !pMessage || !pMessage->pParts || pMessage->iPartCount == 0u ) {
        return NULL;
    }
    if ( pMessage->pParts[0].eKind != XLLM_PART_TEXT ) {
        return NULL;
    }
    if ( pMessage->pParts[0].as.tSource.eKind != XLLM_SOURCE_INLINE_TEXT ) {
        return NULL;
    }
    return pMessage->pParts[0].as.tSource.as.sText;
}

static const char *xwork_mock_first_context_text(const xllm_request *pRequest)
{
    if ( !pRequest || !pRequest->pContextBlocks || pRequest->iContextBlockCount == 0u ) {
        return NULL;
    }
    if ( !pRequest->pContextBlocks[0].pMessages ||
         pRequest->pContextBlocks[0].iMessageCount == 0u ) {
        return NULL;
    }
    return xwork_mock_first_text(&pRequest->pContextBlocks[0].pMessages[0]);
}

static int xwork_mock_fill_response_base(
    xllm_response *pResponse,
    const char *sId,
    const char *sProfileId,
    xllm_response_status eStatus,
    const char *sFinishReason
)
{
    if ( !pResponse ) {
        return 0;
    }

    pResponse->sId = xwork_test_dup_cstr(sId);
    pResponse->sProvider = xwork_test_dup_cstr("mock");
    pResponse->sProfileId = xwork_test_dup_cstr(sProfileId);
    pResponse->sModel = xwork_test_dup_cstr("mock-model");
    pResponse->sFinishReason = xwork_test_dup_cstr(sFinishReason);
    pResponse->eStatus = eStatus;

    return pResponse->sId &&
           pResponse->sProvider &&
           pResponse->sProfileId &&
           pResponse->sModel &&
           pResponse->sFinishReason;
}

static xllm_response *xwork_mock_build_tool_call_response(
    const char *sProfileId,
    const char *sResponseId,
    const char *sToolId,
    const char *sArgumentsJson
)
{
    xllm_response *pResponse;

    pResponse = (xllm_response *)xrtCalloc(1u, sizeof(*pResponse));
    if ( !pResponse ) {
        return NULL;
    }

    if ( !xwork_mock_fill_response_base(
            pResponse,
            sResponseId,
            sProfileId,
            XLLM_STATUS_TOOL_CALL_REQUIRED,
            "tool_call"
        ) ) {
        xllm_response_free(pResponse);
        return NULL;
    }

    pResponse->pOutputs = (xllm_output_item *)xrtCalloc(1u, sizeof(xllm_output_item));
    if ( !pResponse->pOutputs ) {
        xllm_response_free(pResponse);
        return NULL;
    }

    pResponse->iOutputCount = 1u;
    pResponse->pOutputs[0].eKind = XLLM_OUTPUT_TOOL_CALL;
    pResponse->pOutputs[0].as.tToolCall.sCallId = xwork_test_dup_cstr("tool-call-1");
    pResponse->pOutputs[0].as.tToolCall.sToolId = xwork_test_dup_cstr(sToolId);
    pResponse->pOutputs[0].as.tToolCall.sToolName = xwork_test_dup_cstr(sToolId);
    pResponse->pOutputs[0].as.tToolCall.sArgumentsJson = xwork_test_dup_cstr(sArgumentsJson);

    if ( !pResponse->pOutputs[0].as.tToolCall.sCallId ||
         !pResponse->pOutputs[0].as.tToolCall.sToolId ||
         !pResponse->pOutputs[0].as.tToolCall.sToolName ||
         !pResponse->pOutputs[0].as.tToolCall.sArgumentsJson ) {
        xllm_response_free(pResponse);
        return NULL;
    }

    return pResponse;
}

static xllm_response *xwork_mock_build_final_response(
    const char *sProfileId,
    const char *sText
)
{
    xllm_response *pResponse;
    xllm_content_part *pPart;

    pResponse = (xllm_response *)xrtCalloc(1u, sizeof(*pResponse));
    if ( !pResponse ) {
        return NULL;
    }

    if ( !xwork_mock_fill_response_base(
            pResponse,
            "mock-response-2",
            sProfileId,
            XLLM_STATUS_COMPLETED,
            "stop"
        ) ) {
        xllm_response_free(pResponse);
        return NULL;
    }

    pResponse->sVisibleText = xwork_test_dup_cstr(sText);
    pResponse->pOutputs = (xllm_output_item *)xrtCalloc(1u, sizeof(xllm_output_item));
    if ( !pResponse->sVisibleText || !pResponse->pOutputs ) {
        xllm_response_free(pResponse);
        return NULL;
    }

    pResponse->iOutputCount = 1u;
    pResponse->pOutputs[0].eKind = XLLM_OUTPUT_MESSAGE;
    pResponse->pOutputs[0].as.tMessage.pParts =
        (xllm_content_part *)xrtCalloc(1u, sizeof(xllm_content_part));
    if ( !pResponse->pOutputs[0].as.tMessage.pParts ) {
        xllm_response_free(pResponse);
        return NULL;
    }

    pResponse->pOutputs[0].as.tMessage.iPartCount = 1u;
    pPart = &pResponse->pOutputs[0].as.tMessage.pParts[0];
    memset(pPart, 0, sizeof(*pPart));
    pPart->eKind = XLLM_PART_TEXT;
    pPart->as.tSource.eKind = XLLM_SOURCE_INLINE_TEXT;
    pPart->as.tSource.as.sText = xwork_test_dup_cstr(sText);
    if ( !pPart->as.tSource.as.sText ) {
        xllm_response_free(pResponse);
        return NULL;
    }

    return pResponse;
}

static int32 xwork_mock_adapter_chat(
    void *pCtx,
    const xllm_profile *pProfile,
    const xllm_request *pRequest,
    const xllm_call_options *pOptions,
    xllm_response **ppResponse,
    xllm_error *pError
)
{
    xwork_mock_adapter_ctx *pState = (xwork_mock_adapter_ctx *)pCtx;
    size_t iUserIndex;
    size_t iAssistantIndex;
    size_t iToolIndex;
    bool bHasSystemMessage;
    bool bHasInlineSystemMessage;
    const char *sSystemText = NULL;
    const char *sInstruction;
    const char *sToolText;
    const char *sToolId;

    (void)pOptions;
    (void)pError;

    if ( !pState || !pProfile || !pRequest || !ppResponse ) {
        return XRT_NET_ERROR;
    }

    *ppResponse = NULL;
    bHasInlineSystemMessage = (
        pRequest->iMessageCount > 0u &&
        pRequest->pMessages[0].eRole == XLLM_ROLE_SYSTEM
    );
    bHasSystemMessage = bHasInlineSystemMessage;
    if ( bHasInlineSystemMessage ) {
        sSystemText = xwork_mock_first_text(&pRequest->pMessages[0]);
    } else {
        sSystemText = xwork_mock_first_context_text(pRequest);
        bHasSystemMessage = sSystemText != NULL;
    }
    if ( bHasSystemMessage ) {
        if ( !pState->sExpectedMemoryText ||
             !sSystemText ||
             strstr(sSystemText, pState->sExpectedMemoryText) == NULL ) {
            return XRT_NET_ERROR;
        }
        ++pState->iObservedMemoryTurns;
    } else if ( pState->sExpectedMemoryText ) {
        return XRT_NET_ERROR;
    }

    iUserIndex = bHasInlineSystemMessage ? 1u : 0u;
    sInstruction = xwork_mock_first_text(&pRequest->pMessages[iUserIndex]);

    if ( pState->iTurnCount == 0 ) {
        if ( pRequest->iMessageCount != (bHasInlineSystemMessage ? 2u : 1u) ) {
            return XRT_NET_ERROR;
        }
        if ( sInstruction && strstr(sInstruction, "Read the README through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-host",
                "filesystem.read_text",
                "{\"path\":\"README.md\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Read a missing local note through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-host-missing",
                "filesystem.read_text",
                "{\"path\":\"tests/local_host_missing_smoke.txt\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Read part of a local note through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-host-offset",
                "filesystem.read_text",
                "{\"path\":\"tests/local_host_write_smoke.txt\","
                "\"offset_bytes\":6,\"max_bytes\":4}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Write a note through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-write",
                "filesystem.write_text",
                "{\"path\":\"tests/local_host_orchestrator_write_smoke.txt\","
                "\"text\":\"xwork-write-note\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Create a fresh note through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-write-create",
                "filesystem.write_text",
                "{\"path\":\"tests/local_host_orchestrator_create_smoke.txt\","
                "\"text\":\"xwork-create-note\",\"mode\":\"create\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Write a nested note through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-write-create-dirs",
                "filesystem.write_text",
                "{\"path\":\"tests/local_host_nested/orchestrator/"
                "local_host_orchestrator_create_dirs_smoke.txt\","
                "\"text\":\"xwork-create-dirs-note\",\"create_dirs\":true}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Append a note through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-append-write",
                "filesystem.write_text",
                "{\"path\":\"tests/local_host_orchestrator_append_smoke.txt\","
                "\"text\":\"xwork-append-note\",\"mode\":\"append\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process",
                "process.exec",
                "{\"command\":\"echo xwork-local-process\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process in tests through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-cwd",
                "process.exec",
                "{\"command\":\"cd\",\"cwd\":\"tests\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process with output cap through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-truncated",
                "process.exec",
                "{\"command\":\"echo xwork-local-process-truncate\","
                "\"max_output_bytes\":12}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process with env through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-env",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_ENV_COMMAND "\","
                "\"env\":[\"XWORK_TEST_PROCESS_ENV=xwork-process-env\"]}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process with too many env values through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-env-too-many",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_ENV_COMMAND "\","
                "\"env\":[\"XWORK_TEST_PROCESS_ENV=xwork-process-env\","
                "\"XWORK_TEST_PROCESS_ENV_EXTRA=xwork-process-env-extra\"]}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process with stdin through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-stdin",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_STDIN_COMMAND "\","
                "\"stdin_text\":\"" XWORK_TEST_PROCESS_STDIN_INPUT "\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process with non-zero exit through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-nonzero",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_NONZERO_COMMAND "\","
                "\"allow_nonzero_exit\":true}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Read git status through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-vcs",
                "vcs.status",
                "{\"path\":\".\"}"
            );
        } else {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-1",
                "mock.apply_patch",
                "{\"path\":\"README.md\",\"mode\":\"append\"}"
            );
        }
    } else {
        iAssistantIndex = bHasInlineSystemMessage ? 2u : 1u;
        iToolIndex = bHasInlineSystemMessage ? 3u : 2u;
        if ( pRequest->iMessageCount != (bHasInlineSystemMessage ? 4u : 3u) ) {
            return XRT_NET_ERROR;
        }
        if ( pRequest->pMessages[iAssistantIndex].eRole != XLLM_ROLE_ASSISTANT ||
             pRequest->pMessages[iToolIndex].eRole != XLLM_ROLE_TOOL ) {
            return XRT_NET_ERROR;
        }
        sToolId = pRequest->pMessages[iAssistantIndex].pToolCalls[0].sToolId;
        sToolText = xwork_mock_first_text(&pRequest->pMessages[iToolIndex]);
        if ( !sToolId || !sToolText ) {
            return XRT_NET_ERROR;
        }
        if ( strcmp(sToolId, "mock.apply_patch") == 0 ) {
            if ( strcmp(sToolText, "{\"ok\":true,\"changed_files\":[\"README.md\"]}") != 0 ) {
                return XRT_NET_ERROR;
            }
            *ppResponse = xwork_mock_build_final_response(
                pProfile->sId,
                "Mock model completed after tool result."
            );
        } else if ( strcmp(sToolId, "filesystem.read_text") == 0 ) {
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 strstr(sToolText, "\"text\":\"") == NULL ||
                 strstr(sToolText, "\"eof\":") == NULL ||
                 strstr(sToolText, "\"file_size_bytes\":") == NULL ||
                 strstr(sToolText, "\"next_offset_bytes\":") == NULL ||
                 strstr(sToolText, "\"remaining_bytes\":") == NULL ) {
                return XRT_NET_ERROR;
            }
            *ppResponse = xwork_mock_build_final_response(
                pProfile->sId,
                "Host service tool completed."
            );
        } else if ( strcmp(sToolId, "filesystem.write_text") == 0 ) {
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 strstr(sToolText, "\"bytes_written\":") == NULL ) {
                return XRT_NET_ERROR;
            }
            if ( strstr(sToolText, "\"mode\":\"append\"") != NULL ) {
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service append completed."
                );
            } else if ( strstr(sToolText, "\"mode\":\"create\"") != NULL ) {
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service create completed."
                );
            } else if ( strstr(sToolText, "\"create_dirs\":true") != NULL ) {
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service nested write completed."
                );
            } else {
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service write completed."
                );
            }
        } else if ( strcmp(sToolId, "process.exec") == 0 ) {
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 strstr(sToolText, "\"stdout\":\"") == NULL ) {
                return XRT_NET_ERROR;
            }
            if ( strstr(sToolText, "\"truncated\":true") != NULL ) {
                if ( strstr(sToolText, "\"stdout\":\"xwork-local-\"") == NULL ) {
                    return XRT_NET_ERROR;
                }
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service process truncated completed."
                );
            } else if ( strstr(sToolText, XWORK_TEST_PROCESS_ENV_EXPECTED) != NULL ) {
                if ( strstr(sToolText, "\"env_count\":1") == NULL ) {
                    return XRT_NET_ERROR;
                }
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service process env completed."
                );
            } else if ( strstr(sToolText, XWORK_TEST_PROCESS_STDIN_EXPECTED) != NULL ) {
                if ( strstr(sToolText, "\"stdin_bytes\":20") == NULL ) {
                    return XRT_NET_ERROR;
                }
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service process stdin completed."
                );
            } else if ( strstr(sToolText, XWORK_TEST_PROCESS_NONZERO_EXPECTED) != NULL ) {
                if ( strstr(sToolText, "\"exit_code\":3") == NULL ||
                     strstr(sToolText, "\"allow_nonzero_exit\":true") == NULL ) {
                    return XRT_NET_ERROR;
                }
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service process non-zero completed."
                );
            } else if ( strstr(sToolText, "\"cwd\":\"./tests\"") != NULL ) {
                if ( strstr(sToolText, "tests") == NULL ) {
                    return XRT_NET_ERROR;
                }
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service process cwd completed."
                );
            } else {
                if ( strstr(sToolText, "xwork-local-process") == NULL ) {
                    return XRT_NET_ERROR;
                }
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service process completed."
                );
            }
        } else if ( strcmp(sToolId, "vcs.status") == 0 ) {
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 strstr(sToolText, "\"status\":\"") == NULL ) {
                return XRT_NET_ERROR;
            }
            *ppResponse = xwork_mock_build_final_response(
                pProfile->sId,
                "Host service vcs completed."
            );
        } else {
            return XRT_NET_ERROR;
        }
    }

    if ( !*ppResponse ) {
        return XRT_NET_ERROR;
    }

    ++pState->iTurnCount;
    return XRT_NET_OK;
}

static xwork_status xwork_mock_memory_resolve(
    const xwork_run *pRun,
    xwork_memory_context *pContext,
    void *pUserData
)
{
    xwork_mock_memory_ctx *pCtx = (xwork_mock_memory_ctx *)pUserData;

    if ( !pRun || !pContext || !pCtx ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    ++pCtx->iResolveCount;
    pContext->sText = xwork_test_dup_cstr(
        pCtx->sContextText ? pCtx->sContextText : "workspace-memory"
    );
    if ( !pContext->sText ) {
        return XWORK_ERROR_NO_MEMORY;
    }
    pContext->iWorkspaceCount = xwork_run_get_workspace_count(pRun);
    return XWORK_OK;
}

static xwork_status xwork_mock_tool_exec(
    xwork_run *pRun,
    const xwork_tool_call *pCall,
    xwork_tool_result *pResult,
    void *pUserData
)
{
    xwork_mock_tool_exec_ctx *pCtx = (xwork_mock_tool_exec_ctx *)pUserData;
    xwork_patch_artifact_options tArtifactOptions;
    xwork_artifact tArtifact;
    xwork_status iStatus;

    if ( !pRun || !pCall || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !pCall->sToolId ||
         strcmp(pCall->sToolId, "mock.apply_patch") != 0 ) {
        return XWORK_ERROR_NOT_FOUND;
    }

    if ( pCtx ) {
        ++pCtx->iExecCount;
    }

    xwork_patch_artifact_options_init(&tArtifactOptions);
    xwork_artifact_init(&tArtifact);
    tArtifactOptions.sName = "README.patch";
    tArtifactOptions.sTargetRef = "workspace://README.md";
    tArtifactOptions.sSummary = "Patch artifact captured for README.md.";
    tArtifactOptions.sPatchText =
        "--- a/README.md\n"
        "+++ b/README.md\n"
        "@@\n"
        "+xwork smoke patch\n";
    iStatus = xwork_run_emit_patch_artifact(pRun, &tArtifactOptions, &tArtifact);
    xwork_artifact_reset(&tArtifact);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    pResult->sOutputText = "{\"ok\":true,\"changed_files\":[\"README.md\"]}";
    pResult->sVisibleSummary = "mock.apply_patch executed successfully.";
    return XWORK_OK;
}

static xwork_status xwork_mock_host_invoke(
    const char *sOperationId,
    const char *sRequestJson,
    xwork_tool_result *pResult,
    void *pUserData
)
{
    xwork_mock_host_ctx *pCtx = (xwork_mock_host_ctx *)pUserData;

    (void)sRequestJson;

    if ( !sOperationId || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( pCtx ) {
        ++pCtx->iInvokeCount;
    }

    if ( strcmp(sOperationId, "read_text") != 0 ) {
        return XWORK_ERROR_UNSUPPORTED;
    }

    pResult->sOutputText =
        "{\"ok\":true,\"text\":\"README preview\",\"file_size_bytes\":14,\"bytes_read\":14,"
        "\"next_offset_bytes\":14,\"remaining_bytes\":0,\"truncated\":false,\"eof\":true}";
    pResult->sVisibleSummary = "filesystem.read_text ok";
    return XWORK_OK;
}

int main(void)
{
    xwork_mock_adapter_ctx tAdapterCtx;
    xwork_mock_tool_exec_ctx tToolExecCtx;
    xwork_mock_tool_exec_ctx tDefaultMemoryToolExecCtx;
    xwork_mock_host_ctx tHostCtx;
    xwork_mock_memory_ctx tMemoryCtx;
    xwork_mock_persistence_ctx tPersistenceCtx;
    xllm_runtime_options tLlmRuntimeOptions;
    xllm_runtime *pLlmRuntime = NULL;
    xllm_memory *pWorkspaceMemory = NULL;
    xllm_memory *pFileWorkspaceMemory = NULL;
    xllm_adapter tAdapter;
    xllm_profile tProfile;
    xwork_runtime_options tRuntimeOptions;
    xwork_runtime_options tFileRuntimeOptions;
    xwork_runtime_options tLocalHostRuntimeOptions;
    xwork_host_services tHostServices;
    xwork_host_services tLocalHostServices;
    xwork_persistence_backend tPersistenceBackend;
    xwork_persistence_backend tFilePersistenceBackend;
    xwork_local_host_options tLocalHostOptions;
    xwork_local_host tLocalHost;
    xwork_policy_options tPolicyOptions;
    xwork_policy_options tPolicyCopy;
    xwork_approval_eval_input tApprovalInput;
    xwork_approval_decision tApprovalDecision;
    xwork_profile tInteractiveProfile;
    xwork_profile tAutonomousProfile;
    xwork_runtime_options tProfileRuntimeOptions;
    xwork_xllm_profile_options tProfileLlmOptions;
    xwork_xllm_bootstrap_options tProfileBootstrapOptions;
    xwork_workspace_options tProfileWorkspaceOptions;
    xwork_run_options tProfileRunOptions;
    xwork_orchestrator_options tProfileExecOptions;
    xwork_runtime *pProfileBootstrapRuntime = NULL;
    xwork_runtime *pRuntime = NULL;
    xwork_runtime *pFileRuntime = NULL;
    xwork_runtime *pFileRecoverRuntime = NULL;
    xwork_runtime *pLocalHostRuntime = NULL;
    xwork_workspace_options tWorkspaceOptions;
    xwork_workspace *pProfileBootstrapWorkspace = NULL;
    xwork_workspace *pWorkspace = NULL;
    xwork_workspace *pMemoryWorkspace = NULL;
    xwork_workspace *pFileWorkspace = NULL;
    xwork_workspace *pFileRecoverWorkspace = NULL;
    xwork_workspace *pLocalHostWorkspace = NULL;
    const xwork_tool_def *pBuiltinToolDef = NULL;
    xwork_tool_def tToolDef;
    xwork_file_persistence_options tFilePersistenceOptions;
    xwork_file_persistence tFilePersistenceStore;
    xwork_file_persistence tFilePersistenceRecoverStore;
    xwork_run_options tRunOptions;
    xwork_run *pRun = NULL;
    xwork_run *pMemoryRun = NULL;
    xwork_run *pRecoveredMemoryRun = NULL;
    xwork_run *pHostRun = NULL;
    xwork_run *pPendingRun = NULL;
    xwork_run *pRecoveredRun = NULL;
    xwork_run *pPersistenceRun = NULL;
    xwork_run *pPersistenceRecoveredRun = NULL;
    xwork_run *pProfileBootstrapRun = NULL;
    xwork_run *pFilePendingRun = NULL;
    xwork_run *pFileRecoveredRun = NULL;
    xwork_run *pFileIndexedRun = NULL;
    xwork_run *pLocalHostRun = NULL;
    xwork_orchestrator_options tExecOptions;
    xwork_orchestrator_options tPendingExecOptions;
    xwork_command_artifact_options tCommandArtifactOptions;
    xwork_approval_request tApproval;
    xwork_approval_request tPendingApproval;
    xwork_checkpoint tCheckpoint;
    xwork_checkpoint tPendingCheckpoint;
    xwork_artifact tArtifact;
    xwork_artifact tLoadedArtifact;
    xwork_run_summary tLoadedSummary;
    xwork_run_summary_list tPersistedRunSummaries;
    xwork_run_index_list tPersistedRunIndex;
    xwork_run_index_query tRunIndexQuery;
    xwork_run_snapshot tRunSnapshot;
    xwork_event tEvent;
    xwork_event tLoadedEvent;
    xwork_event tPendingEvent;
    xwork_memory_context tObservedMemoryContext;
    xwork_tool_result tHostResult;
    xwork_tool_result tLocalHostResult;
    xwork_string_list tPersistedRunIds;
    xwork_string_list tPersistedEventIds;
    xwork_string_list tPersistedCheckpointIds;
    xwork_string_list tPersistedArtifactIds;
    char *sAfterToolCheckpointId = NULL;
    char *sPendingBeforeToolCheckpointId = NULL;
    char *sFileBeforeToolCheckpointId = NULL;
    char *sFirstArtifactId = NULL;
    size_t iSavedMaxProcessInputBytes = 0u;
    size_t iSavedMaxProcessEnvEntries = 0u;
    char sFilePersistenceRoot[256];
    const char *asWorkspaceIds[1];
    const char *sLocalHostMissingPath = "tests/local_host_missing_smoke.txt";
    const char *sLocalHostWritePath = "tests/local_host_write_smoke.txt";
    const char *sLocalHostAppendPath = "tests/local_host_append_smoke.txt";
    const char *sLocalHostCreatePath = "tests/local_host_create_smoke.txt";
    const char *sLocalHostCreateDirsPath =
        "tests/local_host_nested/create_dirs/local_host_create_dirs_smoke.txt";
    const char *sLocalHostCreateDirsDir = "tests/local_host_nested/create_dirs";
    const char *sLocalHostCreateDirsParentDir = "tests/local_host_nested";
    const char *sLocalHostOrchestratorWritePath =
        "tests/local_host_orchestrator_write_smoke.txt";
    const char *sLocalHostOrchestratorAppendPath =
        "tests/local_host_orchestrator_append_smoke.txt";
    const char *sLocalHostOrchestratorCreatePath =
        "tests/local_host_orchestrator_create_smoke.txt";
    const char *sLocalHostOrchestratorCreateDirsPath =
        "tests/local_host_nested/orchestrator/local_host_orchestrator_create_dirs_smoke.txt";
    const char *sLocalHostOrchestratorCreateDirsDir =
        "tests/local_host_nested/orchestrator";

    memset(&tAdapterCtx, 0, sizeof(tAdapterCtx));
    memset(&tToolExecCtx, 0, sizeof(tToolExecCtx));
    memset(&tDefaultMemoryToolExecCtx, 0, sizeof(tDefaultMemoryToolExecCtx));
    memset(&tHostCtx, 0, sizeof(tHostCtx));
    memset(&tMemoryCtx, 0, sizeof(tMemoryCtx));
    memset(&tPersistenceCtx, 0, sizeof(tPersistenceCtx));
    xwork_local_host_options_init(&tLocalHostOptions);
    xwork_local_host_init(&tLocalHost);
    xwork_file_persistence_init(&tFilePersistenceStore);
    xwork_file_persistence_init(&tFilePersistenceRecoverStore);
    xwork_string_list_init(&tPersistedRunIds);
    xwork_string_list_init(&tPersistedEventIds);
    xwork_string_list_init(&tPersistedCheckpointIds);
    xwork_string_list_init(&tPersistedArtifactIds);
    xwork_command_artifact_options_init(&tCommandArtifactOptions);
    xwork_artifact_init(&tArtifact);
    xwork_artifact_init(&tLoadedArtifact);
    xwork_run_summary_init(&tLoadedSummary);
    xwork_run_summary_list_init(&tPersistedRunSummaries);
    xwork_run_index_list_init(&tPersistedRunIndex);
    xwork_run_index_query_init(&tRunIndexQuery);
    xwork_event_init(&tLoadedEvent);
    xwork_run_snapshot_init(&tPersistenceCtx.tSnapshot);
    xwork_memory_context_init(&tObservedMemoryContext);
    xwork_tool_result_init(&tLocalHostResult);
    (void)remove(sLocalHostMissingPath);
    (void)remove(sLocalHostWritePath);
    (void)remove(sLocalHostAppendPath);
    (void)remove(sLocalHostCreatePath);
    (void)remove(sLocalHostCreateDirsPath);
    (void)remove(sLocalHostOrchestratorWritePath);
    (void)remove(sLocalHostOrchestratorAppendPath);
    (void)remove(sLocalHostOrchestratorCreatePath);
    (void)remove(sLocalHostOrchestratorCreateDirsPath);
    xwork_test_remove_empty_directory(sLocalHostCreateDirsDir);
    xwork_test_remove_empty_directory(sLocalHostOrchestratorCreateDirsDir);
    xwork_test_remove_empty_directory(sLocalHostCreateDirsParentDir);

    xllm_runtime_options_init(&tLlmRuntimeOptions);
    assert(xllm_runtime_create(&tLlmRuntimeOptions, &pLlmRuntime) == XRT_NET_OK);
    assert(
        xwork_test_create_memory(
            pLlmRuntime,
            "workspace-memory-main",
            "memory-readme",
            "README Memory",
            "workspace://README.md",
            "workspace-memory:README.md\nAppend a line to the README if needed.",
            &pWorkspaceMemory
        ) == XRT_NET_OK
    );
    assert(
        xwork_test_create_memory(
            pLlmRuntime,
            "workspace-memory-file",
            "file-memory-readme",
            "README Memory",
            "workspace://README.md",
            "workspace-memory:file-persistence\nAppend a line to the README if needed.",
            &pFileWorkspaceMemory
        ) == XRT_NET_OK
    );

    memset(&tAdapter, 0, sizeof(tAdapter));
    tAdapter.sName = "mock";
    tAdapter.pCtx = &tAdapterCtx;
    tAdapter.pfnCountTokens = xwork_mock_count_tokens;
    tAdapter.pfnChat = xwork_mock_adapter_chat;
    assert(xllm_register_adapter(pLlmRuntime, &tAdapter) == XRT_NET_OK);

    xllm_profile_init(&tProfile);
    tProfile.sId = "mock-profile";
    tProfile.sName = "Mock Profile";
    tProfile.sProvider = "mock";
    tProfile.sAdapter = "mock";
    tProfile.tModels.tText.sModelId = "mock-model";
    tProfile.tModels.tText.eCapMode = XLLM_CAP_MODE_EXACT;
    tProfile.tModels.tText.tCaps.uFlags =
        XLLM_CAP_TEXT_IN |
        XLLM_CAP_TEXT_OUT |
        XLLM_CAP_TOOL_CALL_OUT |
        XLLM_CAP_TOOL_RESULT_IN;
    assert(xllm_register_profile(pLlmRuntime, &tProfile) == XRT_NET_OK);

    xwork_host_services_init(&tHostServices);
    tHostServices.tFilesystem.pfnInvoke = xwork_mock_host_invoke;
    tHostServices.tFilesystem.pUserData = &tHostCtx;
    xwork_persistence_backend_init(&tPersistenceBackend);
    tPersistenceBackend.pfnStoreEvent = xwork_mock_persistence_store_event;
    tPersistenceBackend.pfnStoreCheckpoint = xwork_mock_persistence_store_checkpoint;
    tPersistenceBackend.pfnStoreArtifact = xwork_mock_persistence_store_artifact;
    tPersistenceBackend.pfnLoadRunSnapshot = xwork_mock_persistence_load_run_snapshot;
    tPersistenceBackend.pUserData = &tPersistenceCtx;

    xwork_policy_options_init(&tPolicyOptions);
    tPolicyOptions.eAutoApproveRiskLimit = XWORK_RISK_CRITICAL;
    xwork_run_snapshot_init(&tRunSnapshot);

    xwork_runtime_options_init(&tRuntimeOptions);
    tRuntimeOptions.pLlmRuntime = pLlmRuntime;
    tRuntimeOptions.pHostServices = &tHostServices;
    tRuntimeOptions.pPersistenceBackend = &tPersistenceBackend;
    tRuntimeOptions.tPolicy = tPolicyOptions;
    assert(xwork_runtime_create(&tRuntimeOptions, &pRuntime) == XWORK_OK);
    assert(xwork_runtime_get_host_services(pRuntime) != NULL);
    assert(xwork_runtime_get_host_services(pRuntime)->tFilesystem.pfnInvoke == xwork_mock_host_invoke);
    assert(xwork_runtime_get_persistence_backend(pRuntime) != NULL);
    assert(
        xwork_runtime_get_persistence_backend(pRuntime)->pfnStoreCheckpoint ==
        xwork_mock_persistence_store_checkpoint
    );
    assert(xwork_runtime_get_policy_options(pRuntime, &tPolicyCopy) == XWORK_OK);
    assert(tPolicyCopy.eAutoApproveRiskLimit == XWORK_RISK_CRITICAL);

    xwork_tool_result_init(&tHostResult);
    assert(xwork_runtime_invoke_host_service(
        pRuntime,
        XWORK_HOST_FILESYSTEM,
        "read_text",
        "{\"path\":\"README.md\"}",
        &tHostResult
    ) == XWORK_OK);
    assert(strcmp(tHostResult.sVisibleSummary, "filesystem.read_text ok") == 0);
    assert(tHostCtx.iInvokeCount == 1);

    xwork_approval_eval_input_init(&tApprovalInput);
    tApprovalInput.eAutonomy = XWORK_AUTONOMY_SEMI_AUTO;
    tApprovalInput.eApprovalMode = XWORK_APPROVAL_DEFAULT;
    tApprovalInput.eSideEffect = XWORK_SIDE_EFFECT_WORKSPACE_WRITE;
    tApprovalInput.bAutoApproveRequested = true;
    xwork_approval_decision_init(&tApprovalDecision);
    assert(xwork_policy_evaluate_approval(&tPolicyCopy, &tApprovalInput, &tApprovalDecision) == XWORK_OK);
    assert(tApprovalDecision.bRequiresApproval);
    assert(tApprovalDecision.bAutoApproved);
    assert(tApprovalDecision.eRiskLevel == XWORK_RISK_MEDIUM);

    xwork_profile_init(&tInteractiveProfile);
    assert(xwork_profile_get_builtin(XWORK_PROFILE_XCODE, &tInteractiveProfile) == XWORK_OK);
    assert(strcmp(tInteractiveProfile.sProfileId, XWORK_PROFILE_XCODE) == 0);
    assert(tInteractiveProfile.eAutonomy == XWORK_AUTONOMY_SEMI_AUTO);
    assert(tInteractiveProfile.tPolicy.eAutoApproveRiskLimit == XWORK_RISK_LOW);
    assert(strcmp(tInteractiveProfile.sDefaultLlmProfileId, XWORK_PROFILE_XCODE) == 0);
    assert(strcmp(tInteractiveProfile.sDefaultSessionProfileId, XWORK_PROFILE_XCODE) == 0);
    assert(tInteractiveProfile.tSessionPolicy.bEnableAutoCompact);
    assert(tInteractiveProfile.tSessionPolicy.fCompactTriggerRatio == 0.75);
    assert(tInteractiveProfile.tSessionPolicy.iCompactTriggerTurns == 8u);
    assert(tInteractiveProfile.iDefaultMaxTurns == 8u);
    assert(!tInteractiveProfile.bDefaultAutoApprove);
    assert(!tInteractiveProfile.bEnableWorkspaceMemory);

    xwork_runtime_options_init(&tProfileRuntimeOptions);
    assert(xwork_profile_apply_runtime_options(&tInteractiveProfile, &tProfileRuntimeOptions) == XWORK_OK);
    assert(tProfileRuntimeOptions.tPolicy.eAutoApproveRiskLimit == XWORK_RISK_LOW);
    xwork_xllm_profile_options_init(&tProfileLlmOptions);
    xwork_xllm_bootstrap_options_init(&tProfileBootstrapOptions);
    assert(
        xwork_profile_apply_xllm_bootstrap_options(
            &tInteractiveProfile,
            &tProfileLlmOptions,
            &tProfileBootstrapOptions
        ) == XWORK_OK
    );
    assert(strcmp(tProfileLlmOptions.sProfileId, XWORK_PROFILE_XCODE) == 0);
    assert(strcmp(tProfileLlmOptions.sDisplayName, "xcode interactive") == 0);
    assert(tProfileBootstrapOptions.pProfiles == &tProfileLlmOptions);
    assert(tProfileBootstrapOptions.iProfileCount == 1u);
    xwork_xllm_profile_options_init(&tProfileLlmOptions);
    xwork_xllm_bootstrap_options_init(&tProfileBootstrapOptions);
    tProfileBootstrapOptions.pProfiles = (const xwork_xllm_profile_options *)&tAutonomousProfile;
    tProfileBootstrapOptions.iProfileCount = 7u;
    tProfileLlmOptions.sProfileId = "override-xcode-profile";
    tProfileLlmOptions.sDisplayName = "override-xcode-display";
    assert(
        xwork_profile_apply_xllm_bootstrap_options(
            &tInteractiveProfile,
            &tProfileLlmOptions,
            &tProfileBootstrapOptions
        ) == XWORK_OK
    );
    assert(strcmp(tProfileLlmOptions.sProfileId, "override-xcode-profile") == 0);
    assert(strcmp(tProfileLlmOptions.sDisplayName, "override-xcode-display") == 0);
    assert(tProfileBootstrapOptions.pProfiles == (const xwork_xllm_profile_options *)&tAutonomousProfile);
    assert(tProfileBootstrapOptions.iProfileCount == 7u);
    xwork_workspace_options_init(&tProfileWorkspaceOptions);
    assert(xwork_profile_apply_workspace_options(&tInteractiveProfile, &tProfileWorkspaceOptions) == XWORK_OK);
    assert(!tProfileWorkspaceOptions.bEnableMemory);
    xwork_run_options_init(&tProfileRunOptions);
    assert(xwork_profile_apply_run_options(&tInteractiveProfile, &tProfileRunOptions) == XWORK_OK);
    assert(strcmp(tProfileRunOptions.sLlmProfileId, XWORK_PROFILE_XCODE) == 0);
    assert(strcmp(tProfileRunOptions.sSessionProfileId, XWORK_PROFILE_XCODE) == 0);
    assert(tProfileRunOptions.eAutonomy == XWORK_AUTONOMY_SEMI_AUTO);
    assert(tProfileRunOptions.tSessionPolicy.bEnableAutoCompact);
    assert(tProfileRunOptions.tSessionPolicy.fCompactTriggerRatio == 0.75);
    assert(tProfileRunOptions.tSessionPolicy.iCompactTriggerTurns == 8u);
    xwork_run_options_init(&tProfileRunOptions);
    tProfileRunOptions.sLlmProfileId = "override-llm";
    tProfileRunOptions.sSessionProfileId = "override-session";
    assert(xwork_profile_apply_run_options(&tInteractiveProfile, &tProfileRunOptions) == XWORK_OK);
    assert(tProfileRunOptions.eAutonomy == XWORK_AUTONOMY_SEMI_AUTO);
    assert(strcmp(tProfileRunOptions.sLlmProfileId, "override-llm") == 0);
    assert(strcmp(tProfileRunOptions.sSessionProfileId, "override-session") == 0);
    assert(tProfileRunOptions.tSessionPolicy.bEnableAutoCompact);
    assert(tProfileRunOptions.tSessionPolicy.fCompactTriggerRatio == 0.75);
    assert(tProfileRunOptions.tSessionPolicy.iCompactTriggerTurns == 8u);
    xwork_orchestrator_options_init(&tProfileExecOptions);
    assert(
        xwork_profile_apply_orchestrator_options(
            &tInteractiveProfile,
            &tProfileExecOptions
        ) == XWORK_OK
    );
    assert(tProfileExecOptions.iMaxTurns == 8u);
    assert(!tProfileExecOptions.bAutoApprove);
    xwork_xllm_profile_options_init(&tProfileLlmOptions);
    xwork_xllm_bootstrap_options_init(&tProfileBootstrapOptions);
    assert(
        xwork_profile_apply_xllm_bootstrap_options(
            &tInteractiveProfile,
            &tProfileLlmOptions,
            &tProfileBootstrapOptions
        ) == XWORK_OK
    );
    tProfileLlmOptions.sAdapter = XWORK_XLLM_ADAPTER_OPENAI_COMPAT;
    tProfileLlmOptions.sBaseUrl = "http://127.0.0.1:1/v1";
    tProfileLlmOptions.sModelId = "dummy-xcode-model";
    tProfileLlmOptions.iMaxOutputTokens = 64u;
    xwork_runtime_options_init(&tProfileRuntimeOptions);
    assert(xwork_profile_apply_runtime_options(&tInteractiveProfile, &tProfileRuntimeOptions) == XWORK_OK);
    tProfileRuntimeOptions.pLlmBootstrap = &tProfileBootstrapOptions;
    assert(xwork_runtime_create(&tProfileRuntimeOptions, &pProfileBootstrapRuntime) == XWORK_OK);
    xwork_workspace_options_init(&tProfileWorkspaceOptions);
    tProfileWorkspaceOptions.sWorkspaceId = "profile-main";
    tProfileWorkspaceOptions.sRootPath = ".";
    assert(
        xwork_runtime_add_workspace(
            pProfileBootstrapRuntime,
            &tProfileWorkspaceOptions,
            &pProfileBootstrapWorkspace
        ) == XWORK_OK
    );
    asWorkspaceIds[0] = "profile-main";
    xwork_run_options_init(&tProfileRunOptions);
    assert(xwork_profile_apply_run_options(&tInteractiveProfile, &tProfileRunOptions) == XWORK_OK);
    tProfileRunOptions.sRunId = "run-profile-bootstrap";
    tProfileRunOptions.sInstruction = "Profile bootstrap run.";
    tProfileRunOptions.psWorkspaceIds = asWorkspaceIds;
    tProfileRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pProfileBootstrapRuntime,
            &tProfileRunOptions,
            &pProfileBootstrapRun
        ) == XWORK_OK
    );
    assert(xwork_run_get_snapshot(pProfileBootstrapRun, &tRunSnapshot) == XWORK_OK);
    assert(strcmp(tRunSnapshot.sLlmProfileId, XWORK_PROFILE_XCODE) == 0);
    assert(strcmp(tRunSnapshot.sSessionProfileId, XWORK_PROFILE_XCODE) == 0);
    xwork_run_destroy(pProfileBootstrapRun);
    pProfileBootstrapRun = NULL;
    xwork_runtime_destroy(pProfileBootstrapRuntime);
    pProfileBootstrapRuntime = NULL;

    xwork_profile_init(&tAutonomousProfile);
    assert(xwork_profile_get_builtin(XWORK_PROFILE_XCLAW, &tAutonomousProfile) == XWORK_OK);
    assert(strcmp(tAutonomousProfile.sProfileId, XWORK_PROFILE_XCLAW) == 0);
    assert(tAutonomousProfile.eAutonomy == XWORK_AUTONOMY_AUTO);
    assert(tAutonomousProfile.tPolicy.eAutoApproveRiskLimit == XWORK_RISK_HIGH);
    assert(strcmp(tAutonomousProfile.sDefaultLlmProfileId, XWORK_PROFILE_XCLAW) == 0);
    assert(strcmp(tAutonomousProfile.sDefaultSessionProfileId, XWORK_PROFILE_XCLAW) == 0);
    assert(tAutonomousProfile.tSessionPolicy.bEnableAutoCompact);
    assert(tAutonomousProfile.tSessionPolicy.fCompactTriggerRatio == 0.90);
    assert(tAutonomousProfile.tSessionPolicy.iCompactTriggerTurns == 24u);
    assert(tAutonomousProfile.iDefaultMaxTurns == 32u);
    assert(tAutonomousProfile.bDefaultAutoApprove);
    assert(tAutonomousProfile.bEnableWorkspaceMemory);
    xwork_runtime_options_init(&tProfileRuntimeOptions);
    assert(xwork_profile_apply_runtime_options(&tAutonomousProfile, &tProfileRuntimeOptions) == XWORK_OK);
    assert(tProfileRuntimeOptions.tPolicy.eAutoApproveRiskLimit == XWORK_RISK_HIGH);
    xwork_workspace_options_init(&tProfileWorkspaceOptions);
    assert(xwork_profile_apply_workspace_options(&tAutonomousProfile, &tProfileWorkspaceOptions) == XWORK_OK);
    assert(tProfileWorkspaceOptions.bEnableMemory);
    xwork_xllm_profile_options_init(&tProfileLlmOptions);
    xwork_xllm_bootstrap_options_init(&tProfileBootstrapOptions);
    assert(
        xwork_profile_apply_xllm_bootstrap_options(
            &tAutonomousProfile,
            &tProfileLlmOptions,
            &tProfileBootstrapOptions
        ) == XWORK_OK
    );
    assert(strcmp(tProfileLlmOptions.sProfileId, XWORK_PROFILE_XCLAW) == 0);
    assert(strcmp(tProfileLlmOptions.sDisplayName, "xclaw autonomous") == 0);
    assert(tProfileBootstrapOptions.pProfiles == &tProfileLlmOptions);
    assert(tProfileBootstrapOptions.iProfileCount == 1u);
    xwork_run_options_init(&tProfileRunOptions);
    assert(xwork_profile_apply_run_options(&tAutonomousProfile, &tProfileRunOptions) == XWORK_OK);
    assert(tProfileRunOptions.eAutonomy == XWORK_AUTONOMY_AUTO);
    assert(strcmp(tProfileRunOptions.sLlmProfileId, XWORK_PROFILE_XCLAW) == 0);
    assert(strcmp(tProfileRunOptions.sSessionProfileId, XWORK_PROFILE_XCLAW) == 0);
    assert(tProfileRunOptions.tSessionPolicy.bEnableAutoCompact);
    assert(tProfileRunOptions.tSessionPolicy.fCompactTriggerRatio == 0.90);
    assert(tProfileRunOptions.tSessionPolicy.iCompactTriggerTurns == 24u);
    xwork_orchestrator_options_init(&tProfileExecOptions);
    assert(
        xwork_profile_apply_orchestrator_options(
            &tAutonomousProfile,
            &tProfileExecOptions
        ) == XWORK_OK
    );
    assert(tProfileExecOptions.iMaxTurns == 32u);
    assert(tProfileExecOptions.bAutoApprove);
    assert(xwork_profile_get_builtin("missing-profile", &tInteractiveProfile) == XWORK_ERROR_NOT_FOUND);

    xwork_workspace_options_init(&tWorkspaceOptions);
    tWorkspaceOptions.sWorkspaceId = "main";
    tWorkspaceOptions.sRootPath = "D:/git/xwork";
    assert(xwork_runtime_add_workspace(pRuntime, &tWorkspaceOptions, &pWorkspace) == XWORK_OK);
    assert(pWorkspace != NULL);

    xwork_workspace_options_init(&tWorkspaceOptions);
    tWorkspaceOptions.sWorkspaceId = "memory-main";
    tWorkspaceOptions.sRootPath = "D:/git/xwork";
    assert(
        xwork_profile_apply_workspace_options(
            &tAutonomousProfile,
            &tWorkspaceOptions
        ) == XWORK_OK
    );
    assert(tWorkspaceOptions.bEnableMemory);
    tWorkspaceOptions.pMemory = pWorkspaceMemory;
    assert(xwork_runtime_add_workspace(pRuntime, &tWorkspaceOptions, &pMemoryWorkspace) == XWORK_OK);
    assert(pMemoryWorkspace != NULL);

    xwork_tool_def_init(&tToolDef);
    tToolDef.sToolId = "mock.apply_patch";
    tToolDef.sDisplayName = "Mock Apply Patch";
    tToolDef.sDescription = "Apply a mock workspace patch.";
    tToolDef.eKind = XWORK_TOOL_HOST_SERVICE;
    tToolDef.eSideEffect = XWORK_SIDE_EFFECT_WORKSPACE_WRITE;
    tToolDef.eApprovalMode = XWORK_APPROVAL_ALWAYS;
    assert(xwork_runtime_register_tool(pRuntime, &tToolDef) == XWORK_OK);

    pBuiltinToolDef = xwork_get_builtin_tool_def(XWORK_TOOL_FILESYSTEM_READ_TEXT);
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_FILESYSTEM);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_FILESYSTEM_READ_TEXT) == 0);
    pBuiltinToolDef = xwork_get_builtin_tool_def(XWORK_TOOL_FILESYSTEM_WRITE_TEXT);
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_FILESYSTEM);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_FILESYSTEM_WRITE_TEXT) == 0);
    assert(pBuiltinToolDef->eSideEffect == XWORK_SIDE_EFFECT_WORKSPACE_WRITE);
    pBuiltinToolDef = xwork_get_builtin_tool_def(XWORK_TOOL_PROCESS_EXEC);
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eSideEffect == XWORK_SIDE_EFFECT_PROCESS_EXEC);
    assert(
        xwork_runtime_register_builtin_tool(
            pRuntime,
            XWORK_TOOL_FILESYSTEM_READ_TEXT
        ) == XWORK_OK
    );

    asWorkspaceIds[0] = "main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-smoke";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pRun) == XWORK_OK);

    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tExecOptions.pUserData = &tToolExecCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pRun, &tExecOptions) == XWORK_OK);

    assert(xwork_run_get_state(pRun) == XWORK_RUN_COMPLETED);
    assert(strcmp(xwork_run_get_last_output_text(pRun), "Mock model completed after tool result.") == 0);
    assert(tToolExecCtx.iExecCount == 1);

    xwork_approval_request_init(&tApproval);
    assert(xwork_run_get_last_approval_request(pRun, &tApproval) == XWORK_OK);
    assert(tApproval.eState == XWORK_APPROVAL_APPROVED);
    assert(strcmp(tApproval.sToolId, "mock.apply_patch") == 0);

    xwork_checkpoint_init(&tCheckpoint);
    assert(xwork_run_get_last_checkpoint(pRun, &tCheckpoint) == XWORK_OK);
    assert(tCheckpoint.eKind == XWORK_CHECKPOINT_COMPLETION);
    assert(tCheckpoint.eRunState == XWORK_RUN_COMPLETED);
    assert(tCheckpoint.sArtifactRefs != NULL);

    xwork_event_init(&tEvent);
    assert(xwork_run_get_last_event(pRun, &tEvent) == XWORK_OK);
    assert(tEvent.eKind == XWORK_EVENT_RUN_COMPLETED);
    assert(xwork_run_get_event_count(pRun) >= 10u);
    assert(xwork_run_get_checkpoint_count(pRun) == 2u);
    assert(xwork_run_get_artifact_count(pRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_PATCH);
    assert(strcmp(tArtifact.sName, "README.patch") == 0);
    assert(strcmp(tArtifact.sMimeType, "text/x-diff") == 0);
    assert(strcmp(tArtifact.sStorageRef, "workspace://README.md") == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, "xwork smoke patch") != NULL);
    assert(strcmp(tCheckpoint.sArtifactRefs, tArtifact.sArtifactId) == 0);
    sFirstArtifactId = xwork_test_dup_cstr(tArtifact.sArtifactId);
    assert(sFirstArtifactId != NULL);
    xwork_event_init(&tEvent);
    assert(xwork_run_get_event(pRun, 0u, &tEvent) == XWORK_OK);
    assert(tEvent.eKind == XWORK_EVENT_RUN_CREATED);
    xwork_checkpoint_init(&tCheckpoint);
    assert(xwork_run_get_checkpoint(pRun, 0u, &tCheckpoint) == XWORK_OK);
    assert(tCheckpoint.eKind == XWORK_CHECKPOINT_AFTER_TOOL);
    sAfterToolCheckpointId = xwork_test_dup_cstr(tCheckpoint.sCheckpointId);
    assert(sAfterToolCheckpointId != NULL);
    xwork_checkpoint_init(&tCheckpoint);
    assert(xwork_run_get_checkpoint(pRun, 1u, &tCheckpoint) == XWORK_OK);
    assert(tCheckpoint.eKind == XWORK_CHECKPOINT_COMPLETION);

    tAdapterCtx.iTurnCount = 0;
    tAdapterCtx.sExpectedMemoryText = "workspace-memory:README.md";
    tAdapterCtx.iObservedMemoryTurns = 0;
    asWorkspaceIds[0] = "memory-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-memory-default";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pMemoryRun) == XWORK_OK);

    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tExecOptions.pUserData = &tDefaultMemoryToolExecCtx;
    tExecOptions.bIngestToolResultsToMemory = true;
    tExecOptions.bIngestArtifactsToMemory = true;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pMemoryRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pMemoryRun) == XWORK_RUN_COMPLETED);
    assert(strcmp(xwork_run_get_last_output_text(pMemoryRun), "Mock model completed after tool result.") == 0);
    assert(tDefaultMemoryToolExecCtx.iExecCount == 1);
    assert(tAdapterCtx.iObservedMemoryTurns == 2);
    xwork_memory_context_reset(&tObservedMemoryContext);
    assert(
        xwork_run_get_last_memory_context(
            pMemoryRun,
            &tObservedMemoryContext
        ) == XWORK_OK
    );
    assert(tObservedMemoryContext.sText != NULL);
    assert(strstr(tObservedMemoryContext.sText, "Workspace memory: memory-main") != NULL);
    assert(strstr(tObservedMemoryContext.sText, "workspace-memory:README.md") != NULL);
    assert(tObservedMemoryContext.iWorkspaceCount == 1u);
    {
        xllm_memory_search_options tMemorySearchOptions;
        xllm_memory_search_result tMemorySearchResult;
        xllm_error tMemorySearchError;

        xllm_memory_search_options_init(&tMemorySearchOptions);
        memset(&tMemorySearchResult, 0, sizeof(tMemorySearchResult));
        xllm_error_init(&tMemorySearchError);
        tMemorySearchOptions.sQuery = "mock.apply_patch executed successfully";
        assert(
            xllm_memory_search(
                pWorkspaceMemory,
                &tMemorySearchOptions,
                &tMemorySearchResult,
                &tMemorySearchError
            ) == XRT_NET_OK
        );
        assert(tMemorySearchResult.iHitCount >= 1u);
        assert(tMemorySearchResult.pHits[0].sText != NULL);
        assert(
            strstr(
                tMemorySearchResult.pHits[0].sText,
                "mock.apply_patch executed successfully."
            ) != NULL
        );
        xllm_memory_search_result_reset(&tMemorySearchResult);
        xllm_error_free(&tMemorySearchError);
    }
    {
        xllm_memory_search_options tMemorySearchOptions;
        xllm_memory_search_result tMemorySearchResult;
        xllm_error tMemorySearchError;
        size_t i;
        bool bFoundArtifactRecord = false;

        xllm_memory_search_options_init(&tMemorySearchOptions);
        memset(&tMemorySearchResult, 0, sizeof(tMemorySearchResult));
        xllm_error_init(&tMemorySearchError);
        tMemorySearchOptions.sQuery = "Patch artifact captured README.patch workspace://README.md";
        assert(
            xllm_memory_search(
                pWorkspaceMemory,
                &tMemorySearchOptions,
                &tMemorySearchResult,
                &tMemorySearchError
            ) == XRT_NET_OK
        );
        for ( i = 0u; i < tMemorySearchResult.iHitCount; ++i ) {
            const char *sHitText = tMemorySearchResult.pHits[i].sText;

            if ( sHitText &&
                 strstr(sHitText, "Patch artifact captured for README.md.") != NULL &&
                 strstr(sHitText, "README.patch") != NULL ) {
                bFoundArtifactRecord = true;
                break;
            }
        }
        assert(bFoundArtifactRecord);
        xllm_memory_search_result_reset(&tMemorySearchResult);
        xllm_error_free(&tMemorySearchError);
    }
    xwork_run_destroy(pMemoryRun);
    pMemoryRun = NULL;
    tAdapterCtx.sExpectedMemoryText = NULL;
    tAdapterCtx.iObservedMemoryTurns = 0;

    tMemoryCtx.iResolveCount = 0;
    tMemoryCtx.sContextText = "workspace-memory:README.md";
    tAdapterCtx.iTurnCount = 0;
    tAdapterCtx.sExpectedMemoryText = tMemoryCtx.sContextText;
    tAdapterCtx.iObservedMemoryTurns = 0;
    asWorkspaceIds[0] = "memory-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-memory-context";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pMemoryRun) == XWORK_OK);

    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tExecOptions.pUserData = &tToolExecCtx;
    tExecOptions.pfnResolveMemoryContext = xwork_mock_memory_resolve;
    tExecOptions.pMemoryUserData = &tMemoryCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pMemoryRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pMemoryRun) == XWORK_RUN_COMPLETED);
    assert(strcmp(xwork_run_get_last_output_text(pMemoryRun), "Mock model completed after tool result.") == 0);
    assert(tMemoryCtx.iResolveCount == 2);
    assert(tAdapterCtx.iObservedMemoryTurns == 2);
    xwork_event_init(&tEvent);
    assert(xwork_run_get_event(pMemoryRun, 2u, &tEvent) == XWORK_OK);
    assert(tEvent.eKind == XWORK_EVENT_MEMORY_CONTEXT_ATTACHED);
    assert(xwork_run_get_artifact_count(pMemoryRun) == 1u);
    xwork_memory_context_reset(&tObservedMemoryContext);
    assert(
        xwork_run_get_last_memory_context(
            pMemoryRun,
            &tObservedMemoryContext
        ) == XWORK_OK
    );
    assert(strcmp(tObservedMemoryContext.sText, tMemoryCtx.sContextText) == 0);
    assert(tObservedMemoryContext.iWorkspaceCount == 1u);
    assert(xwork_run_get_snapshot(pMemoryRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.bHasMemoryContext);
    assert(strcmp(tRunSnapshot.sLastMemoryContextText, tMemoryCtx.sContextText) == 0);
    assert(tRunSnapshot.iLastMemoryWorkspaceCount == 1u);
    assert(tRunSnapshot.sSessionStateData != NULL);
    assert(strstr(tRunSnapshot.sSessionStateData, "xllm_session_state") != NULL);
    assert(!tRunSnapshot.tSessionPolicy.bEnableAutoCompact);
    assert(tRunSnapshot.tSessionPolicy.fCompactTriggerRatio == 0.60);
    assert(tRunSnapshot.tSessionPolicy.iCompactTriggerTurns == 5u);
    xwork_run_destroy(pMemoryRun);
    pMemoryRun = NULL;
    assert(xwork_runtime_recover_run(pRuntime, &tRunSnapshot, &pRecoveredMemoryRun) == XWORK_OK);
    xwork_memory_context_reset(&tObservedMemoryContext);
    assert(
        xwork_run_get_last_memory_context(
            pRecoveredMemoryRun,
            &tObservedMemoryContext
        ) == XWORK_OK
    );
    assert(strcmp(tObservedMemoryContext.sText, tMemoryCtx.sContextText) == 0);
    assert(tObservedMemoryContext.iWorkspaceCount == 1u);
    xwork_run_destroy(pRecoveredMemoryRun);
    pRecoveredMemoryRun = NULL;
    xwork_run_snapshot_reset(&tRunSnapshot);
    tAdapterCtx.sExpectedMemoryText = NULL;
    tAdapterCtx.iObservedMemoryTurns = 0;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-host-tool";
    tRunOptions.sInstruction = "Read the README through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pHostRun) == XWORK_OK);

    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pHostRun) == XWORK_RUN_COMPLETED);
    assert(strcmp(xwork_run_get_last_output_text(pHostRun), "Host service tool completed.") == 0);
    assert(tToolExecCtx.iExecCount == 2);
    assert(tHostCtx.iInvokeCount == 2);
    assert(xwork_run_get_event_count(pHostRun) >= 8u);
    assert(xwork_run_get_checkpoint_count(pHostRun) == 2u);
    assert(xwork_run_get_artifact_count(pHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
    assert(strcmp(tArtifact.sName, "README.md") == 0);
    assert(strcmp(tArtifact.sStorageRef, "README.md") == 0);
    assert(strcmp(tArtifact.sContentText, "README preview") == 0);
    xwork_approval_request_init(&tApproval);
    assert(xwork_run_get_last_approval_request(pHostRun, &tApproval) == XWORK_ERROR_NOT_FOUND);

    tLocalHostOptions.sDefaultWorkingDirectory = ".";
    xwork_host_services_init(&tLocalHostServices);
    assert(
        xwork_local_host_configure_services(
            &tLocalHost,
            &tLocalHostOptions,
            &tLocalHostServices
        ) == XWORK_OK
    );
    xwork_runtime_options_init(&tLocalHostRuntimeOptions);
    tLocalHostRuntimeOptions.pLlmRuntime = pLlmRuntime;
    tLocalHostRuntimeOptions.pHostServices = &tLocalHostServices;
    assert(
        xwork_runtime_create(
            &tLocalHostRuntimeOptions,
            &pLocalHostRuntime
        ) == XWORK_OK
    );
    xwork_workspace_options_init(&tWorkspaceOptions);
    tWorkspaceOptions.sWorkspaceId = "local-host-main";
    tWorkspaceOptions.sRootPath = ".";
    assert(
        xwork_runtime_add_workspace(
            pLocalHostRuntime,
            &tWorkspaceOptions,
            &pLocalHostWorkspace
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_register_builtin_tool(
            pLocalHostRuntime,
            XWORK_TOOL_FILESYSTEM_READ_TEXT
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_register_builtin_tool(
            pLocalHostRuntime,
            XWORK_TOOL_FILESYSTEM_WRITE_TEXT
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_register_builtin_tool(
            pLocalHostRuntime,
            XWORK_TOOL_PROCESS_EXEC
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_register_builtin_tool(
            pLocalHostRuntime,
            XWORK_TOOL_VCS_STATUS
        ) == XWORK_OK
    );
    pBuiltinToolDef = xwork_runtime_find_tool(
        pLocalHostRuntime,
        XWORK_TOOL_PROCESS_EXEC
    );
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_PROCESS);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_PROCESS_EXEC) == 0);

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"README.md\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.read_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":true") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"text\":\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "xwork") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_missing_smoke.txt\"}",
            &tLocalHostResult
        ) == XWORK_ERROR_NOT_FOUND
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.read_text not found") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"not_found\"") != NULL);
    assert(
        strstr(
            tLocalHostResult.sOutputText,
            "\"error\":\"path does not exist\""
        ) != NULL
    );

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_WRITE_TEXT,
            "{\"path\":\"tests/local_host_write_smoke.txt\","
            "\"text\":\"local host write smoke\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.write_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"mode\":\"overwrite\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"bytes_written\":22") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_write_smoke.txt\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.read_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "local host write smoke") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"file_size_bytes\":22") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"next_offset_bytes\":22") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"remaining_bytes\":0") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"eof\":true") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_write_smoke.txt\","
            "\"offset_bytes\":6,\"max_bytes\":4}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.read_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"offset_bytes\":6") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"file_size_bytes\":22") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"bytes_read\":4") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"next_offset_bytes\":10") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"remaining_bytes\":12") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"text\":\"host\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"truncated\":true") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"eof\":false") != NULL);

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_WRITE_TEXT,
            "{\"path\":\"tests/local_host_append_smoke.txt\","
            "\"text\":\"alpha\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.write_text ok") == 0);
    assert(strstr(tLocalHostResult.sOutputText, "\"mode\":\"overwrite\"") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_WRITE_TEXT,
            "{\"path\":\"tests/local_host_append_smoke.txt\","
            "\"text\":\"-beta\",\"mode\":\"append\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.write_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"mode\":\"append\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"bytes_written\":5") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_append_smoke.txt\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.read_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "alpha-beta") != NULL);

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_WRITE_TEXT,
            "{\"path\":\"tests/local_host_create_smoke.txt\","
            "\"text\":\"local host create smoke\",\"mode\":\"create\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.write_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"mode\":\"create\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"bytes_written\":23") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_create_smoke.txt\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.read_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "local host create smoke") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_WRITE_TEXT,
            "{\"path\":\"tests/local_host_create_smoke.txt\","
            "\"text\":\"local host create smoke\",\"mode\":\"create\"}",
            &tLocalHostResult
        ) == XWORK_ERROR_EXTERNAL_FAILURE
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.write_text failed (target exists)") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"mode\":\"create\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"already_exists\"") != NULL);
    assert(
        strstr(
            tLocalHostResult.sOutputText,
            "\"error\":\"path already exists\""
        ) != NULL
    );

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_WRITE_TEXT,
            "{\"path\":\"tests/local_host_nested/create_dirs/"
            "local_host_create_dirs_smoke.txt\","
            "\"text\":\"xwork-create-dirs-note\"}",
            &tLocalHostResult
        ) == XWORK_ERROR_EXTERNAL_FAILURE
    );
    assert(
        strcmp(
            tLocalHostResult.sVisibleSummary,
            "filesystem.write_text failed (parent directory not found)"
        ) == 0
    );
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"parent_not_found\"") != NULL);
    assert(
        strstr(
            tLocalHostResult.sOutputText,
            "\"error\":\"parent directory does not exist\""
        ) != NULL
    );
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_WRITE_TEXT,
            "{\"path\":\"tests/local_host_nested/create_dirs/"
            "local_host_create_dirs_smoke.txt\","
            "\"text\":\"xwork-create-dirs-note\",\"create_dirs\":true}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.write_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"create_dirs\":true") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"bytes_written\":22") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_nested/create_dirs/"
            "local_host_create_dirs_smoke.txt\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "filesystem.read_text ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "xwork-create-dirs-note") != NULL);

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"echo xwork-local-process\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stdout\":\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "xwork-local-process") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"cwd\":\".\"") != NULL);

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"cd\",\"cwd\":\"tests\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"cwd\":\"./tests\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stdout\":\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "tests") != NULL);

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"echo xwork-local-process-truncate\","
            "\"max_output_bytes\":12}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stdout\":\"xwork-local-\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"truncated\":true") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"exit_code\":0") != NULL);

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_ENV_COMMAND "\","
            "\"env\":[\"XWORK_TEST_PROCESS_ENV=xwork-process-env\"]}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_ENV_EXPECTED) != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"env_count\":1") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"truncated\":false") != NULL);
    iSavedMaxProcessEnvEntries = tLocalHost.iMaxProcessEnvEntries;
    tLocalHost.iMaxProcessEnvEntries = 1u;
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_ENV_COMMAND "\","
            "\"env\":[\"XWORK_TEST_PROCESS_ENV=xwork-process-env\","
            "\"XWORK_TEST_PROCESS_ENV_EXTRA=xwork-process-env-extra\"]}",
            &tLocalHostResult
        ) == XWORK_ERROR_INVALID_ARGUMENT
    );
    assert(
        strcmp(
            tLocalHostResult.sVisibleSummary,
            "process.exec invalid request (env limit exceeded)"
        ) == 0
    );
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"env_count\":2") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"invalid_request\"") != NULL);
    assert(
        strstr(
            tLocalHostResult.sOutputText,
            "\"error\":\"env exceeds max_process_env_entries\""
        ) != NULL
    );
    tLocalHost.iMaxProcessEnvEntries = iSavedMaxProcessEnvEntries;

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_STDIN_COMMAND "\","
            "\"stdin_text\":\"" XWORK_TEST_PROCESS_STDIN_INPUT "\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_STDIN_EXPECTED) != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stdin_bytes\":20") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"truncated\":false") != NULL);
    iSavedMaxProcessInputBytes = tLocalHost.iMaxProcessInputBytes;
    tLocalHost.iMaxProcessInputBytes = 8u;
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_STDIN_COMMAND "\","
            "\"stdin_text\":\"" XWORK_TEST_PROCESS_STDIN_INPUT "\"}",
            &tLocalHostResult
        ) == XWORK_ERROR_INVALID_ARGUMENT
    );
    assert(
        strcmp(
            tLocalHostResult.sVisibleSummary,
            "process.exec invalid request (stdin_text exceeds limit)"
        ) == 0
    );
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"invalid_request\"") != NULL);
    assert(
        strstr(
            tLocalHostResult.sOutputText,
            "\"error\":\"stdin_text exceeds max_process_input_bytes\""
        ) != NULL
    );
    tLocalHost.iMaxProcessInputBytes = iSavedMaxProcessInputBytes;
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_NONZERO_COMMAND "\"}",
            &tLocalHostResult
        ) == XWORK_ERROR_EXTERNAL_FAILURE
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec failed (exit 3)") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_NONZERO_EXPECTED) != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"nonzero_exit\"") != NULL);
    assert(
        strstr(
            tLocalHostResult.sOutputText,
            "\"error\":\"process exited with code 3\""
        ) != NULL
    );
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_NONZERO_COMMAND "\","
            "\"allow_nonzero_exit\":true}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_NONZERO_EXPECTED) != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"exit_code\":3") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"allow_nonzero_exit\":true") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"truncated\":false") != NULL);

    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_VCS,
            XWORK_HOST_VCS_STATUS,
            "{\"path\":\".\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "vcs.status ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"status\":\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":true") != NULL);

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-host-tool";
    tRunOptions.sInstruction = "Read the README through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );

    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service tool completed."
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"text\":\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "xwork") != NULL);
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_command_artifact_options_init(&tCommandArtifactOptions);
    tCommandArtifactOptions.sName = "local-process-command";
    tCommandArtifactOptions.sStorageRef = "host://process/echo";
    tCommandArtifactOptions.sSummary = "Local host command artifact captured.";
    tCommandArtifactOptions.sCommandText = "echo xwork-local-process";
    tCommandArtifactOptions.sOutputText = "xwork-local-process\n";
    tCommandArtifactOptions.bHasExitCode = true;
    tCommandArtifactOptions.iExitCode = 0;
    xwork_artifact_reset(&tArtifact);
    assert(
        xwork_run_emit_command_artifact(
            pLocalHostRun,
            &tCommandArtifactOptions,
            &tArtifact
        ) == XWORK_OK
    );
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sCommandText, "echo xwork-local-process") == 0);
    assert(strcmp(tArtifact.sContentText, "xwork-local-process\n") == 0);
    assert(tArtifact.bHasExitCode);
    assert(tArtifact.iExitCode == 0);
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 2u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
    assert(strcmp(tArtifact.sName, "README.md") == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, "xwork") != NULL);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-host-offset-tool";
    tRunOptions.sInstruction = "Read part of a local note through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );

    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service tool completed."
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"offset_bytes\":6") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"file_size_bytes\":22") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"bytes_read\":4") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"next_offset_bytes\":10") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"remaining_bytes\":12") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"text\":\"host\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"eof\":false") != NULL);
    xwork_run_snapshot_reset(&tRunSnapshot);
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
    assert(strcmp(tArtifact.sName, "local_host_write_smoke.txt") == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strcmp(tArtifact.sContentText, "host") == 0);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-read-missing-tool";
    tRunOptions.sInstruction = "Read a missing local note through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_ERROR_NOT_FOUND);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_FAILED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "filesystem.read_text not found"
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"ok\":false") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"error_kind\":\"not_found\"") != NULL);
    assert(
        strstr(
            tRunSnapshot.sLastToolResultText,
            "\"error\":\"path does not exist\""
        ) != NULL
    );
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_WRITE_TEXT,
            "{\"path\":\"tests/local_host_orchestrator_append_smoke.txt\","
            "\"text\":\"seed-\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-append-tool";
    tRunOptions.sInstruction = "Append a note through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service append completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
    assert(strcmp(tArtifact.sName, "local_host_orchestrator_append_smoke.txt") == 0);
    assert(tArtifact.sStorageRef != NULL);
    assert(strstr(tArtifact.sStorageRef, sLocalHostOrchestratorAppendPath) != NULL);
    assert(strcmp(tArtifact.sContentText, "xwork-append-note") == 0);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_orchestrator_append_smoke.txt\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strstr(tLocalHostResult.sOutputText, "seed-xwork-append-note") != NULL);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-create-tool";
    tRunOptions.sInstruction = "Create a fresh note through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service create completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
    assert(strcmp(tArtifact.sName, "local_host_orchestrator_create_smoke.txt") == 0);
    assert(tArtifact.sStorageRef != NULL);
    assert(strstr(tArtifact.sStorageRef, sLocalHostOrchestratorCreatePath) != NULL);
    assert(strcmp(tArtifact.sContentText, "xwork-create-note") == 0);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_orchestrator_create_smoke.txt\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strstr(tLocalHostResult.sOutputText, "xwork-create-note") != NULL);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-create-existing-tool";
    tRunOptions.sInstruction = "Create a fresh note through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_ERROR_EXTERNAL_FAILURE);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_FAILED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "filesystem.write_text failed (target exists)"
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"ok\":false") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"mode\":\"create\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"error_kind\":\"already_exists\"") != NULL);
    assert(
        strstr(
            tRunSnapshot.sLastToolResultText,
            "\"error\":\"path already exists\""
        ) != NULL
    );
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-write-create-dirs-tool";
    tRunOptions.sInstruction = "Write a nested note through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service nested write completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
    assert(strcmp(tArtifact.sName, "local_host_orchestrator_create_dirs_smoke.txt") == 0);
    assert(tArtifact.sStorageRef != NULL);
    assert(strstr(tArtifact.sStorageRef, sLocalHostOrchestratorCreateDirsPath) != NULL);
    assert(strcmp(tArtifact.sContentText, "xwork-create-dirs-note") == 0);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_nested/orchestrator/"
            "local_host_orchestrator_create_dirs_smoke.txt\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strstr(tLocalHostResult.sOutputText, "xwork-create-dirs-note") != NULL);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-write-tool";
    tRunOptions.sInstruction = "Write a note through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service write completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
    assert(strcmp(tArtifact.sName, "local_host_orchestrator_write_smoke.txt") == 0);
    assert(tArtifact.sStorageRef != NULL);
    assert(strstr(tArtifact.sStorageRef, sLocalHostOrchestratorWritePath) != NULL);
    assert(strcmp(tArtifact.sContentText, "xwork-write-note") == 0);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_FILESYSTEM,
            XWORK_HOST_FILESYSTEM_READ_TEXT,
            "{\"path\":\"tests/local_host_orchestrator_write_smoke.txt\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strstr(tLocalHostResult.sOutputText, "xwork-write-note") != NULL);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-tool";
    tRunOptions.sInstruction = "Run the local process through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service process completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sName, "process.exec.txt") == 0);
    assert(strcmp(tArtifact.sCommandText, "echo xwork-local-process") == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, "xwork-local-process") != NULL);
    assert(tArtifact.bHasExitCode);
    assert(tArtifact.iExitCode == 0);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-stdin-tool";
    tRunOptions.sInstruction = "Run the local process with stdin through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service process stdin completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sName, "process.exec.txt") == 0);
    assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_STDIN_COMMAND) == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, XWORK_TEST_PROCESS_STDIN_EXPECTED) != NULL);
    assert(tArtifact.bHasExitCode);
    assert(tArtifact.iExitCode == 0);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-nonzero-tool";
    tRunOptions.sInstruction = "Run the local process with non-zero exit through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service process non-zero completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sName, "process.exec.txt") == 0);
    assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_NONZERO_COMMAND_RAW) == 0);
    assert(tArtifact.sStorageRef != NULL);
    assert(strcmp(tArtifact.sStorageRef, ".") == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, XWORK_TEST_PROCESS_NONZERO_EXPECTED) != NULL);
    assert(tArtifact.bHasExitCode);
    assert(tArtifact.iExitCode == XWORK_TEST_PROCESS_NONZERO_EXIT_CODE);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    iSavedMaxProcessInputBytes = tLocalHost.iMaxProcessInputBytes;
    tLocalHost.iMaxProcessInputBytes = 8u;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-stdin-too-large-tool";
    tRunOptions.sInstruction = "Run the local process with stdin through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_ERROR_INVALID_ARGUMENT);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_FAILED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "process.exec invalid request (stdin_text exceeds limit)"
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"ok\":false") != NULL);
    assert(
        strstr(
            tRunSnapshot.sLastToolResultText,
            "\"error_kind\":\"invalid_request\""
        ) != NULL
    );
    assert(
        strstr(
            tRunSnapshot.sLastToolResultText,
            "\"error\":\"stdin_text exceeds max_process_input_bytes\""
        ) != NULL
    );
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;
    tLocalHost.iMaxProcessInputBytes = iSavedMaxProcessInputBytes;

    tAdapterCtx.iTurnCount = 0;
    iSavedMaxProcessEnvEntries = tLocalHost.iMaxProcessEnvEntries;
    tLocalHost.iMaxProcessEnvEntries = 1u;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-env-too-many-tool";
    tRunOptions.sInstruction = "Run the local process with too many env values through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_ERROR_INVALID_ARGUMENT);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_FAILED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "process.exec invalid request (env limit exceeded)"
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"ok\":false") != NULL);
    assert(
        strstr(
            tRunSnapshot.sLastToolResultText,
            "\"env_count\":2"
        ) != NULL
    );
    assert(
        strstr(
            tRunSnapshot.sLastToolResultText,
            "\"error\":\"env exceeds max_process_env_entries\""
        ) != NULL
    );
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;
    tLocalHost.iMaxProcessEnvEntries = iSavedMaxProcessEnvEntries;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-cwd-tool";
    tRunOptions.sInstruction = "Run the local process in tests through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service process cwd completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sName, "process.exec.txt") == 0);
    assert(strcmp(tArtifact.sCommandText, "cd") == 0);
    assert(tArtifact.sStorageRef != NULL);
    assert(strcmp(tArtifact.sStorageRef, "./tests") == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, "tests") != NULL);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-truncated-tool";
    tRunOptions.sInstruction = "Run the local process with output cap through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service process truncated completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sName, "process.exec.txt") == 0);
    assert(strcmp(tArtifact.sCommandText, "echo xwork-local-process-truncate") == 0);
    assert(tArtifact.sStorageRef != NULL);
    assert(strcmp(tArtifact.sStorageRef, ".") == 0);
    assert(strcmp(tArtifact.sContentText, "xwork-local-") == 0);
    assert(tArtifact.bHasExitCode);
    assert(tArtifact.iExitCode == 0);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-env-tool";
    tRunOptions.sInstruction = "Run the local process with env through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service process env completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sName, "process.exec.txt") == 0);
    assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_ENV_COMMAND) == 0);
    assert(tArtifact.sStorageRef != NULL);
    assert(strcmp(tArtifact.sStorageRef, ".") == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, XWORK_TEST_PROCESS_ENV_EXPECTED) != NULL);
    assert(tArtifact.bHasExitCode);
    assert(tArtifact.iExitCode == 0);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-vcs-tool";
    tRunOptions.sInstruction = "Read git status through host service.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pLocalHostRuntime,
            &tRunOptions,
            &pLocalHostRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pLocalHostRun),
            "Host service vcs completed."
        ) == 0
    );
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sName, "git-status.txt") == 0);
    assert(strcmp(tArtifact.sCommandText, "git status --short --branch") == 0);
    assert(tArtifact.sContentText != NULL);
    assert(tArtifact.sContentText[0] != '\0');
    assert(tArtifact.sStorageRef != NULL);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;
    xwork_runtime_destroy(pLocalHostRuntime);
    pLocalHostRuntime = NULL;
    xwork_local_host_reset(&tLocalHost);
    asWorkspaceIds[0] = "main";

    assert(xwork_run_load_checkpoint(pRun, sAfterToolCheckpointId) == XWORK_OK);
    assert(xwork_run_get_state(pRun) == XWORK_RUN_RUNNING);
    assert(xwork_run_get_last_output_text(pRun) == NULL);
    xwork_checkpoint_init(&tCheckpoint);
    assert(xwork_run_get_last_checkpoint(pRun, &tCheckpoint) == XWORK_OK);
    assert(tCheckpoint.eKind == XWORK_CHECKPOINT_AFTER_TOOL);
    assert(tCheckpoint.sArtifactRefs != NULL);
    assert(strcmp(tCheckpoint.sArtifactRefs, sFirstArtifactId) == 0);
    xwork_event_init(&tEvent);
    assert(xwork_run_get_last_event(pRun, &tEvent) == XWORK_OK);
    assert(tEvent.eKind == XWORK_EVENT_CHECKPOINT_LOADED);

    tAdapterCtx.iTurnCount = 1;
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tExecOptions.pUserData = &tToolExecCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pRun) == XWORK_RUN_COMPLETED);
    assert(strcmp(xwork_run_get_last_output_text(pRun), "Mock model completed after tool result.") == 0);
    assert(tToolExecCtx.iExecCount == 2);
    assert(xwork_run_get_checkpoint_count(pRun) == 3u);

    tAdapterCtx.iTurnCount = 0;
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-wait-approval";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pPendingRun) == XWORK_OK);

    xwork_orchestrator_options_init(&tPendingExecOptions);
    tPendingExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tPendingExecOptions.pUserData = &tToolExecCtx;
    tPendingExecOptions.iMaxTurns = 3u;
    tPendingExecOptions.bAutoApprove = false;
    assert(xwork_run_execute(pPendingRun, &tPendingExecOptions) == XWORK_OK);

    assert(xwork_run_get_state(pPendingRun) == XWORK_RUN_WAITING_APPROVAL);
    assert(tToolExecCtx.iExecCount == 2);

    xwork_approval_request_init(&tPendingApproval);
    assert(xwork_run_get_last_approval_request(pPendingRun, &tPendingApproval) == XWORK_OK);
    assert(tPendingApproval.eState == XWORK_APPROVAL_PENDING);
    assert(strcmp(tPendingApproval.sToolId, "mock.apply_patch") == 0);

    xwork_checkpoint_init(&tPendingCheckpoint);
    assert(xwork_run_get_last_checkpoint(pPendingRun, &tPendingCheckpoint) == XWORK_OK);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    assert(tPendingCheckpoint.eRunState == XWORK_RUN_WAITING_APPROVAL);
    sPendingBeforeToolCheckpointId = xwork_test_dup_cstr(tPendingCheckpoint.sCheckpointId);
    assert(sPendingBeforeToolCheckpointId != NULL);

    xwork_event_init(&tPendingEvent);
    assert(xwork_run_get_last_event(pPendingRun, &tPendingEvent) == XWORK_OK);
    assert(tPendingEvent.eKind == XWORK_EVENT_CHECKPOINT_SAVED);
    assert(xwork_run_get_event_count(pPendingRun) >= 6u);
    assert(xwork_run_get_checkpoint_count(pPendingRun) == 1u);

    assert(xwork_run_resume(pPendingRun) == XWORK_ERROR_INVALID_STATE);
    assert(xwork_run_submit_approval(pPendingRun, XWORK_APPROVAL_APPROVED) == XWORK_OK);
    assert(xwork_run_get_state(pPendingRun) == XWORK_RUN_PAUSED);

    xwork_approval_request_init(&tPendingApproval);
    assert(xwork_run_get_last_approval_request(pPendingRun, &tPendingApproval) == XWORK_OK);
    assert(tPendingApproval.eState == XWORK_APPROVAL_APPROVED);

    xwork_event_init(&tPendingEvent);
    assert(xwork_run_get_last_event(pPendingRun, &tPendingEvent) == XWORK_OK);
    assert(tPendingEvent.eKind == XWORK_EVENT_APPROVAL_RESOLVED);

    assert(xwork_run_resume(pPendingRun) == XWORK_OK);
    assert(xwork_run_get_state(pPendingRun) == XWORK_RUN_RUNNING);

    xwork_event_init(&tPendingEvent);
    assert(xwork_run_get_last_event(pPendingRun, &tPendingEvent) == XWORK_OK);
    assert(tPendingEvent.eKind == XWORK_EVENT_CHECKPOINT_LOADED);

    assert(xwork_run_execute(pPendingRun, &tPendingExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pPendingRun) == XWORK_RUN_COMPLETED);
    assert(strcmp(xwork_run_get_last_output_text(pPendingRun), "Mock model completed after tool result.") == 0);
    assert(tToolExecCtx.iExecCount == 3);

    xwork_checkpoint_init(&tPendingCheckpoint);
    assert(xwork_run_get_last_checkpoint(pPendingRun, &tPendingCheckpoint) == XWORK_OK);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_COMPLETION);
    assert(tPendingCheckpoint.eRunState == XWORK_RUN_COMPLETED);

    xwork_event_init(&tPendingEvent);
    assert(xwork_run_get_last_event(pPendingRun, &tPendingEvent) == XWORK_OK);
    assert(tPendingEvent.eKind == XWORK_EVENT_RUN_COMPLETED);
    assert(xwork_run_get_event_count(pPendingRun) >= 13u);
    assert(xwork_run_get_checkpoint_count(pPendingRun) == 3u);
    xwork_checkpoint_init(&tPendingCheckpoint);
    assert(xwork_run_get_checkpoint(pPendingRun, 1u, &tPendingCheckpoint) == XWORK_OK);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_AFTER_TOOL);
    xwork_checkpoint_init(&tPendingCheckpoint);
    assert(xwork_run_get_checkpoint(pPendingRun, 2u, &tPendingCheckpoint) == XWORK_OK);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_COMPLETION);

    assert(xwork_run_load_checkpoint(pPendingRun, sPendingBeforeToolCheckpointId) == XWORK_OK);
    assert(xwork_run_get_state(pPendingRun) == XWORK_RUN_WAITING_APPROVAL);
    assert(xwork_run_get_last_output_text(pPendingRun) == NULL);
    xwork_approval_request_init(&tPendingApproval);
    assert(xwork_run_get_last_approval_request(pPendingRun, &tPendingApproval) == XWORK_OK);
    assert(tPendingApproval.eState == XWORK_APPROVAL_PENDING);
    xwork_checkpoint_init(&tPendingCheckpoint);
    assert(xwork_run_get_last_checkpoint(pPendingRun, &tPendingCheckpoint) == XWORK_OK);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    xwork_event_init(&tPendingEvent);
    assert(xwork_run_get_last_event(pPendingRun, &tPendingEvent) == XWORK_OK);
    assert(tPendingEvent.eKind == XWORK_EVENT_CHECKPOINT_LOADED);
    assert(xwork_run_resume(pPendingRun) == XWORK_ERROR_INVALID_STATE);

    tAdapterCtx.iTurnCount = 0;
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-recover-snapshot";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pRecoveredRun) == XWORK_OK);

    xwork_orchestrator_options_init(&tPendingExecOptions);
    tPendingExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tPendingExecOptions.pUserData = &tToolExecCtx;
    tPendingExecOptions.iMaxTurns = 3u;
    tPendingExecOptions.bAutoApprove = false;
    assert(xwork_run_execute(pRecoveredRun, &tPendingExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pRecoveredRun) == XWORK_RUN_WAITING_APPROVAL);
    assert(xwork_run_get_snapshot(pRecoveredRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sSessionStateData != NULL);
    assert(strstr(tRunSnapshot.sSessionStateData, "xllm_session_state") != NULL);
    assert(!tRunSnapshot.tSessionPolicy.bEnableAutoCompact);
    assert(tRunSnapshot.tSessionPolicy.fCompactTriggerRatio == 0.60);
    assert(tRunSnapshot.tSessionPolicy.iCompactTriggerTurns == 5u);
    xwork_run_destroy(pRecoveredRun);
    pRecoveredRun = NULL;

    assert(xwork_runtime_recover_run(pRuntime, &tRunSnapshot, &pRecoveredRun) == XWORK_OK);
    assert(xwork_run_get_state(pRecoveredRun) == XWORK_RUN_WAITING_APPROVAL);
    xwork_approval_request_init(&tPendingApproval);
    assert(xwork_run_get_last_approval_request(pRecoveredRun, &tPendingApproval) == XWORK_OK);
    assert(tPendingApproval.eState == XWORK_APPROVAL_PENDING);
    xwork_checkpoint_init(&tPendingCheckpoint);
    assert(xwork_run_get_last_checkpoint(pRecoveredRun, &tPendingCheckpoint) == XWORK_OK);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    xwork_event_init(&tPendingEvent);
    assert(xwork_run_get_last_event(pRecoveredRun, &tPendingEvent) == XWORK_OK);
    assert(tPendingEvent.eKind == XWORK_EVENT_CHECKPOINT_LOADED);

    assert(xwork_run_submit_approval(pRecoveredRun, XWORK_APPROVAL_APPROVED) == XWORK_OK);
    assert(xwork_run_resume(pRecoveredRun) == XWORK_OK);
    tAdapterCtx.iTurnCount = 1;
    assert(xwork_run_execute(pRecoveredRun, &tPendingExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pRecoveredRun) == XWORK_RUN_COMPLETED);
    assert(strcmp(xwork_run_get_last_output_text(pRecoveredRun), "Mock model completed after tool result.") == 0);
    assert(tToolExecCtx.iExecCount == 4);
    assert(xwork_run_get_artifact_count(pRecoveredRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pRecoveredRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_PATCH);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, "xwork smoke patch") != NULL);

    tPersistenceCtx.iStoreEventCount = 0;
    tPersistenceCtx.iStoreCheckpointCount = 0;
    tPersistenceCtx.iStoreArtifactCount = 0;
    tPersistenceCtx.iLoadSnapshotCount = 0;
    tPersistenceCtx.bHasSnapshot = false;
    xwork_run_snapshot_reset(&tPersistenceCtx.tSnapshot);

    tAdapterCtx.iTurnCount = 0;
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-recover-persistence";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pPersistenceRun) == XWORK_OK);

    xwork_orchestrator_options_init(&tPendingExecOptions);
    tPendingExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tPendingExecOptions.pUserData = &tToolExecCtx;
    tPendingExecOptions.iMaxTurns = 3u;
    tPendingExecOptions.bAutoApprove = false;
    assert(xwork_run_execute(pPersistenceRun, &tPendingExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pPersistenceRun) == XWORK_RUN_WAITING_APPROVAL);
    assert(tPersistenceCtx.iStoreEventCount >= 1);
    assert(tPersistenceCtx.iStoreCheckpointCount >= 1);
    assert(tPersistenceCtx.bHasSnapshot);
    assert(strcmp(tPersistenceCtx.tSnapshot.sRunId, "run-recover-persistence") == 0);
    assert(tPersistenceCtx.tSnapshot.eState == XWORK_RUN_WAITING_APPROVAL);
    assert(tPersistenceCtx.tSnapshot.eLastCheckpointKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    assert(tPersistenceCtx.tSnapshot.sSessionStateData != NULL);
    assert(strstr(tPersistenceCtx.tSnapshot.sSessionStateData, "xllm_session_state") != NULL);
    assert(!tPersistenceCtx.tSnapshot.tSessionPolicy.bEnableAutoCompact);
    assert(tPersistenceCtx.tSnapshot.tSessionPolicy.fCompactTriggerRatio == 0.60);
    assert(tPersistenceCtx.tSnapshot.tSessionPolicy.iCompactTriggerTurns == 5u);

    xwork_run_destroy(pPersistenceRun);
    pPersistenceRun = NULL;

    assert(
        xwork_runtime_recover_run_from_persistence(
            pRuntime,
            "run-recover-persistence",
            &pPersistenceRecoveredRun
        ) == XWORK_OK
    );
    assert(tPersistenceCtx.iLoadSnapshotCount == 1);
    assert(xwork_run_get_state(pPersistenceRecoveredRun) == XWORK_RUN_WAITING_APPROVAL);
    assert(xwork_run_get_snapshot(pPersistenceRecoveredRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sSessionStateData != NULL);
    assert(strstr(tRunSnapshot.sSessionStateData, "xllm_session_state") != NULL);
    assert(!tRunSnapshot.tSessionPolicy.bEnableAutoCompact);
    assert(tRunSnapshot.tSessionPolicy.fCompactTriggerRatio == 0.60);
    assert(tRunSnapshot.tSessionPolicy.iCompactTriggerTurns == 5u);
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_approval_request_init(&tPendingApproval);
    assert(xwork_run_get_last_approval_request(pPersistenceRecoveredRun, &tPendingApproval) == XWORK_OK);
    assert(tPendingApproval.eState == XWORK_APPROVAL_PENDING);
    xwork_checkpoint_init(&tPendingCheckpoint);
    assert(xwork_run_get_last_checkpoint(pPersistenceRecoveredRun, &tPendingCheckpoint) == XWORK_OK);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    xwork_event_init(&tPendingEvent);
    assert(xwork_run_get_last_event(pPersistenceRecoveredRun, &tPendingEvent) == XWORK_OK);
    assert(tPendingEvent.eKind == XWORK_EVENT_CHECKPOINT_LOADED);

    assert(xwork_run_submit_approval(pPersistenceRecoveredRun, XWORK_APPROVAL_APPROVED) == XWORK_OK);
    assert(xwork_run_resume(pPersistenceRecoveredRun) == XWORK_OK);
    tAdapterCtx.iTurnCount = 1;
    assert(xwork_run_execute(pPersistenceRecoveredRun, &tPendingExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pPersistenceRecoveredRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pPersistenceRecoveredRun),
            "Mock model completed after tool result."
        ) == 0
    );
    assert(tToolExecCtx.iExecCount == 5);
    assert(tPersistenceCtx.iStoreArtifactCount >= 1);
    assert(xwork_run_get_artifact_count(pPersistenceRecoveredRun) == 1u);

    assert(
        snprintf(
            sFilePersistenceRoot,
            sizeof(sFilePersistenceRoot),
            "D:/git/xwork/tests/persistence_store_%llu_%lu",
            (unsigned long long)time(NULL),
            (unsigned long)clock()
        ) > 0
    );
    xwork_file_persistence_options_init(&tFilePersistenceOptions);
    tFilePersistenceOptions.sRootPath = sFilePersistenceRoot;
    assert(
        xwork_file_persistence_configure_backend(
            &tFilePersistenceStore,
            &tFilePersistenceOptions,
            &tFilePersistenceBackend
        ) == XWORK_OK
    );

    xwork_runtime_options_init(&tFileRuntimeOptions);
    tFileRuntimeOptions.pLlmRuntime = pLlmRuntime;
    tFileRuntimeOptions.pHostServices = &tHostServices;
    tFileRuntimeOptions.pPersistenceBackend = &tFilePersistenceBackend;
    tFileRuntimeOptions.tPolicy = tPolicyOptions;
    assert(xwork_runtime_create(&tFileRuntimeOptions, &pFileRuntime) == XWORK_OK);

    xwork_workspace_options_init(&tWorkspaceOptions);
    tWorkspaceOptions.sWorkspaceId = "memory-main";
    tWorkspaceOptions.sRootPath = "D:/git/xwork";
    assert(
        xwork_profile_apply_workspace_options(
            &tAutonomousProfile,
            &tWorkspaceOptions
        ) == XWORK_OK
    );
    assert(tWorkspaceOptions.bEnableMemory);
    tWorkspaceOptions.pMemory = pFileWorkspaceMemory;
    assert(xwork_runtime_add_workspace(pFileRuntime, &tWorkspaceOptions, &pFileWorkspace) == XWORK_OK);
    assert(xwork_runtime_register_tool(pFileRuntime, &tToolDef) == XWORK_OK);
    assert(
        xwork_runtime_register_builtin_tool(
            pFileRuntime,
            XWORK_TOOL_FILESYSTEM_READ_TEXT
        ) == XWORK_OK
    );

    tMemoryCtx.iResolveCount = 0;
    tMemoryCtx.sContextText = "workspace-memory:file-persistence";
    tAdapterCtx.iTurnCount = 0;
    tAdapterCtx.sExpectedMemoryText = tMemoryCtx.sContextText;
    tAdapterCtx.iObservedMemoryTurns = 0;
    asWorkspaceIds[0] = "memory-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-file-persistence";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pFileRuntime, &tRunOptions, &pFilePendingRun) == XWORK_OK);

    xwork_orchestrator_options_init(&tPendingExecOptions);
    tPendingExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tPendingExecOptions.pUserData = &tToolExecCtx;
    tPendingExecOptions.iMaxTurns = 3u;
    tPendingExecOptions.bAutoApprove = false;
    assert(xwork_run_execute(pFilePendingRun, &tPendingExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pFilePendingRun) == XWORK_RUN_WAITING_APPROVAL);
    assert(tAdapterCtx.iObservedMemoryTurns == 1);
    xwork_memory_context_reset(&tObservedMemoryContext);
    assert(
        xwork_run_get_last_memory_context(
            pFilePendingRun,
            &tObservedMemoryContext
        ) == XWORK_OK
    );
    assert(tObservedMemoryContext.sText != NULL);
    assert(strstr(tObservedMemoryContext.sText, "Workspace memory: memory-main") != NULL);
    assert(strstr(tObservedMemoryContext.sText, tMemoryCtx.sContextText) != NULL);
    assert(tObservedMemoryContext.iWorkspaceCount == 1u);
    xwork_checkpoint_init(&tPendingCheckpoint);
    assert(xwork_run_get_last_checkpoint(pFilePendingRun, &tPendingCheckpoint) == XWORK_OK);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    sFileBeforeToolCheckpointId = xwork_test_dup_cstr(tPendingCheckpoint.sCheckpointId);
    assert(sFileBeforeToolCheckpointId != NULL);
    assert(xwork_file_persistence_list_runs(&tFilePersistenceStore, &tPersistedRunIds) == XWORK_OK);
    assert(tPersistedRunIds.iCount == 1u);
    assert(strcmp(tPersistedRunIds.psItems[0], "run-file-persistence") == 0);
    assert(
        xwork_file_persistence_list_checkpoints(
            &tFilePersistenceStore,
            "run-file-persistence",
            &tPersistedCheckpointIds
        ) == XWORK_OK
    );
    assert(tPersistedCheckpointIds.iCount == 1u);
    assert(strcmp(tPersistedCheckpointIds.psItems[0], sFileBeforeToolCheckpointId) == 0);
    assert(
        xwork_file_persistence_list_artifacts(
            &tFilePersistenceStore,
            "run-file-persistence",
            &tPersistedArtifactIds
        ) == XWORK_OK
    );
    assert(tPersistedArtifactIds.iCount == 0u);

    xwork_runtime_destroy(pFileRuntime);
    pFileRuntime = NULL;

    assert(
        xwork_file_persistence_configure_backend(
            &tFilePersistenceRecoverStore,
            &tFilePersistenceOptions,
            &tFilePersistenceBackend
        ) == XWORK_OK
    );
    xwork_runtime_options_init(&tFileRuntimeOptions);
    tFileRuntimeOptions.pLlmRuntime = pLlmRuntime;
    tFileRuntimeOptions.pHostServices = &tHostServices;
    tFileRuntimeOptions.pPersistenceBackend = &tFilePersistenceBackend;
    tFileRuntimeOptions.tPolicy = tPolicyOptions;
    assert(xwork_runtime_create(&tFileRuntimeOptions, &pFileRecoverRuntime) == XWORK_OK);

    xwork_workspace_options_init(&tWorkspaceOptions);
    tWorkspaceOptions.sWorkspaceId = "memory-main";
    tWorkspaceOptions.sRootPath = "D:/git/xwork";
    assert(
        xwork_profile_apply_workspace_options(
            &tAutonomousProfile,
            &tWorkspaceOptions
        ) == XWORK_OK
    );
    assert(tWorkspaceOptions.bEnableMemory);
    tWorkspaceOptions.pMemory = pFileWorkspaceMemory;
    assert(
        xwork_runtime_add_workspace(
            pFileRecoverRuntime,
            &tWorkspaceOptions,
            &pFileRecoverWorkspace
        ) == XWORK_OK
    );
    assert(xwork_runtime_register_tool(pFileRecoverRuntime, &tToolDef) == XWORK_OK);
    assert(
        xwork_runtime_register_builtin_tool(
            pFileRecoverRuntime,
            XWORK_TOOL_FILESYSTEM_READ_TEXT
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_recover_run_from_persistence(
            pFileRecoverRuntime,
            "run-file-persistence",
            &pFileRecoveredRun
        ) == XWORK_OK
    );
    assert(xwork_run_get_state(pFileRecoveredRun) == XWORK_RUN_WAITING_APPROVAL);
    assert(xwork_run_get_snapshot(pFileRecoveredRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sSessionStateData != NULL);
    assert(strstr(tRunSnapshot.sSessionStateData, "xllm_session_state") != NULL);
    assert(!tRunSnapshot.tSessionPolicy.bEnableAutoCompact);
    assert(tRunSnapshot.tSessionPolicy.fCompactTriggerRatio == 0.60);
    assert(tRunSnapshot.tSessionPolicy.iCompactTriggerTurns == 5u);
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_memory_context_reset(&tObservedMemoryContext);
    assert(
        xwork_run_get_last_memory_context(
            pFileRecoveredRun,
            &tObservedMemoryContext
        ) == XWORK_OK
    );
    assert(tObservedMemoryContext.sText != NULL);
    assert(strstr(tObservedMemoryContext.sText, "Workspace memory: memory-main") != NULL);
    assert(strstr(tObservedMemoryContext.sText, tMemoryCtx.sContextText) != NULL);
    assert(tObservedMemoryContext.iWorkspaceCount == 1u);
    xwork_approval_request_init(&tPendingApproval);
    assert(xwork_run_get_last_approval_request(pFileRecoveredRun, &tPendingApproval) == XWORK_OK);
    assert(tPendingApproval.eState == XWORK_APPROVAL_PENDING);
    assert(xwork_run_load_checkpoint(pFileRecoveredRun, sFileBeforeToolCheckpointId) == XWORK_OK);
    assert(xwork_run_get_state(pFileRecoveredRun) == XWORK_RUN_WAITING_APPROVAL);
    xwork_memory_context_reset(&tObservedMemoryContext);
    assert(
        xwork_run_get_last_memory_context(
            pFileRecoveredRun,
            &tObservedMemoryContext
        ) == XWORK_OK
    );
    assert(tObservedMemoryContext.sText != NULL);
    assert(strstr(tObservedMemoryContext.sText, "Workspace memory: memory-main") != NULL);
    assert(strstr(tObservedMemoryContext.sText, tMemoryCtx.sContextText) != NULL);
    assert(tObservedMemoryContext.iWorkspaceCount == 1u);
    xwork_approval_request_init(&tPendingApproval);
    assert(xwork_run_get_last_approval_request(pFileRecoveredRun, &tPendingApproval) == XWORK_OK);
    assert(tPendingApproval.eState == XWORK_APPROVAL_PENDING);
    xwork_event_init(&tPendingEvent);
    assert(xwork_run_get_last_event(pFileRecoveredRun, &tPendingEvent) == XWORK_OK);
    assert(tPendingEvent.eKind == XWORK_EVENT_CHECKPOINT_LOADED);
    assert(xwork_run_submit_approval(pFileRecoveredRun, XWORK_APPROVAL_APPROVED) == XWORK_OK);
    assert(xwork_run_resume(pFileRecoveredRun) == XWORK_OK);
    tAdapterCtx.iTurnCount = 1;
    assert(xwork_run_execute(pFileRecoveredRun, &tPendingExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pFileRecoveredRun) == XWORK_RUN_COMPLETED);
    assert(
        strcmp(
            xwork_run_get_last_output_text(pFileRecoveredRun),
            "Mock model completed after tool result."
        ) == 0
    );
    assert(tToolExecCtx.iExecCount == 6);
    assert(tAdapterCtx.iObservedMemoryTurns == 2);
    assert(xwork_run_get_artifact_count(pFileRecoveredRun) == 1u);
    assert(
        xwork_file_persistence_list_checkpoints(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            &tPersistedCheckpointIds
        ) == XWORK_OK
    );
    assert(tPersistedCheckpointIds.iCount == 3u);
    assert(strcmp(tPersistedCheckpointIds.psItems[0], sFileBeforeToolCheckpointId) == 0);
    assert(
        xwork_file_persistence_list_artifacts(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            &tPersistedArtifactIds
        ) == XWORK_OK
    );
    assert(tPersistedArtifactIds.iCount == 1u);
    assert(
        xwork_file_persistence_list_events(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            &tPersistedEventIds
        ) == XWORK_OK
    );
    assert(tPersistedEventIds.iCount >= 3u);
    xwork_run_summary_reset(&tLoadedSummary);
    assert(
        xwork_file_persistence_load_run_summary(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            &tLoadedSummary
        ) == XWORK_OK
    );
    assert(strcmp(tLoadedSummary.sRunId, "run-file-persistence") == 0);
    assert(tLoadedSummary.eState == XWORK_RUN_COMPLETED);
    assert(tLoadedSummary.iWorkspaceCount == 1u);
    assert(
        xwork_file_persistence_load_run_snapshot(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            &tRunSnapshot
        ) == XWORK_OK
    );
    assert(tRunSnapshot.eState == XWORK_RUN_COMPLETED);
    assert(tRunSnapshot.sSessionStateData != NULL);
    assert(strstr(tRunSnapshot.sSessionStateData, "xllm_session_state") != NULL);
    assert(!tRunSnapshot.tSessionPolicy.bEnableAutoCompact);
    assert(tRunSnapshot.tSessionPolicy.fCompactTriggerRatio == 0.60);
    assert(tRunSnapshot.tSessionPolicy.iCompactTriggerTurns == 5u);
    xwork_run_snapshot_reset(&tRunSnapshot);
    assert(
        xwork_file_persistence_load_checkpoint_snapshot(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            sFileBeforeToolCheckpointId,
            &tRunSnapshot
        ) == XWORK_OK
    );
    assert(tRunSnapshot.eState == XWORK_RUN_WAITING_APPROVAL);
    assert(tRunSnapshot.eLastCheckpointKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    assert(tRunSnapshot.sSessionStateData != NULL);
    assert(strstr(tRunSnapshot.sSessionStateData, "xllm_session_state") != NULL);
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_checkpoint_reset(&tPendingCheckpoint);
    assert(
        xwork_file_persistence_load_checkpoint(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            sFileBeforeToolCheckpointId,
            &tPendingCheckpoint
        ) == XWORK_OK
    );
    assert(strcmp(tPendingCheckpoint.sCheckpointId, sFileBeforeToolCheckpointId) == 0);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    assert(tPendingCheckpoint.eRunState == XWORK_RUN_WAITING_APPROVAL);
    assert(tPendingCheckpoint.sSessionStateRef != NULL);
    xwork_checkpoint_reset(&tPendingCheckpoint);
    assert(
        xwork_file_persistence_load_last_checkpoint(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            &tPendingCheckpoint
        ) == XWORK_OK
    );
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_COMPLETION);
    assert(tPendingCheckpoint.eRunState == XWORK_RUN_COMPLETED);
    xwork_approval_request_reset(&tPendingApproval);
    assert(
        xwork_file_persistence_load_last_approval_request(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            &tPendingApproval
        ) == XWORK_OK
    );
    assert(tPendingApproval.sRequestId != NULL);
    assert(strcmp(tPendingApproval.sToolId, "mock.apply_patch") == 0);
    assert(tPendingApproval.eState == XWORK_APPROVAL_APPROVED);
    xwork_event_reset(&tLoadedEvent);
    assert(
        xwork_file_persistence_load_last_event(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            &tLoadedEvent
        ) == XWORK_OK
    );
    assert(tLoadedEvent.eKind == XWORK_EVENT_RUN_COMPLETED);
    assert(tLoadedEvent.eRunState == XWORK_RUN_COMPLETED);
    xwork_event_reset(&tLoadedEvent);
    assert(
        xwork_file_persistence_load_event(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            tPersistedEventIds.psItems[0],
            &tLoadedEvent
        ) == XWORK_OK
    );
    assert(tLoadedEvent.eKind == XWORK_EVENT_RUN_CREATED);
    assert(strcmp(tLoadedEvent.sSummary, "Run created.") == 0);
    xwork_artifact_reset(&tLoadedArtifact);
    assert(
        xwork_file_persistence_load_artifact(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            tPersistedArtifactIds.psItems[0],
            &tLoadedArtifact
        ) == XWORK_OK
    );
    assert(tLoadedArtifact.eKind == XWORK_ARTIFACT_PATCH);
    assert(strcmp(tLoadedArtifact.sName, "README.patch") == 0);
    assert(tLoadedArtifact.sContentText != NULL);
    assert(strstr(tLoadedArtifact.sContentText, "xwork smoke patch") != NULL);
    assert(strcmp(tLoadedArtifact.sMimeType, "text/x-diff") == 0);
    assert(strcmp(tLoadedArtifact.sStorageRef, "workspace://README.md") == 0);
    xwork_artifact_reset(&tLoadedArtifact);
    assert(
        xwork_file_persistence_load_last_artifact(
            &tFilePersistenceRecoverStore,
            "run-file-persistence",
            &tLoadedArtifact
        ) == XWORK_OK
    );
    assert(tLoadedArtifact.eKind == XWORK_ARTIFACT_PATCH);
    assert(strcmp(tLoadedArtifact.sName, "README.patch") == 0);
    assert(tLoadedArtifact.sContentText != NULL);
    assert(strstr(tLoadedArtifact.sContentText, "xwork smoke patch") != NULL);
    assert(strcmp(tLoadedArtifact.sMimeType, "text/x-diff") == 0);
    assert(strcmp(tLoadedArtifact.sStorageRef, "workspace://README.md") == 0);

    tAdapterCtx.iTurnCount = 0;
    tAdapterCtx.sExpectedMemoryText = tMemoryCtx.sContextText;
    tAdapterCtx.iObservedMemoryTurns = 0;
    asWorkspaceIds[0] = "memory-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-file-persistence-2";
    tRunOptions.sInstruction = "Append another line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pFileRecoverRuntime,
            &tRunOptions,
            &pFileIndexedRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tExecOptions.pUserData = &tToolExecCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pFileIndexedRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pFileIndexedRun) == XWORK_RUN_COMPLETED);
    xwork_run_summary_list_reset(&tPersistedRunSummaries);
    assert(
        xwork_file_persistence_list_run_summaries(
            &tFilePersistenceRecoverStore,
            &tPersistedRunSummaries
        ) == XWORK_OK
    );
    assert(tPersistedRunSummaries.iCount == 2u);
    assert(strcmp(tPersistedRunSummaries.pItems[0].sRunId, "run-file-persistence") == 0);
    assert(
        strcmp(
            tPersistedRunSummaries.pItems[0].sInstruction,
            "Append a line to the README if needed."
        ) == 0
    );
    assert(tPersistedRunSummaries.pItems[0].eState == XWORK_RUN_COMPLETED);
    assert(tPersistedRunSummaries.pItems[0].iWorkspaceCount == 1u);
    assert(strcmp(tPersistedRunSummaries.pItems[1].sRunId, "run-file-persistence-2") == 0);
    assert(
        strcmp(
            tPersistedRunSummaries.pItems[1].sInstruction,
            "Append another line to the README if needed."
        ) == 0
    );
    assert(tPersistedRunSummaries.pItems[1].eState == XWORK_RUN_COMPLETED);
    assert(tPersistedRunSummaries.pItems[1].iWorkspaceCount == 1u);
    xwork_run_index_list_reset(&tPersistedRunIndex);
    assert(
        xwork_file_persistence_list_run_index(
            &tFilePersistenceRecoverStore,
            &tPersistedRunIndex
        ) == XWORK_OK
    );
    assert(tPersistedRunIndex.iCount == 2u);
    assert(strcmp(tPersistedRunIndex.pItems[0].tSummary.sRunId, "run-file-persistence") == 0);
    assert(tPersistedRunIndex.pItems[0].bHasLastApprovalRequest);
    assert(tPersistedRunIndex.pItems[0].tLastApprovalRequest.sRequestId != NULL);
    assert(strcmp(tPersistedRunIndex.pItems[0].tLastApprovalRequest.sToolId, "mock.apply_patch") == 0);
    assert(tPersistedRunIndex.pItems[0].tLastApprovalRequest.eState == XWORK_APPROVAL_APPROVED);
    assert(tPersistedRunIndex.pItems[0].bHasLastEvent);
    assert(tPersistedRunIndex.pItems[0].tLastEvent.eKind == XWORK_EVENT_RUN_COMPLETED);
    assert(tPersistedRunIndex.pItems[0].tLastEvent.eRunState == XWORK_RUN_COMPLETED);
    assert(tPersistedRunIndex.pItems[0].iEventCount >= 3u);
    assert(tPersistedRunIndex.pItems[0].bHasLastCheckpoint);
    assert(tPersistedRunIndex.pItems[0].tLastCheckpoint.eKind == XWORK_CHECKPOINT_COMPLETION);
    assert(tPersistedRunIndex.pItems[0].tLastCheckpoint.eRunState == XWORK_RUN_COMPLETED);
    assert(tPersistedRunIndex.pItems[0].iCheckpointCount == 3u);
    assert(tPersistedRunIndex.pItems[0].iArtifactCount == 1u);
    assert(tPersistedRunIndex.pItems[0].bHasLastArtifact);
    assert(tPersistedRunIndex.pItems[0].tLastArtifact.eKind == XWORK_ARTIFACT_PATCH);
    assert(strcmp(tPersistedRunIndex.pItems[0].tLastArtifact.sName, "README.patch") == 0);
    assert(strcmp(tPersistedRunIndex.pItems[0].tLastArtifact.sStorageRef, "workspace://README.md") == 0);
    assert(tPersistedRunIndex.pItems[0].tLastArtifact.sContentText != NULL);
    assert(strcmp(tPersistedRunIndex.pItems[1].tSummary.sRunId, "run-file-persistence-2") == 0);
    assert(tPersistedRunIndex.pItems[1].bHasLastApprovalRequest);
    assert(tPersistedRunIndex.pItems[1].tLastApprovalRequest.sRequestId != NULL);
    assert(strcmp(tPersistedRunIndex.pItems[1].tLastApprovalRequest.sToolId, "mock.apply_patch") == 0);
    assert(tPersistedRunIndex.pItems[1].tLastApprovalRequest.eState == XWORK_APPROVAL_APPROVED);
    assert(tPersistedRunIndex.pItems[1].bHasLastEvent);
    assert(tPersistedRunIndex.pItems[1].tLastEvent.eKind == XWORK_EVENT_RUN_COMPLETED);
    assert(tPersistedRunIndex.pItems[1].iEventCount >= 3u);
    assert(tPersistedRunIndex.pItems[1].bHasLastCheckpoint);
    assert(tPersistedRunIndex.pItems[1].tLastCheckpoint.eKind == XWORK_CHECKPOINT_COMPLETION);
    assert(tPersistedRunIndex.pItems[1].tLastCheckpoint.eRunState == XWORK_RUN_COMPLETED);
    assert(tPersistedRunIndex.pItems[1].iCheckpointCount >= 1u);
    assert(tPersistedRunIndex.pItems[1].iArtifactCount == 1u);
    assert(tPersistedRunIndex.pItems[1].bHasLastArtifact);
    assert(tPersistedRunIndex.pItems[1].tLastArtifact.eKind == XWORK_ARTIFACT_PATCH);
    assert(strcmp(tPersistedRunIndex.pItems[1].tLastArtifact.sName, "README.patch") == 0);
    assert(strcmp(tPersistedRunIndex.pItems[1].tLastArtifact.sStorageRef, "workspace://README.md") == 0);
    assert(tPersistedRunIndex.pItems[1].tLastArtifact.sContentText != NULL);

    tAdapterCtx.iTurnCount = 0;
    tAdapterCtx.sExpectedMemoryText = tMemoryCtx.sContextText;
    tAdapterCtx.iObservedMemoryTurns = 0;
    asWorkspaceIds[0] = "memory-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-file-persistence-pending";
    tRunOptions.sInstruction = "Review the README before applying changes.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pFileRecoverRuntime,
            &tRunOptions,
            &pFileIndexedRun
        ) == XWORK_OK
    );
    xwork_orchestrator_options_init(&tPendingExecOptions);
    tPendingExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tPendingExecOptions.pUserData = &tToolExecCtx;
    tPendingExecOptions.pfnResolveMemoryContext = xwork_mock_memory_resolve;
    tPendingExecOptions.pMemoryUserData = &tMemoryCtx;
    tPendingExecOptions.iMaxTurns = 3u;
    tPendingExecOptions.bAutoApprove = false;
    assert(xwork_run_execute(pFileIndexedRun, &tPendingExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pFileIndexedRun) == XWORK_RUN_WAITING_APPROVAL);

    xwork_run_index_query_init(&tRunIndexQuery);
    tRunIndexQuery.eSort = XWORK_RUN_INDEX_SORT_RUN_ID_DESC;
    xwork_run_index_list_reset(&tPersistedRunIndex);
    assert(
        xwork_file_persistence_query_run_index(
            &tFilePersistenceRecoverStore,
            &tRunIndexQuery,
            &tPersistedRunIndex
        ) == XWORK_OK
    );
    assert(tPersistedRunIndex.iCount == 3u);
    assert(strcmp(tPersistedRunIndex.pItems[0].tSummary.sRunId, "run-file-persistence-pending") == 0);
    assert(strcmp(tPersistedRunIndex.pItems[2].tSummary.sRunId, "run-file-persistence") == 0);

    xwork_run_index_query_init(&tRunIndexQuery);
    tRunIndexQuery.bFilterLastApprovalState = true;
    tRunIndexQuery.eLastApprovalState = XWORK_APPROVAL_PENDING;
    tRunIndexQuery.bRequireLastCheckpoint = true;
    xwork_run_index_list_reset(&tPersistedRunIndex);
    assert(
        xwork_file_persistence_query_run_index(
            &tFilePersistenceRecoverStore,
            &tRunIndexQuery,
            &tPersistedRunIndex
        ) == XWORK_OK
    );
    assert(tPersistedRunIndex.iCount == 1u);
    assert(strcmp(tPersistedRunIndex.pItems[0].tSummary.sRunId, "run-file-persistence-pending") == 0);
    assert(tPersistedRunIndex.pItems[0].tSummary.eState == XWORK_RUN_WAITING_APPROVAL);
    assert(tPersistedRunIndex.pItems[0].bHasLastApprovalRequest);
    assert(tPersistedRunIndex.pItems[0].tLastApprovalRequest.eState == XWORK_APPROVAL_PENDING);
    assert(tPersistedRunIndex.pItems[0].bHasLastCheckpoint);
    assert(tPersistedRunIndex.pItems[0].tLastCheckpoint.eKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    assert(tPersistedRunIndex.pItems[0].tLastCheckpoint.eRunState == XWORK_RUN_WAITING_APPROVAL);
    assert(tPersistedRunIndex.pItems[0].iCheckpointCount == 1u);
    assert(tPersistedRunIndex.pItems[0].iArtifactCount == 0u);
    assert(!tPersistedRunIndex.pItems[0].bHasLastArtifact);

    xwork_run_index_query_init(&tRunIndexQuery);
    tRunIndexQuery.bFilterLastEventKind = true;
    tRunIndexQuery.eLastEventKind = XWORK_EVENT_RUN_COMPLETED;
    tRunIndexQuery.bFilterMinArtifactCount = true;
    tRunIndexQuery.iMinArtifactCount = 1u;
    xwork_run_index_list_reset(&tPersistedRunIndex);
    assert(
        xwork_file_persistence_query_run_index(
            &tFilePersistenceRecoverStore,
            &tRunIndexQuery,
            &tPersistedRunIndex
        ) == XWORK_OK
    );
    assert(tPersistedRunIndex.iCount == 2u);
    assert(strcmp(tPersistedRunIndex.pItems[0].tSummary.sRunId, "run-file-persistence") == 0);
    assert(strcmp(tPersistedRunIndex.pItems[1].tSummary.sRunId, "run-file-persistence-2") == 0);
    assert(tPersistedRunIndex.pItems[0].iArtifactCount == 1u);
    assert(tPersistedRunIndex.pItems[1].iArtifactCount == 1u);

    xwork_string_list_reset(&tPersistedRunIds);
    assert(
        xwork_runtime_list_persisted_runs(
            pFileRecoverRuntime,
            &tPersistedRunIds
        ) == XWORK_OK
    );
    assert(tPersistedRunIds.iCount == 3u);
    assert(strcmp(tPersistedRunIds.psItems[0], "run-file-persistence") == 0);
    assert(strcmp(tPersistedRunIds.psItems[2], "run-file-persistence-pending") == 0);

    xwork_run_summary_list_reset(&tPersistedRunSummaries);
    assert(
        xwork_runtime_list_persisted_run_summaries(
            pFileRecoverRuntime,
            &tPersistedRunSummaries
        ) == XWORK_OK
    );
    assert(tPersistedRunSummaries.iCount == 3u);
    assert(strcmp(tPersistedRunSummaries.pItems[0].sRunId, "run-file-persistence") == 0);
    assert(tPersistedRunSummaries.pItems[0].eState == XWORK_RUN_COMPLETED);
    assert(strcmp(tPersistedRunSummaries.pItems[1].sRunId, "run-file-persistence-2") == 0);
    assert(tPersistedRunSummaries.pItems[1].eState == XWORK_RUN_COMPLETED);
    assert(strcmp(tPersistedRunSummaries.pItems[2].sRunId, "run-file-persistence-pending") == 0);
    assert(tPersistedRunSummaries.pItems[2].eState == XWORK_RUN_WAITING_APPROVAL);

    xwork_run_index_query_init(&tRunIndexQuery);
    tRunIndexQuery.eSort = XWORK_RUN_INDEX_SORT_ARTIFACT_COUNT_DESC;
    xwork_run_index_list_reset(&tPersistedRunIndex);
    assert(
        xwork_runtime_query_persisted_run_index(
            pFileRecoverRuntime,
            &tRunIndexQuery,
            &tPersistedRunIndex
        ) == XWORK_OK
    );
    assert(tPersistedRunIndex.iCount == 3u);
    assert(tPersistedRunIndex.pItems[0].iArtifactCount == 1u);
    assert(tPersistedRunIndex.pItems[1].iArtifactCount == 1u);
    assert(strcmp(tPersistedRunIndex.pItems[2].tSummary.sRunId, "run-file-persistence-pending") == 0);
    assert(tPersistedRunIndex.pItems[2].iArtifactCount == 0u);

    xwork_run_index_list_reset(&tPersistedRunIndex);
    assert(
        xwork_runtime_list_persisted_run_index(
            pFileRecoverRuntime,
            &tPersistedRunIndex
        ) == XWORK_OK
    );
    assert(tPersistedRunIndex.iCount == 3u);
    assert(strcmp(tPersistedRunIndex.pItems[2].tSummary.sRunId, "run-file-persistence-pending") == 0);
    assert(tPersistedRunIndex.pItems[2].bHasLastEvent);
    assert(tPersistedRunIndex.pItems[2].bHasLastApprovalRequest);
    assert(tPersistedRunIndex.pItems[2].tLastApprovalRequest.eState == XWORK_APPROVAL_PENDING);
    assert(tPersistedRunIndex.pItems[2].bHasLastCheckpoint);
    assert(tPersistedRunIndex.pItems[2].iCheckpointCount == 1u);
    assert(tPersistedRunIndex.pItems[2].iArtifactCount == 0u);

    xwork_run_index_query_init(&tRunIndexQuery);
    tRunIndexQuery.bFilterMaxEventCount = true;
    tRunIndexQuery.iMaxEventCount = tPersistedRunIndex.pItems[2].iEventCount;
    tRunIndexQuery.bFilterMaxCheckpointCount = true;
    tRunIndexQuery.iMaxCheckpointCount = tPersistedRunIndex.pItems[2].iCheckpointCount;
    tRunIndexQuery.bFilterMaxArtifactCount = true;
    tRunIndexQuery.iMaxArtifactCount = tPersistedRunIndex.pItems[2].iArtifactCount;
    tRunIndexQuery.bFilterLastApprovalState = true;
    tRunIndexQuery.eLastApprovalState = XWORK_APPROVAL_PENDING;
    tRunIndexQuery.bFilterMinLastEventSequence = true;
    tRunIndexQuery.iMinLastEventSequence = tPersistedRunIndex.pItems[2].tLastEvent.iSequence;
    tRunIndexQuery.bFilterMaxLastEventSequence = true;
    tRunIndexQuery.iMaxLastEventSequence = tPersistedRunIndex.pItems[2].tLastEvent.iSequence;
    tRunIndexQuery.bFilterMinLastCheckpointSequence = true;
    tRunIndexQuery.iMinLastCheckpointSequence = tPersistedRunIndex.pItems[2].tLastCheckpoint.iSequence;
    tRunIndexQuery.bFilterMaxLastCheckpointSequence = true;
    tRunIndexQuery.iMaxLastCheckpointSequence = tPersistedRunIndex.pItems[2].tLastCheckpoint.iSequence;
    xwork_run_index_list_reset(&tPersistedRunIndex);
    assert(
        xwork_runtime_query_persisted_run_index(
            pFileRecoverRuntime,
            &tRunIndexQuery,
            &tPersistedRunIndex
        ) == XWORK_OK
    );
    assert(tPersistedRunIndex.iCount == 1u);
    assert(strcmp(tPersistedRunIndex.pItems[0].tSummary.sRunId, "run-file-persistence-pending") == 0);

    xwork_run_index_query_init(&tRunIndexQuery);
    tRunIndexQuery.bFilterLastApprovalState = true;
    tRunIndexQuery.eLastApprovalState = XWORK_APPROVAL_PENDING;
    tRunIndexQuery.bRequireLastCheckpoint = true;
    xwork_run_index_list_reset(&tPersistedRunIndex);
    assert(
        xwork_runtime_query_persisted_run_index(
            pFileRecoverRuntime,
            &tRunIndexQuery,
            &tPersistedRunIndex
        ) == XWORK_OK
    );
    assert(tPersistedRunIndex.iCount == 1u);
    assert(strcmp(tPersistedRunIndex.pItems[0].tSummary.sRunId, "run-file-persistence-pending") == 0);

    xwork_run_summary_reset(&tLoadedSummary);
    assert(
        xwork_runtime_load_persisted_run_summary(
            pFileRecoverRuntime,
            "run-file-persistence",
            &tLoadedSummary
        ) == XWORK_OK
    );
    assert(strcmp(tLoadedSummary.sRunId, "run-file-persistence") == 0);
    assert(tLoadedSummary.eState == XWORK_RUN_COMPLETED);

    xwork_approval_request_reset(&tPendingApproval);
    assert(
        xwork_runtime_load_persisted_last_approval_request(
            pFileRecoverRuntime,
            "run-file-persistence",
            &tPendingApproval
        ) == XWORK_OK
    );
    assert(strcmp(tPendingApproval.sToolId, "mock.apply_patch") == 0);
    assert(tPendingApproval.eState == XWORK_APPROVAL_APPROVED);

    xwork_event_reset(&tLoadedEvent);
    assert(
        xwork_runtime_load_persisted_last_event(
            pFileRecoverRuntime,
            "run-file-persistence",
            &tLoadedEvent
        ) == XWORK_OK
    );
    assert(tLoadedEvent.eKind == XWORK_EVENT_RUN_COMPLETED);
    assert(tLoadedEvent.eRunState == XWORK_RUN_COMPLETED);

    xwork_checkpoint_reset(&tPendingCheckpoint);
    assert(
        xwork_runtime_load_persisted_last_checkpoint(
            pFileRecoverRuntime,
            "run-file-persistence",
            &tPendingCheckpoint
        ) == XWORK_OK
    );
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_COMPLETION);
    assert(tPendingCheckpoint.eRunState == XWORK_RUN_COMPLETED);

    xwork_artifact_reset(&tLoadedArtifact);
    assert(
        xwork_runtime_load_persisted_last_artifact(
            pFileRecoverRuntime,
            "run-file-persistence",
            &tLoadedArtifact
        ) == XWORK_OK
    );
    assert(tLoadedArtifact.eKind == XWORK_ARTIFACT_PATCH);
    assert(strcmp(tLoadedArtifact.sName, "README.patch") == 0);
    assert(tLoadedArtifact.sContentText != NULL);
    assert(strstr(tLoadedArtifact.sContentText, "xwork smoke patch") != NULL);
    xwork_event_reset(&tLoadedEvent);
    assert(
        xwork_runtime_load_persisted_event(
            pFileRecoverRuntime,
            "run-file-persistence",
            tPersistedEventIds.psItems[0],
            &tLoadedEvent
        ) == XWORK_OK
    );
    assert(tLoadedEvent.eKind == XWORK_EVENT_RUN_CREATED);
    assert(strcmp(tLoadedEvent.sSummary, "Run created.") == 0);

    xwork_checkpoint_reset(&tPendingCheckpoint);
    assert(
        xwork_runtime_load_persisted_checkpoint(
            pFileRecoverRuntime,
            "run-file-persistence",
            sFileBeforeToolCheckpointId,
            &tPendingCheckpoint
        ) == XWORK_OK
    );
    assert(strcmp(tPendingCheckpoint.sCheckpointId, sFileBeforeToolCheckpointId) == 0);
    assert(tPendingCheckpoint.eKind == XWORK_CHECKPOINT_BEFORE_TOOL);
    assert(tPendingCheckpoint.eRunState == XWORK_RUN_WAITING_APPROVAL);

    xwork_artifact_reset(&tLoadedArtifact);
    assert(
        xwork_runtime_load_persisted_artifact(
            pFileRecoverRuntime,
            "run-file-persistence",
            tPersistedArtifactIds.psItems[0],
            &tLoadedArtifact
        ) == XWORK_OK
    );
    assert(tLoadedArtifact.eKind == XWORK_ARTIFACT_PATCH);
    assert(strcmp(tLoadedArtifact.sName, "README.patch") == 0);

    xwork_string_list_reset(&tPersistedCheckpointIds);
    assert(
        xwork_runtime_list_persisted_checkpoints(
            pFileRecoverRuntime,
            "run-file-persistence",
            &tPersistedCheckpointIds
        ) == XWORK_OK
    );
    assert(tPersistedCheckpointIds.iCount == 3u);
    assert(strcmp(tPersistedCheckpointIds.psItems[0], sFileBeforeToolCheckpointId) == 0);

    xwork_string_list_reset(&tPersistedEventIds);
    assert(
        xwork_runtime_list_persisted_events(
            pFileRecoverRuntime,
            "run-file-persistence",
            &tPersistedEventIds
        ) == XWORK_OK
    );
    assert(tPersistedEventIds.iCount >= 3u);
    xwork_event_reset(&tLoadedEvent);
    assert(
        xwork_runtime_load_persisted_event(
            pFileRecoverRuntime,
            "run-file-persistence",
            tPersistedEventIds.psItems[0],
            &tLoadedEvent
        ) == XWORK_OK
    );
    assert(tLoadedEvent.eKind == XWORK_EVENT_RUN_CREATED);

    xwork_string_list_reset(&tPersistedArtifactIds);
    assert(
        xwork_runtime_list_persisted_artifacts(
            pFileRecoverRuntime,
            "run-file-persistence",
            &tPersistedArtifactIds
        ) == XWORK_OK
    );
    assert(tPersistedArtifactIds.iCount == 1u);
    xwork_artifact_reset(&tLoadedArtifact);
    assert(
        xwork_runtime_load_persisted_artifact(
            pFileRecoverRuntime,
            "run-file-persistence",
            tPersistedArtifactIds.psItems[0],
            &tLoadedArtifact
        ) == XWORK_OK
    );
    assert(tLoadedArtifact.eKind == XWORK_ARTIFACT_PATCH);
    assert(strcmp(tLoadedArtifact.sName, "README.patch") == 0);

    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_run_snapshot_reset(&tPersistenceCtx.tSnapshot);
    xwork_runtime_destroy(pLocalHostRuntime);
    xwork_runtime_destroy(pFileRecoverRuntime);
    xwork_runtime_destroy(pRuntime);
    xwork_local_host_reset(&tLocalHost);
    xwork_memory_context_reset(&tObservedMemoryContext);
    xwork_run_summary_reset(&tLoadedSummary);
    xwork_event_reset(&tLoadedEvent);
    xwork_artifact_reset(&tLoadedArtifact);
    xwork_artifact_reset(&tArtifact);
    xwork_run_index_list_reset(&tPersistedRunIndex);
    xwork_run_summary_list_reset(&tPersistedRunSummaries);
    xwork_string_list_reset(&tPersistedArtifactIds);
    xwork_string_list_reset(&tPersistedCheckpointIds);
    xwork_string_list_reset(&tPersistedEventIds);
    xwork_string_list_reset(&tPersistedRunIds);
    xwork_file_persistence_reset(&tFilePersistenceRecoverStore);
    xwork_file_persistence_reset(&tFilePersistenceStore);
    (void)remove(sLocalHostMissingPath);
    (void)remove(sLocalHostWritePath);
    (void)remove(sLocalHostAppendPath);
    (void)remove(sLocalHostCreatePath);
    (void)remove(sLocalHostCreateDirsPath);
    (void)remove(sLocalHostOrchestratorWritePath);
    (void)remove(sLocalHostOrchestratorAppendPath);
    (void)remove(sLocalHostOrchestratorCreatePath);
    (void)remove(sLocalHostOrchestratorCreateDirsPath);
    xwork_test_remove_empty_directory(sLocalHostCreateDirsDir);
    xwork_test_remove_empty_directory(sLocalHostOrchestratorCreateDirsDir);
    xwork_test_remove_empty_directory(sLocalHostCreateDirsParentDir);
    free(sPendingBeforeToolCheckpointId);
    free(sAfterToolCheckpointId);
    free(sFileBeforeToolCheckpointId);
    free(sFirstArtifactId);
    xllm_memory_destroy(pFileWorkspaceMemory);
    xllm_memory_destroy(pWorkspaceMemory);
    xllm_runtime_destroy(pLlmRuntime);
    return 0;
}
