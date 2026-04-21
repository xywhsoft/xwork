#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define XWORK_TEST_MKDIR(path) _mkdir(path)
#define XWORK_TEST_RMDIR _rmdir
#else
#include <sys/stat.h>
#include <unistd.h>
#define XWORK_TEST_MKDIR(path) mkdir((path), 0777)
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
    bool bTerminalWriteEof;
    bool bTerminalListFlow;
} xwork_mock_adapter_ctx;

typedef struct {
    int iExecCount;
    int iExecExCount;
    int iCancelCheckCount;
} xwork_mock_tool_exec_ctx;

typedef struct {
    int iInvokeCount;
} xwork_mock_host_ctx;

typedef struct {
    int iResolveCount;
    const char *sContextText;
} xwork_mock_memory_ctx;

typedef struct {
    const char *sInterruptPhase;
    int iCheckCount;
} xwork_mock_interrupt_ctx;

typedef struct {
    int iEventCount;
    int iTextDeltaCount;
    bool bCancelOnTextDelta;
} xwork_mock_model_event_ctx;

typedef struct {
    int iStoreEventCount;
    int iStoreCheckpointCount;
    int iStoreArtifactCount;
    int iLoadSnapshotCount;
    bool bHasSnapshot;
    xwork_run_snapshot tSnapshot;
} xwork_mock_persistence_ctx;

#define XWORK_TEST_STRINGIFY_INNER(x) #x
#define XWORK_TEST_STRINGIFY(x) XWORK_TEST_STRINGIFY_INNER(x)

#ifdef _WIN32
#define XWORK_TEST_PROCESS_ENV_COMMAND "set XWORK_TEST_PROCESS_ENV"
#define XWORK_TEST_PROCESS_ENV_EXPECTED "XWORK_TEST_PROCESS_ENV=xwork-process-env"
#define XWORK_TEST_PROCESS_STDIN_COMMAND "more"
#define XWORK_TEST_PROCESS_NONZERO_COMMAND_RAW \
    "echo xwork-local-process-nonzero && exit /b 3"
#define XWORK_TEST_PROCESS_NONZERO_COMMAND \
    "echo xwork-local-process-nonzero && exit /b 3"
#define XWORK_TEST_PROCESS_STDERR_COMMAND_RAW \
    "echo xwork-local-process-stdout && echo xwork-local-process-stderr 1>&2"
#define XWORK_TEST_PROCESS_STDERR_COMMAND XWORK_TEST_PROCESS_STDERR_COMMAND_RAW
#define XWORK_TEST_PROCESS_TERMINAL_COMMAND "echo xwork-local-process-terminal"
#define XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND XWORK_TEST_PROCESS_STDIN_COMMAND

#define XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "xwork-terminal-session\r\n"
#define XWORK_TEST_PROCESS_TIMEOUT_COMMAND "ping -n 3 127.0.0.1 >nul"
#define XWORK_TEST_PROCESS_ASYNC_CANCEL_COMMAND "ping -n 6 127.0.0.1 >nul"
#else
#define XWORK_TEST_PROCESS_ENV_COMMAND "printf %s \"$XWORK_TEST_PROCESS_ENV\""
#define XWORK_TEST_PROCESS_ENV_EXPECTED "xwork-process-env"
#define XWORK_TEST_PROCESS_STDIN_COMMAND "cat"
#define XWORK_TEST_PROCESS_NONZERO_COMMAND_RAW \
    "sh -c 'printf %s xwork-local-process-nonzero; exit 3'"
#define XWORK_TEST_PROCESS_NONZERO_COMMAND XWORK_TEST_PROCESS_NONZERO_COMMAND_RAW
#define XWORK_TEST_PROCESS_STDERR_COMMAND_RAW \
    "printf %s xwork-local-process-stdout; printf %s xwork-local-process-stderr >&2"
#define XWORK_TEST_PROCESS_STDERR_COMMAND XWORK_TEST_PROCESS_STDERR_COMMAND_RAW
#define XWORK_TEST_PROCESS_TERMINAL_COMMAND "printf %s xwork-local-process-terminal"
#define XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND XWORK_TEST_PROCESS_STDIN_COMMAND

#define XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "xwork-terminal-session\n"
#define XWORK_TEST_PROCESS_TIMEOUT_COMMAND "sleep 1"
#define XWORK_TEST_PROCESS_ASYNC_CANCEL_COMMAND "sleep 5"
#endif

#define XWORK_TEST_PROCESS_STDIN_INPUT "xwork-process-stdin\\n"
#define XWORK_TEST_PROCESS_STDIN_EXPECTED "xwork-process-stdin"
#define XWORK_TEST_PROCESS_NONZERO_EXPECTED "xwork-local-process-nonzero"
#define XWORK_TEST_PROCESS_STDOUT_EXPECTED "xwork-local-process-stdout"
#define XWORK_TEST_PROCESS_STDERR_EXPECTED "xwork-local-process-stderr"
#define XWORK_TEST_PROCESS_TERMINAL_EXPECTED "xwork-local-process-terminal"
#define XWORK_TEST_PROCESS_TERMINAL_SESSION_EXPECTED "xwork-terminal-session"
#define XWORK_TEST_PROCESS_NONZERO_EXIT_CODE 3
#define XWORK_TEST_PROCESS_TIMEOUT_MS 50

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

static void xwork_test_assert_content_stats(
    const xwork_artifact *pArtifact,
    size_t iByteCount,
    size_t iLineCount
)
{
    assert(pArtifact != NULL);
    assert(pArtifact->bHasContentStats);
    assert(pArtifact->iContentByteCount == iByteCount);
    assert(pArtifact->iContentLineCount == iLineCount);
}

static void xwork_test_assert_summary_content_stats(
    const xwork_artifact_summary *pSummary,
    size_t iByteCount,
    size_t iLineCount
)
{
    assert(pSummary != NULL);
    assert(pSummary->bHasContentStats);
    assert(pSummary->iContentByteCount == iByteCount);
    assert(pSummary->iContentLineCount == iLineCount);
}

static void xwork_test_assert_command_io_stats(
    const xwork_artifact *pArtifact,
    size_t iStdoutByteCount,
    size_t iStderrByteCount,
    bool bStdoutTruncated,
    bool bStderrTruncated
)
{
    assert(pArtifact != NULL);
    assert(pArtifact->bHasCommandIoStats);
    assert(pArtifact->iStdoutByteCount == iStdoutByteCount);
    assert(pArtifact->iStderrByteCount == iStderrByteCount);
    assert(pArtifact->bStdoutTruncated == bStdoutTruncated);
    assert(pArtifact->bStderrTruncated == bStderrTruncated);
}

static void xwork_test_assert_summary_command_io_stats(
    const xwork_artifact_summary *pSummary,
    size_t iStdoutByteCount,
    size_t iStderrByteCount,
    bool bStdoutTruncated,
    bool bStderrTruncated
)
{
    assert(pSummary != NULL);
    assert(pSummary->bHasCommandIoStats);
    assert(pSummary->iStdoutByteCount == iStdoutByteCount);
    assert(pSummary->iStderrByteCount == iStderrByteCount);
    assert(pSummary->bStdoutTruncated == bStdoutTruncated);
    assert(pSummary->bStderrTruncated == bStderrTruncated);
}

static void xwork_test_assert_output_class(
    const xwork_artifact *pArtifact,
    xwork_artifact_output_class eOutputClass,
    const char *sOutputRole
)
{
    assert(pArtifact != NULL);
    assert(pArtifact->eOutputClass == eOutputClass);
    if ( sOutputRole ) {
        assert(pArtifact->sOutputRole != NULL);
        assert(strcmp(pArtifact->sOutputRole, sOutputRole) == 0);
    }
}

static void xwork_test_assert_summary_output_class(
    const xwork_artifact_summary *pSummary,
    xwork_artifact_output_class eOutputClass,
    const char *sOutputRole
)
{
    assert(pSummary != NULL);
    assert(pSummary->eOutputClass == eOutputClass);
    if ( sOutputRole ) {
        assert(pSummary->sOutputRole != NULL);
        assert(strcmp(pSummary->sOutputRole, sOutputRole) == 0);
    }
}

static void xwork_test_assert_report_class(
    const xwork_artifact *pArtifact,
    xwork_artifact_report_class eReportClass,
    const char *sReportSubjectRef
)
{
    assert(pArtifact != NULL);
    assert(pArtifact->eReportClass == eReportClass);
    if ( sReportSubjectRef ) {
        assert(pArtifact->sReportSubjectRef != NULL);
        assert(strcmp(pArtifact->sReportSubjectRef, sReportSubjectRef) == 0);
    }
}

static void xwork_test_assert_summary_report_class(
    const xwork_artifact_summary *pSummary,
    xwork_artifact_report_class eReportClass,
    const char *sReportSubjectRef
)
{
    assert(pSummary != NULL);
    assert(pSummary->eReportClass == eReportClass);
    if ( sReportSubjectRef ) {
        assert(pSummary->sReportSubjectRef != NULL);
        assert(strcmp(pSummary->sReportSubjectRef, sReportSubjectRef) == 0);
    }
}

static void xwork_test_assert_readme_patch_stats(const xwork_artifact *pArtifact)
{
    assert(pArtifact != NULL);
    xwork_test_assert_content_stats(pArtifact, 54u, 4u);
    assert(pArtifact->bHasPatchStats);
    assert(pArtifact->iPatchFileCount == 1u);
    assert(pArtifact->iPatchHunkCount == 1u);
    assert(pArtifact->iPatchAddedLineCount == 1u);
    assert(pArtifact->iPatchDeletedLineCount == 0u);
}

static void xwork_test_assert_readme_patch_summary_stats(
    const xwork_artifact_summary *pSummary
)
{
    assert(pSummary != NULL);
    xwork_test_assert_summary_content_stats(pSummary, 54u, 4u);
    assert(pSummary->bHasPatchStats);
    assert(pSummary->iPatchFileCount == 1u);
    assert(pSummary->iPatchHunkCount == 1u);
    assert(pSummary->iPatchAddedLineCount == 1u);
    assert(pSummary->iPatchDeletedLineCount == 0u);
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
            &pArtifacts[i].sOutputRole,
            pSource->pArtifacts[i].sOutputRole
        );
        if ( iStatus != XWORK_OK ) {
            xwork__run_snapshot_reset_artifacts(pTarget);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(
            &pArtifacts[i].sReportSubjectRef,
            pSource->pArtifacts[i].sReportSubjectRef
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
        pArtifacts[i].eOutputClass = pSource->pArtifacts[i].eOutputClass;
        pArtifacts[i].eReportClass = pSource->pArtifacts[i].eReportClass;
        pArtifacts[i].bHasContentStats = pSource->pArtifacts[i].bHasContentStats;
        pArtifacts[i].iContentByteCount = pSource->pArtifacts[i].iContentByteCount;
        pArtifacts[i].iContentLineCount = pSource->pArtifacts[i].iContentLineCount;
        pArtifacts[i].bHasPatchStats = pSource->pArtifacts[i].bHasPatchStats;
        pArtifacts[i].iPatchFileCount = pSource->pArtifacts[i].iPatchFileCount;
        pArtifacts[i].iPatchHunkCount = pSource->pArtifacts[i].iPatchHunkCount;
        pArtifacts[i].iPatchAddedLineCount = pSource->pArtifacts[i].iPatchAddedLineCount;
        pArtifacts[i].iPatchDeletedLineCount = pSource->pArtifacts[i].iPatchDeletedLineCount;
        pArtifacts[i].bHasCommandIoStats = pSource->pArtifacts[i].bHasCommandIoStats;
        pArtifacts[i].iStdoutByteCount = pSource->pArtifacts[i].iStdoutByteCount;
        pArtifacts[i].iStderrByteCount = pSource->pArtifacts[i].iStderrByteCount;
        pArtifacts[i].bStdoutTruncated = pSource->pArtifacts[i].bStdoutTruncated;
        pArtifacts[i].bStderrTruncated = pSource->pArtifacts[i].bStderrTruncated;
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

static bool xwork_test_parse_json_table(const char *sJson, xvalue *ptTable)
{
    xvalue tValue;

    if ( !ptTable ) {
        return false;
    }

    *ptTable = NULL;
    if ( !sJson || !sJson[0] ) {
        return false;
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

static const char *xwork_test_json_get_text(xvalue tTable, const char *sKey)
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

static xvalue xwork_test_json_get_value(xvalue tTable, const char *sKey)
{
    if ( !tTable || !sKey ) {
        return NULL;
    }
    return xvoTableGetValue(tTable, sKey, (uint32)strlen(sKey));
}

static bool xwork_test_json_get_size(xvalue tTable, const char *sKey, size_t *piValue)
{
    xvalue tValue;
    int64 iValue;

    if ( piValue ) {
        *piValue = 0u;
    }
    if ( !tTable || !sKey || !piValue ) {
        return false;
    }

    tValue = xvoTableGetValue(tTable, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) != XVO_DT_INT ) {
        return false;
    }
    iValue = xvoGetInt(tValue);
    if ( iValue < 0 ) {
        return false;
    }

    *piValue = (size_t)iValue;
    return true;
}

static bool xwork_test_json_get_bool(xvalue tTable, const char *sKey, bool *pbValue)
{
    xvalue tValue;

    if ( pbValue ) {
        *pbValue = false;
    }
    if ( !tTable || !sKey || !pbValue ) {
        return false;
    }

    tValue = xvoTableGetValue(tTable, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) != XVO_DT_BOOL ) {
        return false;
    }

    *pbValue = xvoGetBool(tValue);
    return true;
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

static int32 xwork_mock_emit_model_events(
    const xllm_call_options *pOptions,
    const xllm_response *pResponse
)
{
    xllm_event tEvent;
    size_t i;

    if ( !pOptions || !pOptions->pfnOnEvent || !pResponse ) {
        return XRT_NET_OK;
    }

    memset(&tEvent, 0, sizeof(tEvent));
    tEvent.eType = XLLM_EVENT_START;
    tEvent.bSynthetic = true;
    tEvent.as.tStart.sResponseId = pResponse->sId;
    tEvent.as.tStart.sModel = pResponse->sModel;
    if ( !pOptions->pfnOnEvent(&tEvent, pOptions->pUserData) ) {
        return XRT_NET_CANCELLED;
    }

    for ( i = 0u; i < pResponse->iOutputCount; ++i ) {
        const xllm_output_item *pOutput = &pResponse->pOutputs[i];

        memset(&tEvent, 0, sizeof(tEvent));
        tEvent.eType = XLLM_EVENT_OUTPUT_BEGIN;
        tEvent.bSynthetic = true;
        tEvent.uOutputIndex = (uint32)i;
        tEvent.as.tOutputBegin.eKind = pOutput->eKind;
        if ( !pOptions->pfnOnEvent(&tEvent, pOptions->pUserData) ) {
            return XRT_NET_CANCELLED;
        }

        if ( pOutput->eKind == XLLM_OUTPUT_MESSAGE &&
             pResponse->sVisibleText &&
             pResponse->sVisibleText[0] ) {
            memset(&tEvent, 0, sizeof(tEvent));
            tEvent.eType = XLLM_EVENT_TEXT_DELTA;
            tEvent.bSynthetic = true;
            tEvent.uOutputIndex = (uint32)i;
            tEvent.as.tTextDelta.sText = pResponse->sVisibleText;
            if ( !pOptions->pfnOnEvent(&tEvent, pOptions->pUserData) ) {
                return XRT_NET_CANCELLED;
            }
        } else if ( pOutput->eKind == XLLM_OUTPUT_TOOL_CALL ) {
            memset(&tEvent, 0, sizeof(tEvent));
            tEvent.eType = XLLM_EVENT_TOOL_CALL_READY;
            tEvent.bSynthetic = true;
            tEvent.uOutputIndex = (uint32)i;
            tEvent.as.tToolCallReady.tToolCall = pOutput->as.tToolCall;
            if ( !pOptions->pfnOnEvent(&tEvent, pOptions->pUserData) ) {
                return XRT_NET_CANCELLED;
            }
        }

        memset(&tEvent, 0, sizeof(tEvent));
        tEvent.eType = XLLM_EVENT_OUTPUT_END;
        tEvent.bSynthetic = true;
        tEvent.uOutputIndex = (uint32)i;
        if ( !pOptions->pfnOnEvent(&tEvent, pOptions->pUserData) ) {
            return XRT_NET_CANCELLED;
        }
    }

    memset(&tEvent, 0, sizeof(tEvent));
    tEvent.eType = XLLM_EVENT_END;
    tEvent.bSynthetic = true;
    return pOptions->pfnOnEvent(&tEvent, pOptions->pUserData)
        ? XRT_NET_OK
        : XRT_NET_CANCELLED;
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
        pState->bTerminalWriteEof = false;
        pState->bTerminalListFlow = false;
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
                    strstr(sInstruction, "Run the local process with timeout through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-timeout",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_TIMEOUT_COMMAND "\","
                "\"timeout_ms\":" XWORK_TEST_STRINGIFY(XWORK_TEST_PROCESS_TIMEOUT_MS) "}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process for async cancellation through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-async-cancel",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_ASYNC_CANCEL_COMMAND "\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process with kill-tree timeout through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-timeout-kill-tree",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_TIMEOUT_COMMAND "\","
                "\"timeout_ms\":" XWORK_TEST_STRINGIFY(XWORK_TEST_PROCESS_TIMEOUT_MS) ","
                "\"timeout_stop\":\"kill_tree\"}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process with separate stderr through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-stderr",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_STDERR_COMMAND "\","
                "\"merge_stderr\":false}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process with ordered events through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-events",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_STDERR_COMMAND "\","
                "\"merge_stderr\":false,\"include_events\":true}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local process in terminal mode through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-terminal",
                "process.exec",
                "{\"command\":\"" XWORK_TEST_PROCESS_TERMINAL_COMMAND "\","
                "\"use_terminal\":true,\"include_events\":true,"
                "\"terminal_cols\":100,\"terminal_rows\":32}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local interactive terminal session listing through host service.") != NULL ) {
            pState->bTerminalListFlow = true;
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-terminal-session-list-start",
                XWORK_TOOL_PROCESS_START_TERMINAL,
                "{\"command\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND "\","
                "\"session_name\":\"build-shell\","
                "\"terminal_cols\":100,\"terminal_rows\":32}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local interactive terminal session through host service.") != NULL ) {
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-terminal-session-start",
                XWORK_TOOL_PROCESS_START_TERMINAL,
                "{\"command\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND "\","
                "\"terminal_cols\":100,\"terminal_rows\":32}"
            );
        } else if ( sInstruction &&
                    strstr(sInstruction, "Run the local interactive terminal session with EOF through host service.") != NULL ) {
            pState->bTerminalWriteEof = true;
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-process-terminal-session-eof-start",
                XWORK_TOOL_PROCESS_START_TERMINAL,
                "{\"command\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND "\","
                "\"terminal_cols\":100,\"terminal_rows\":32}"
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
        xvalue tToolTable = NULL;
        const char *sSessionIdText = NULL;
        char *sNextArgsJson = NULL;
        size_t iParsedNextAfterSeq = 0u;
        bool bDone = false;

        if ( pRequest->iMessageCount < (bHasInlineSystemMessage ? 4u : 3u) ) {
            return XRT_NET_ERROR;
        }
        iToolIndex = pRequest->iMessageCount - 1u;
        iAssistantIndex = iToolIndex - 1u;
        if ( pRequest->pMessages[iAssistantIndex].eRole != XLLM_ROLE_ASSISTANT ||
             pRequest->pMessages[iToolIndex].eRole != XLLM_ROLE_TOOL ) {
            return XRT_NET_ERROR;
        }
        if ( pRequest->pMessages[iAssistantIndex].iToolCallCount == 0u ) {
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
        } else if ( strcmp(sToolId, XWORK_TOOL_PROCESS_START_TERMINAL) == 0 ) {
            size_t iParsedEventEndSeq = 0u;
            bool bParsedHasMoreEvents = false;
            bool bParsedEventStreamDone = false;

            if ( !xwork_test_parse_json_table(sToolText, &tToolTable) ) {
                return XRT_NET_ERROR;
            }
            sSessionIdText = xwork_test_json_get_text(tToolTable, "session_id");
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 !sSessionIdText ||
                 strstr(sToolText, "\"session_index\":") == NULL ||
                 strstr(sToolText, "\"stdin_closed\":false") == NULL ||
                 strstr(sToolText, "\"terminal_output_captured\":") == NULL ||
                 strstr(sToolText, "\"output_text\":\"") == NULL ||
                 strstr(sToolText, "\"output_bytes\":") == NULL ||
                 strstr(sToolText, "\"event_count\":") == NULL ||
                 strstr(sToolText, "\"events\":[") == NULL ||
                 !xwork_test_json_get_size(tToolTable, "next_after_seq", &iParsedNextAfterSeq) ||
                 !xwork_test_json_get_size(tToolTable, "event_end_seq", &iParsedEventEndSeq) ||
                 !xwork_test_json_get_bool(tToolTable, "has_more_events", &bParsedHasMoreEvents) ||
                 !xwork_test_json_get_bool(tToolTable, "event_stream_done", &bParsedEventStreamDone) ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            if ( pState->bTerminalListFlow &&
                 strstr(sToolText, "\"session_name\":\"build-shell\"") == NULL ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            if ( iParsedEventEndSeq < iParsedNextAfterSeq || bParsedEventStreamDone ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            if ( pState->bTerminalListFlow ) {
                xvoUnref(tToolTable);
                *ppResponse = xwork_mock_build_tool_call_response(
                    pProfile->sId,
                    "mock-response-terminal-list",
                    XWORK_TOOL_PROCESS_LIST_TERMINALS,
                    "{\"session_name\":\"build-shell\",\"running\":true,\"limit\":1}"
                );
                return 0;
            }
            sNextArgsJson = xwork_test_dup_cstr(
                "{\"terminal_cols\":140,\"terminal_rows\":40}"
            );
            free(sNextArgsJson);
            sNextArgsJson = xwork__dup_printf(
                "{\"session_id\":\"%s\",\"terminal_cols\":140,\"terminal_rows\":40}",
                sSessionIdText
            );
            xvoUnref(tToolTable);
            if ( !sNextArgsJson ) {
                return XRT_NET_ERROR;
            }
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-terminal-resize",
                XWORK_TOOL_PROCESS_TERMINAL_RESIZE,
                sNextArgsJson
            );
            free(sNextArgsJson);
        } else if ( strcmp(sToolId, XWORK_TOOL_PROCESS_TERMINAL_RESIZE) == 0 ) {
            if ( !xwork_test_parse_json_table(sToolText, &tToolTable) ) {
                return XRT_NET_ERROR;
            }
            sSessionIdText = xwork_test_json_get_text(tToolTable, "session_id");
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 !sSessionIdText ||
                 strstr(sToolText, "\"session_index\":") == NULL ||
                 strstr(sToolText, "\"stdin_closed\":false") == NULL ||
                 strstr(sToolText, "\"resize_applied\":") == NULL ||
                 strstr(sToolText, "\"terminal_cols\":140") == NULL ||
                 strstr(sToolText, "\"terminal_rows\":40") == NULL ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            sNextArgsJson = xwork__dup_printf(
                pState->bTerminalWriteEof
                    ? (iParsedNextAfterSeq > 0u
                        ? "{\"session_id\":\"%s\",\"input_text\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "\","
                          "\"include_state\":true,\"write_eof\":true,\"after_seq\":%zu}"
                        : "{\"session_id\":\"%s\",\"input_text\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "\","
                          "\"include_state\":true,\"write_eof\":true}")
                    : (iParsedNextAfterSeq > 0u
                        ? "{\"session_id\":\"%s\",\"input_text\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "\","
                          "\"include_state\":true,\"after_seq\":%zu}"
                        : "{\"session_id\":\"%s\",\"input_text\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "\","
                          "\"include_state\":true}"),
                sSessionIdText,
                iParsedNextAfterSeq
            );
            xvoUnref(tToolTable);
            if ( !sNextArgsJson ) {
                return XRT_NET_ERROR;
            }
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-terminal-write",
                XWORK_TOOL_PROCESS_TERMINAL_WRITE,
                sNextArgsJson
            );
            free(sNextArgsJson);
        } else if ( strcmp(sToolId, XWORK_TOOL_PROCESS_LIST_TERMINALS) == 0 ) {
            xvalue tSessions = NULL;
            xvalue tSessionItem = NULL;

            if ( !xwork_test_parse_json_table(sToolText, &tToolTable) ) {
                return XRT_NET_ERROR;
            }
            tSessions = xwork_test_json_get_value(tToolTable, "sessions");
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 strstr(sToolText, "\"session_count\":") == NULL ||
                 strstr(sToolText, "\"total_session_count\":") == NULL ||
                 strstr(sToolText, "\"sort\":\"session_index_asc\"") == NULL ||
                 strstr(sToolText, "\"has_more_sessions\":false") == NULL ||
                 strstr(sToolText, "\"next_after_session_index\":") == NULL ||
                 strstr(sToolText, "\"filters\":{\"session_name\":\"build-shell\",\"running\":true,\"done\":null,\"after_session_index\":null,\"limit\":1}") == NULL ||
                 !tSessions ||
                 xvoType(tSessions) != XVO_DT_ARRAY ||
                 xvoArrayItemCount(tSessions) == 0u ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            tSessionItem = xvoArrayGetValue(tSessions, 0u);
            if ( !tSessionItem || xvoType(tSessionItem) != XVO_DT_TABLE ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            sSessionIdText = xwork_test_json_get_text(tSessionItem, "session_id");
            if ( !sSessionIdText ||
                 strcmp(xwork_test_json_get_text(tSessionItem, "session_name"), "build-shell") != 0 ||
                 !xwork_test_json_get_bool(tSessionItem, "running", &bDone) ||
                 !bDone ||
                 strstr(sToolText, "\"stdin_closed\":false") == NULL ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            sNextArgsJson = xwork__dup_printf(
                "{\"session_id\":\"%s\"}",
                sSessionIdText
            );
            xvoUnref(tToolTable);
            if ( !sNextArgsJson ) {
                return XRT_NET_ERROR;
            }
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-terminal-stop-after-list",
                XWORK_TOOL_PROCESS_TERMINAL_STOP,
                sNextArgsJson
            );
            free(sNextArgsJson);
        } else if ( strcmp(sToolId, XWORK_TOOL_PROCESS_TERMINAL_WRITE) == 0 ) {
            size_t iParsedNextAfterSeq = 0u;
            size_t iParsedEventEndSeq = 0u;
            bool bParsedHasMoreEvents = false;
            bool bParsedEventStreamDone = false;

            if ( !xwork_test_parse_json_table(sToolText, &tToolTable) ) {
                return XRT_NET_ERROR;
            }
            sSessionIdText = xwork_test_json_get_text(tToolTable, "session_id");
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 !sSessionIdText ||
                 strstr(sToolText, "\"session_index\":") == NULL ||
                 strstr(sToolText, "\"bytes_written\":") == NULL ||
                 strstr(sToolText, pState->bTerminalWriteEof ? "\"write_eof\":true" : "\"write_eof\":false") == NULL ||
                 strstr(sToolText, pState->bTerminalWriteEof ? "\"stdin_closed\":true" : "\"stdin_closed\":false") == NULL ||
                 strstr(sToolText, "\"output_text\":\"") == NULL ||
                 strstr(sToolText, "\"output_bytes\":") == NULL ||
                 strstr(sToolText, "\"event_count\":") == NULL ||
                 strstr(sToolText, "\"events\":[") == NULL ||
                 !xwork_test_json_get_size(tToolTable, "next_after_seq", &iParsedNextAfterSeq) ||
                 !xwork_test_json_get_size(tToolTable, "event_end_seq", &iParsedEventEndSeq) ||
                 !xwork_test_json_get_bool(tToolTable, "has_more_events", &bParsedHasMoreEvents) ||
                 !xwork_test_json_get_bool(tToolTable, "event_stream_done", &bParsedEventStreamDone) ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            if ( iParsedEventEndSeq < iParsedNextAfterSeq ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            sNextArgsJson = xwork__dup_printf(
                "{\"session_id\":\"%s\",\"after_seq\":%zu}",
                sSessionIdText,
                iParsedNextAfterSeq
            );
            xvoUnref(tToolTable);
            if ( !sNextArgsJson ) {
                return XRT_NET_ERROR;
            }
            *ppResponse = xwork_mock_build_tool_call_response(
                pProfile->sId,
                "mock-response-terminal-read",
                XWORK_TOOL_PROCESS_TERMINAL_READ,
                sNextArgsJson
            );
            free(sNextArgsJson);
        } else if ( strcmp(sToolId, XWORK_TOOL_PROCESS_TERMINAL_READ) == 0 ) {
            size_t iParsedNextAfterSeq = 0u;
            size_t iParsedEventEndSeq = 0u;
            bool bParsedHasMoreEvents = false;
            bool bParsedEventStreamDone = false;

            if ( !xwork_test_parse_json_table(sToolText, &tToolTable) ) {
                return XRT_NET_ERROR;
            }
            sSessionIdText = xwork_test_json_get_text(tToolTable, "session_id");
            bDone = xwork_test_json_get_bool(tToolTable, "done", &bDone) && bDone;
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 !sSessionIdText ||
                 strstr(sToolText, "\"session_index\":") == NULL ||
                 strstr(sToolText, pState->bTerminalWriteEof ? "\"stdin_closed\":true" : "\"stdin_closed\":false") == NULL ||
                 strstr(sToolText, "\"terminal_output_captured\":") == NULL ||
                 strstr(sToolText, "\"output_text\":\"") == NULL ||
                 strstr(sToolText, "\"output_bytes\":") == NULL ||
                 strstr(sToolText, "\"event_count\":") == NULL ||
                 strstr(sToolText, "\"events\":[") == NULL ||
                 !xwork_test_json_get_size(tToolTable, "next_after_seq", &iParsedNextAfterSeq) ||
                 !xwork_test_json_get_size(tToolTable, "event_end_seq", &iParsedEventEndSeq) ||
                 !xwork_test_json_get_bool(tToolTable, "has_more_events", &bParsedHasMoreEvents) ||
                 !xwork_test_json_get_bool(tToolTable, "event_stream_done", &bParsedEventStreamDone) ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            if ( iParsedEventEndSeq < iParsedNextAfterSeq ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            if ( bDone && !bParsedEventStreamDone ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            if ( bDone && strstr(sToolText, "\"kind\":\"exit\"") == NULL ) {
                xvoUnref(tToolTable);
                return XRT_NET_ERROR;
            }
            if ( pState->bTerminalWriteEof ) {
                xvoUnref(tToolTable);
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service terminal EOF session completed."
                );
            } else {
                sNextArgsJson = xwork__dup_printf(
                    "{\"session_id\":\"%s\"}",
                    sSessionIdText
                );
                xvoUnref(tToolTable);
                if ( !sNextArgsJson ) {
                    return XRT_NET_ERROR;
                }
                *ppResponse = xwork_mock_build_tool_call_response(
                    pProfile->sId,
                    "mock-response-terminal-stop",
                    XWORK_TOOL_PROCESS_TERMINAL_STOP,
                    sNextArgsJson
                );
                free(sNextArgsJson);
            }
        } else if ( strcmp(sToolId, XWORK_TOOL_PROCESS_TERMINAL_STOP) == 0 ) {
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 strstr(sToolText, "\"session_index\":") == NULL ||
                 strstr(sToolText, "\"terminal_output_captured\":") == NULL ||
                 strstr(sToolText, "\"output_text\":\"") == NULL ||
                 strstr(sToolText, "\"output_bytes\":") == NULL ||
                 strstr(sToolText, "\"event_end_seq\":") == NULL ||
                 strstr(sToolText, "\"has_more_events\":") == NULL ||
                 strstr(sToolText, "\"event_stream_done\":true") == NULL ||
                 strstr(sToolText, "\"removed\":true") == NULL ) {
                return XRT_NET_ERROR;
            }
            *ppResponse = xwork_mock_build_final_response(
                pProfile->sId,
                pState->bTerminalWriteEof
                    ? "Host service terminal EOF session completed."
                    : (pState->bTerminalListFlow
                        ? "Host service terminal list completed."
                        : "Host service terminal session completed.")
            );
        } else if ( strcmp(sToolId, "process.exec") == 0 ) {
            if ( strstr(sToolText, "\"ok\":true") == NULL ||
                 strstr(sToolText, "\"stdout\":\"") == NULL ) {
                return XRT_NET_ERROR;
            }
            if ( strstr(sToolText, "\"use_terminal\":true") != NULL ) {
                if ( strstr(sToolText, "\"include_events\":true") == NULL ||
                     strstr(sToolText, "\"terminal_output_captured\":") == NULL ||
                     strstr(sToolText, "\"terminal_cols\":100") == NULL ||
                     strstr(sToolText, "\"terminal_rows\":32") == NULL ||
                     strstr(sToolText, "\"event_count\":") == NULL ||
                     strstr(sToolText, "\"events\":[") == NULL ||
                     strstr(sToolText, "\"kind\":\"start\"") == NULL ||
                     strstr(sToolText, "\"kind\":\"exit\"") == NULL ) {
                    return XRT_NET_ERROR;
                }
                if ( strstr(sToolText, "\"terminal_output_captured\":true") != NULL ) {
                    if ( strstr(sToolText, "\"kind\":\"output\"") == NULL ||
                         strstr(sToolText, "\"stream\":\"terminal\"") == NULL ||
                         strstr(sToolText, XWORK_TEST_PROCESS_TERMINAL_EXPECTED) == NULL ) {
                        return XRT_NET_ERROR;
                    }
                }
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service process terminal completed."
                );
            } else if ( strstr(sToolText, "\"include_events\":true") != NULL ) {
                if ( strstr(sToolText, "\"event_count\":") == NULL ||
                     strstr(sToolText, "\"events\":[") == NULL ||
                     strstr(sToolText, "\"kind\":\"start\"") == NULL ||
                     strstr(sToolText, "\"kind\":\"output\"") == NULL ||
                     strstr(sToolText, "\"kind\":\"exit\"") == NULL ||
                     strstr(sToolText, "\"stream\":\"stdout\"") == NULL ||
                     strstr(sToolText, "\"stream\":\"stderr\"") == NULL ||
                     strstr(sToolText, XWORK_TEST_PROCESS_STDOUT_EXPECTED) == NULL ||
                     strstr(sToolText, XWORK_TEST_PROCESS_STDERR_EXPECTED) == NULL ) {
                    return XRT_NET_ERROR;
                }
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service process events completed."
                );
            } else if ( strstr(sToolText, XWORK_TEST_PROCESS_STDERR_EXPECTED) != NULL ) {
                if ( strstr(sToolText, "\"merge_stderr\":false") == NULL ||
                     strstr(sToolText, "\"stderr\":\"") == NULL ||
                     strstr(sToolText, XWORK_TEST_PROCESS_STDOUT_EXPECTED) == NULL ) {
                    return XRT_NET_ERROR;
                }
                *ppResponse = xwork_mock_build_final_response(
                    pProfile->sId,
                    "Host service process stderr completed."
                );
            } else if ( strstr(sToolText, "\"truncated\":true") != NULL ) {
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
    if ( xwork_mock_emit_model_events(pOptions, *ppResponse) != XRT_NET_OK ) {
        xllm_response_free(*ppResponse);
        *ppResponse = NULL;
        return XRT_NET_CANCELLED;
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

static bool xwork_mock_interrupt_check(
    const xwork_run *pRun,
    const char *sPhase,
    void *pUserData
)
{
    xwork_mock_interrupt_ctx *pCtx = (xwork_mock_interrupt_ctx *)pUserData;

    (void)pRun;

    if ( !pCtx ) {
        return false;
    }
    ++pCtx->iCheckCount;
    return pCtx->sInterruptPhase &&
           sPhase &&
           strcmp(pCtx->sInterruptPhase, sPhase) == 0;
}

static bool xwork_mock_model_event(
    xwork_run *pRun,
    const xwork_model_event *pEvent,
    void *pUserData
)
{
    xwork_mock_model_event_ctx *pCtx = (xwork_mock_model_event_ctx *)pUserData;

    (void)pRun;

    if ( !pCtx || !pEvent ) {
        return true;
    }
    ++pCtx->iEventCount;
    if ( pEvent->eType == XLLM_EVENT_TEXT_DELTA ) {
        ++pCtx->iTextDeltaCount;
        return !pCtx->bCancelOnTextDelta;
    }
    return true;
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
    if ( iStatus == XWORK_OK ) {
        xwork_test_assert_readme_patch_stats(&tArtifact);
    }
    xwork_artifact_reset(&tArtifact);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    pResult->sOutputText = "{\"ok\":true,\"changed_files\":[\"README.md\"]}";
    pResult->sVisibleSummary = "mock.apply_patch executed successfully.";
    return XWORK_OK;
}

static xwork_status xwork_mock_tool_exec_ex(
    xwork_run *pRun,
    const xwork_tool_call *pCall,
    const xwork_tool_exec_context *pContext,
    xwork_tool_result *pResult,
    void *pUserData
)
{
    xwork_mock_tool_exec_ctx *pCtx = (xwork_mock_tool_exec_ctx *)pUserData;

    if ( pCtx ) {
        ++pCtx->iExecExCount;
        ++pCtx->iCancelCheckCount;
    }
    if ( xwork_tool_exec_context_should_cancel(pRun, pContext, "mock_tool_exec") ) {
        pResult->sOutputText = "{\"ok\":false,\"cancelled\":true}";
        pResult->sVisibleSummary = "mock.apply_patch cancelled.";
        return XWORK_ERROR_CANCELLED;
    }

    return xwork_mock_tool_exec(pRun, pCall, pResult, pUserData);
}

static xwork_status xwork_mock_slow_tool_exec_ex(
    xwork_run *pRun,
    const xwork_tool_call *pCall,
    const xwork_tool_exec_context *pContext,
    xwork_tool_result *pResult,
    void *pUserData
)
{
    xwork_mock_tool_exec_ctx *pCtx = (xwork_mock_tool_exec_ctx *)pUserData;
    int i;

    if ( pCtx ) {
        ++pCtx->iExecExCount;
    }
    for ( i = 0; i < 100; ++i ) {
        if ( pCtx ) {
            ++pCtx->iCancelCheckCount;
        }
        if ( xwork_tool_exec_context_should_cancel(pRun, pContext, "mock_slow_tool_exec") ) {
            pResult->sOutputText = "{\"ok\":false,\"cancelled\":true}";
            pResult->sVisibleSummary = "mock.apply_patch cancelled.";
            return XWORK_ERROR_CANCELLED;
        }
        xrtSleep(5u);
    }

    return xwork_mock_tool_exec(pRun, pCall, pResult, pUserData);
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
    xwork_mock_tool_exec_ctx tStreamToolExecCtx;
    xwork_mock_tool_exec_ctx tDefaultMemoryToolExecCtx;
    xwork_mock_host_ctx tHostCtx;
    xwork_mock_memory_ctx tMemoryCtx;
    xwork_mock_interrupt_ctx tInterruptCtx;
    xwork_mock_model_event_ctx tModelEventCtx;
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
    xwork_run *pInterruptedRun = NULL;
    xwork_run *pStreamRun = NULL;
    xwork_run *pStreamCancelledRun = NULL;
    xwork_run *pAsyncRun = NULL;
    xwork_run_async *pAsync = NULL;
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
    xwork_run *pFileArtifactQueryRun = NULL;
    xwork_run *pLocalHostRun = NULL;
    xwork_orchestrator_options tExecOptions;
    xwork_orchestrator_options tPendingExecOptions;
    xwork_report_artifact_options tReportArtifactOptions;
    xwork_command_artifact_options tCommandArtifactOptions;
    xwork_approval_request tApproval;
    xwork_approval_request tPendingApproval;
    xwork_checkpoint tCheckpoint;
    xwork_checkpoint tPendingCheckpoint;
    xwork_artifact tArtifact;
    xwork_artifact tLoadedArtifact;
    xwork_run_summary tLoadedSummary;
    xwork_run_summary_list tPersistedRunSummaries;
    xwork_artifact_summary_list tPersistedArtifactSummaries;
    xwork_artifact_summary_query tArtifactSummaryQuery;
    xwork_workspace_memory_sync_summary tMemorySyncSummary;
    xwork_workspace_memory_file_sync_summary tMemoryFileSyncSummary;
    xwork_run_index_list tPersistedRunIndex;
    xwork_run_index_query tRunIndexQuery;
    xwork_run_snapshot tRunSnapshot;
    xwork_event tEvent;
    xwork_event tLoadedEvent;
    xwork_event tPendingEvent;
    xwork_memory_context tObservedMemoryContext;
    xwork_host_invoke_context tHostInvokeContext;
    xwork_tool_result tHostResult;
    xwork_tool_result tLocalHostResult;
    xwork_status iHostCancelStatus;
    xwork_status iAsyncStatus;
    xwork_string_list tPersistedRunIds;
    xwork_string_list tPersistedEventIds;
    xwork_string_list tPersistedCheckpointIds;
    xwork_string_list tPersistedArtifactIds;
    char *sAfterToolCheckpointId = NULL;
    char *sPendingBeforeToolCheckpointId = NULL;
    char *sFileBeforeToolCheckpointId = NULL;
    char *sFirstArtifactId = NULL;
    bool bTerminalOutputCaptured = false;
    bool bAsyncCompleted = false;
    size_t iSavedMaxProcessInputBytes = 0u;
    size_t iSavedMaxProcessEnvEntries = 0u;
    int iWaitPoll;
    char sFilePersistenceRoot[256];
    char sTerminalStorageRefPrefix[256];
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
    const char *sMemorySyncWorkspaceDir = "tests/memory_sync_workspace";
    const char *sMemorySyncWorkspaceFile = "tests/memory_sync_workspace/sync_note.md";

    memset(&tAdapterCtx, 0, sizeof(tAdapterCtx));
    memset(&tToolExecCtx, 0, sizeof(tToolExecCtx));
    memset(&tStreamToolExecCtx, 0, sizeof(tStreamToolExecCtx));
    memset(&tDefaultMemoryToolExecCtx, 0, sizeof(tDefaultMemoryToolExecCtx));
    memset(&tHostCtx, 0, sizeof(tHostCtx));
    memset(&tMemoryCtx, 0, sizeof(tMemoryCtx));
    memset(&tInterruptCtx, 0, sizeof(tInterruptCtx));
    memset(&tModelEventCtx, 0, sizeof(tModelEventCtx));
    memset(&tPersistenceCtx, 0, sizeof(tPersistenceCtx));
    xwork_local_host_options_init(&tLocalHostOptions);
    xwork_local_host_init(&tLocalHost);
    xwork_file_persistence_init(&tFilePersistenceStore);
    xwork_file_persistence_init(&tFilePersistenceRecoverStore);
    xwork_string_list_init(&tPersistedRunIds);
    xwork_string_list_init(&tPersistedEventIds);
    xwork_string_list_init(&tPersistedCheckpointIds);
    xwork_string_list_init(&tPersistedArtifactIds);
    xwork_report_artifact_options_init(&tReportArtifactOptions);
    xwork_command_artifact_options_init(&tCommandArtifactOptions);
    xwork_artifact_init(&tArtifact);
    xwork_artifact_init(&tLoadedArtifact);
    xwork_run_summary_init(&tLoadedSummary);
    xwork_run_summary_list_init(&tPersistedRunSummaries);
    xwork_artifact_summary_list_init(&tPersistedArtifactSummaries);
    xwork_artifact_summary_query_init(&tArtifactSummaryQuery);
    xwork_workspace_memory_sync_summary_init(&tMemorySyncSummary);
    xwork_workspace_memory_file_sync_summary_init(&tMemoryFileSyncSummary);
    xwork_run_index_list_init(&tPersistedRunIndex);
    xwork_run_index_query_init(&tRunIndexQuery);
    xwork_event_init(&tLoadedEvent);
    memset(&tHostInvokeContext, 0, sizeof(tHostInvokeContext));
    xwork_run_snapshot_init(&tPersistenceCtx.tSnapshot);
    xwork_memory_context_init(&tObservedMemoryContext);
    xwork_tool_result_init(&tLocalHostResult);
    sTerminalStorageRefPrefix[0] = '\0';
    (void)remove(sLocalHostMissingPath);
    (void)remove(sLocalHostWritePath);
    (void)remove(sLocalHostAppendPath);
    (void)remove(sLocalHostCreatePath);
    (void)remove(sLocalHostCreateDirsPath);
    (void)remove(sLocalHostOrchestratorWritePath);
    (void)remove(sLocalHostOrchestratorAppendPath);
    (void)remove(sLocalHostOrchestratorCreatePath);
    (void)remove(sLocalHostOrchestratorCreateDirsPath);
    (void)remove(sMemorySyncWorkspaceFile);
    xwork_test_remove_empty_directory(sLocalHostCreateDirsDir);
    xwork_test_remove_empty_directory(sLocalHostOrchestratorCreateDirsDir);
    xwork_test_remove_empty_directory(sLocalHostCreateDirsParentDir);
    xwork_test_remove_empty_directory(sMemorySyncWorkspaceDir);

    assert(XWORK_TEST_MKDIR(sMemorySyncWorkspaceDir) == 0);
    {
        FILE *pSyncFile = fopen(sMemorySyncWorkspaceFile, "wb");

        assert(pSyncFile != NULL);
        assert(
            fputs(
                "xwork-memory-sync-keyword: workspace memory sync file content.\n",
                pSyncFile
            ) >= 0
        );
        assert(fclose(pSyncFile) == 0);
    }

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
    tWorkspaceOptions.sRootPath = sMemorySyncWorkspaceDir;
    tWorkspaceOptions.pMemory = pWorkspaceMemory;
    assert(xwork_runtime_add_workspace(pRuntime, &tWorkspaceOptions, &pMemoryWorkspace) == XWORK_OK);
    assert(pMemoryWorkspace != NULL);
    assert(xwork_workspace_sync_memory(pWorkspace, &tMemorySyncSummary) == XWORK_ERROR_INVALID_ARGUMENT);
    assert(xwork_workspace_sync_memory(pMemoryWorkspace, &tMemorySyncSummary) == XWORK_OK);
    assert(tMemorySyncSummary.iVisitedFileCount >= 1u);
    assert(tMemorySyncSummary.iIngestedFileCount >= 1u);
    assert(tMemorySyncSummary.iCreatedRecordCount >= 1u);
    assert(tMemorySyncSummary.iFailedFileCount == 0u);
    {
        xllm_memory_search_options tMemorySearchOptions;
        xllm_memory_search_result tMemorySearchResult;
        xllm_error tMemoryError;

        xllm_memory_search_options_init(&tMemorySearchOptions);
        memset(&tMemorySearchResult, 0, sizeof(tMemorySearchResult));
        xllm_error_init(&tMemoryError);

        tMemorySearchOptions.sQuery = "xwork-memory-sync-keyword";
        assert(
            xllm_memory_search(
                pWorkspaceMemory,
                &tMemorySearchOptions,
                &tMemorySearchResult,
                &tMemoryError
            ) == XRT_NET_OK
        );
        assert(tMemorySearchResult.iHitCount > 0u);
        assert(tMemorySearchResult.pHits != NULL);
        assert(strstr(tMemorySearchResult.pHits[0].sText, "workspace memory sync file") != NULL);
        xllm_memory_search_result_reset(&tMemorySearchResult);
        xllm_error_free(&tMemoryError);
    }
    {
        FILE *pSyncFile = fopen(sMemorySyncWorkspaceFile, "wb");

        assert(pSyncFile != NULL);
        assert(
            fputs(
                "xwork-memory-sync-updated-keyword: updated workspace memory sync file content.\n",
                pSyncFile
            ) >= 0
        );
        assert(fclose(pSyncFile) == 0);
    }
    assert(
        xwork_workspace_sync_memory_file(
            pMemoryWorkspace,
            sMemorySyncWorkspaceFile,
            &tMemoryFileSyncSummary
        ) == XWORK_OK
    );
    assert(tMemoryFileSyncSummary.iChangeCount >= 1u);
    assert((tMemoryFileSyncSummary.iCreatedCount + tMemoryFileSyncSummary.iUpdatedCount) >= 1u);
    assert(tMemoryFileSyncSummary.iFailedCount == 0u);
    {
        xllm_memory_search_options tMemorySearchOptions;
        xllm_memory_search_result tMemorySearchResult;
        xllm_error tMemoryError;

        xllm_memory_search_options_init(&tMemorySearchOptions);
        memset(&tMemorySearchResult, 0, sizeof(tMemorySearchResult));
        xllm_error_init(&tMemoryError);

        tMemorySearchOptions.sQuery = "xwork-memory-sync-updated-keyword";
        assert(
            xllm_memory_search(
                pWorkspaceMemory,
                &tMemorySearchOptions,
                &tMemorySearchResult,
                &tMemoryError
            ) == XRT_NET_OK
        );
        assert(tMemorySearchResult.iHitCount > 0u);
        assert(tMemorySearchResult.pHits != NULL);
        assert(strstr(tMemorySearchResult.pHits[0].sText, "updated workspace memory sync file") != NULL);
        xllm_memory_search_result_reset(&tMemorySearchResult);
        xllm_error_free(&tMemoryError);
    }

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
    pBuiltinToolDef = xwork_get_builtin_tool_def(XWORK_TOOL_PROCESS_START_TERMINAL);
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_PROCESS);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_PROCESS_START_TERMINAL) == 0);
    pBuiltinToolDef = xwork_get_builtin_tool_def(XWORK_TOOL_PROCESS_LIST_TERMINALS);
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_PROCESS);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_PROCESS_LIST_TERMINALS) == 0);
    pBuiltinToolDef = xwork_get_builtin_tool_def(XWORK_TOOL_PROCESS_TERMINAL_READ);
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_PROCESS);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_PROCESS_TERMINAL_READ) == 0);
    pBuiltinToolDef = xwork_get_builtin_tool_def(XWORK_TOOL_PROCESS_TERMINAL_WRITE);
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_PROCESS);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_PROCESS_TERMINAL_WRITE) == 0);
    pBuiltinToolDef = xwork_get_builtin_tool_def(XWORK_TOOL_PROCESS_TERMINAL_RESIZE);
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_PROCESS);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_PROCESS_TERMINAL_RESIZE) == 0);
    pBuiltinToolDef = xwork_get_builtin_tool_def(XWORK_TOOL_PROCESS_TERMINAL_STOP);
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_PROCESS);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_PROCESS_TERMINAL_STOP) == 0);
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
    xwork_test_assert_readme_patch_stats(&tArtifact);
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
    memset(&tInterruptCtx, 0, sizeof(tInterruptCtx));
    tInterruptCtx.sInterruptPhase = "before_model";
    asWorkspaceIds[0] = "main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-interrupted-before-model";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pInterruptedRun) == XWORK_OK);
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tExecOptions.pUserData = &tStreamToolExecCtx;
    tExecOptions.pfnShouldInterrupt = xwork_mock_interrupt_check;
    tExecOptions.pInterruptUserData = &tInterruptCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pInterruptedRun, &tExecOptions) == XWORK_ERROR_CANCELLED);
    assert(xwork_run_get_state(pInterruptedRun) == XWORK_RUN_CANCELLED);
    assert(tAdapterCtx.iTurnCount == 0);
    assert(tInterruptCtx.iCheckCount > 0);
    xwork_checkpoint_init(&tCheckpoint);
    assert(xwork_run_get_last_checkpoint(pInterruptedRun, &tCheckpoint) == XWORK_OK);
    assert(tCheckpoint.eKind == XWORK_CHECKPOINT_COMPLETION);
    assert(tCheckpoint.eRunState == XWORK_RUN_CANCELLED);
    xwork_event_init(&tEvent);
    assert(xwork_run_get_last_event(pInterruptedRun, &tEvent) == XWORK_OK);
    assert(tEvent.eKind == XWORK_EVENT_RUN_CANCELLED);
    xwork_run_destroy(pInterruptedRun);
    pInterruptedRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    memset(&tInterruptCtx, 0, sizeof(tInterruptCtx));
    tInterruptCtx.sInterruptPhase = "mock_tool_exec";
    memset(&tStreamToolExecCtx, 0, sizeof(tStreamToolExecCtx));
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-tool-exec-ex-cancelled";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pInterruptedRun) == XWORK_OK);
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExecEx = xwork_mock_tool_exec_ex;
    tExecOptions.pUserData = &tStreamToolExecCtx;
    tExecOptions.pfnShouldInterrupt = xwork_mock_interrupt_check;
    tExecOptions.pInterruptUserData = &tInterruptCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pInterruptedRun, &tExecOptions) == XWORK_ERROR_CANCELLED);
    assert(xwork_run_get_state(pInterruptedRun) == XWORK_RUN_CANCELLED);
    assert(tAdapterCtx.iTurnCount == 1);
    assert(tStreamToolExecCtx.iExecExCount == 1);
    assert(tStreamToolExecCtx.iExecCount == 0);
    assert(tStreamToolExecCtx.iCancelCheckCount == 1);
    assert(tInterruptCtx.iCheckCount > 0);
    xwork_checkpoint_init(&tCheckpoint);
    assert(xwork_run_get_last_checkpoint(pInterruptedRun, &tCheckpoint) == XWORK_OK);
    assert(tCheckpoint.eRunState == XWORK_RUN_CANCELLED);
    xwork_event_init(&tEvent);
    assert(xwork_run_get_last_event(pInterruptedRun, &tEvent) == XWORK_OK);
    assert(tEvent.eKind == XWORK_EVENT_RUN_CANCELLED);
    xwork_run_destroy(pInterruptedRun);
    pInterruptedRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    memset(&tStreamToolExecCtx, 0, sizeof(tStreamToolExecCtx));
    memset(&tModelEventCtx, 0, sizeof(tModelEventCtx));
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-stream-events";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pStreamRun) == XWORK_OK);
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tExecOptions.pUserData = &tStreamToolExecCtx;
    tExecOptions.eModelStreamMode = XWORK_MODEL_STREAM_PREFER;
    tExecOptions.pfnModelEvent = xwork_mock_model_event;
    tExecOptions.pModelEventUserData = &tModelEventCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pStreamRun, &tExecOptions) == XWORK_OK);
    assert(xwork_run_get_state(pStreamRun) == XWORK_RUN_COMPLETED);
    assert(tModelEventCtx.iEventCount > 0);
    assert(tModelEventCtx.iTextDeltaCount > 0);
    xwork_run_destroy(pStreamRun);
    pStreamRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    memset(&tModelEventCtx, 0, sizeof(tModelEventCtx));
    tModelEventCtx.bCancelOnTextDelta = true;
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-stream-cancelled";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pStreamCancelledRun) == XWORK_OK);
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tExecOptions.pUserData = &tStreamToolExecCtx;
    tExecOptions.eModelStreamMode = XWORK_MODEL_STREAM_PREFER;
    tExecOptions.pfnModelEvent = xwork_mock_model_event;
    tExecOptions.pModelEventUserData = &tModelEventCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute(pStreamCancelledRun, &tExecOptions) == XWORK_ERROR_CANCELLED);
    assert(xwork_run_get_state(pStreamCancelledRun) == XWORK_RUN_CANCELLED);
    assert(tModelEventCtx.iTextDeltaCount == 1);
    xwork_checkpoint_init(&tCheckpoint);
    assert(xwork_run_get_last_checkpoint(pStreamCancelledRun, &tCheckpoint) == XWORK_OK);
    assert(tCheckpoint.eRunState == XWORK_RUN_CANCELLED);
    xwork_event_init(&tEvent);
    assert(xwork_run_get_last_event(pStreamCancelledRun, &tEvent) == XWORK_OK);
    assert(tEvent.eKind == XWORK_EVENT_RUN_CANCELLED);
    xwork_run_destroy(pStreamCancelledRun);
    pStreamCancelledRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    memset(&tStreamToolExecCtx, 0, sizeof(tStreamToolExecCtx));
    asWorkspaceIds[0] = "main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-async-complete";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pAsyncRun) == XWORK_OK);
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExec = xwork_mock_tool_exec;
    tExecOptions.pUserData = &tStreamToolExecCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute_async(pAsyncRun, &tExecOptions, &pAsync) == XWORK_OK);
    assert(xwork_run_async_wait_timeout(pAsync, 0u, &bAsyncCompleted) == XWORK_OK);
    assert(xwork_run_async_wait(pAsync) == XWORK_OK);
    assert(xwork_run_async_get_status(pAsync, &iAsyncStatus, &bAsyncCompleted) == XWORK_OK);
    assert(bAsyncCompleted);
    assert(iAsyncStatus == XWORK_OK);
    assert(xwork_run_get_state(pAsyncRun) == XWORK_RUN_COMPLETED);
    assert(strcmp(xwork_run_get_last_output_text(pAsyncRun), "Mock model completed after tool result.") == 0);
    assert(tStreamToolExecCtx.iExecCount == 1);
    xwork_run_async_destroy(pAsync);
    pAsync = NULL;
    xwork_run_destroy(pAsyncRun);
    pAsyncRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    memset(&tStreamToolExecCtx, 0, sizeof(tStreamToolExecCtx));
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-async-cancel";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pAsyncRun) == XWORK_OK);
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExecEx = xwork_mock_slow_tool_exec_ex;
    tExecOptions.pUserData = &tStreamToolExecCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute_async(pAsyncRun, &tExecOptions, &pAsync) == XWORK_OK);
    for ( iWaitPoll = 0; iWaitPoll < 100 && tStreamToolExecCtx.iExecExCount == 0; ++iWaitPoll ) {
        xrtSleep(5u);
    }
    assert(tStreamToolExecCtx.iExecExCount == 1);
    assert(xwork_run_async_wait_timeout(pAsync, 1u, &bAsyncCompleted) == XWORK_OK);
    assert(!bAsyncCompleted);
    assert(xwork_run_async_get_status(pAsync, &iAsyncStatus, &bAsyncCompleted) == XWORK_OK);
    assert(!bAsyncCompleted);
    assert(xwork_run_execute(pAsyncRun, &tExecOptions) == XWORK_ERROR_INVALID_STATE);
    assert(xwork_run_async_cancel(pAsync, "async smoke cancel") == XWORK_OK);
    assert(xwork_run_async_wait(pAsync) == XWORK_ERROR_CANCELLED);
    assert(xwork_run_async_get_status(pAsync, &iAsyncStatus, &bAsyncCompleted) == XWORK_OK);
    assert(bAsyncCompleted);
    assert(iAsyncStatus == XWORK_ERROR_CANCELLED);
    assert(xwork_run_get_state(pAsyncRun) == XWORK_RUN_CANCELLED);
    xwork_run_async_destroy(pAsync);
    pAsync = NULL;
    xwork_run_destroy(pAsyncRun);
    pAsyncRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    memset(&tStreamToolExecCtx, 0, sizeof(tStreamToolExecCtx));
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-async-destroy-cancel";
    tRunOptions.sInstruction = "Append a line to the README if needed.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(xwork_run_create(pRuntime, &tRunOptions, &pAsyncRun) == XWORK_OK);
    xwork_orchestrator_options_init(&tExecOptions);
    tExecOptions.pfnToolExecEx = xwork_mock_slow_tool_exec_ex;
    tExecOptions.pUserData = &tStreamToolExecCtx;
    tExecOptions.iMaxTurns = 3u;
    tExecOptions.bAutoApprove = true;
    assert(xwork_run_execute_async(pAsyncRun, &tExecOptions, &pAsync) == XWORK_OK);
    for ( iWaitPoll = 0; iWaitPoll < 100 && tStreamToolExecCtx.iExecExCount == 0; ++iWaitPoll ) {
        xrtSleep(5u);
    }
    assert(tStreamToolExecCtx.iExecExCount == 1);
    xwork_run_async_destroy(pAsync);
    pAsync = NULL;
    assert(xwork_run_get_state(pAsyncRun) == XWORK_RUN_CANCELLED);
    xwork_run_destroy(pAsyncRun);
    pAsyncRun = NULL;

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
            XWORK_TOOL_PROCESS_START_TERMINAL
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_register_builtin_tool(
            pLocalHostRuntime,
            XWORK_TOOL_PROCESS_LIST_TERMINALS
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_register_builtin_tool(
            pLocalHostRuntime,
            XWORK_TOOL_PROCESS_TERMINAL_READ
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_register_builtin_tool(
            pLocalHostRuntime,
            XWORK_TOOL_PROCESS_TERMINAL_WRITE
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_register_builtin_tool(
            pLocalHostRuntime,
            XWORK_TOOL_PROCESS_TERMINAL_RESIZE
        ) == XWORK_OK
    );
    assert(
        xwork_runtime_register_builtin_tool(
            pLocalHostRuntime,
            XWORK_TOOL_PROCESS_TERMINAL_STOP
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
    pBuiltinToolDef = xwork_runtime_find_tool(
        pLocalHostRuntime,
        XWORK_TOOL_PROCESS_START_TERMINAL
    );
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_PROCESS);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_PROCESS_START_TERMINAL) == 0);
    pBuiltinToolDef = xwork_runtime_find_tool(
        pLocalHostRuntime,
        XWORK_TOOL_PROCESS_LIST_TERMINALS
    );
    assert(pBuiltinToolDef != NULL);
    assert(pBuiltinToolDef->eHostService == XWORK_HOST_PROCESS);
    assert(strcmp(pBuiltinToolDef->sOperationId, XWORK_HOST_PROCESS_LIST_TERMINALS) == 0);

    memset(&tInterruptCtx, 0, sizeof(tInterruptCtx));
    memset(&tHostInvokeContext, 0, sizeof(tHostInvokeContext));
    tInterruptCtx.sInterruptPhase = "before_process_spawn";
    tHostInvokeContext.pfnShouldInterrupt = xwork_mock_interrupt_check;
    tHostInvokeContext.pInterruptUserData = &tInterruptCtx;
    tHostInvokeContext.sPhase = "tool_execution";
    iHostCancelStatus = xwork_runtime_invoke_host_service_ex(
        pLocalHostRuntime,
        XWORK_HOST_PROCESS,
        XWORK_HOST_PROCESS_EXEC,
        "{\"command\":\"" XWORK_TEST_PROCESS_TIMEOUT_COMMAND "\","
        "\"include_events\":true}",
        &tHostInvokeContext,
        &tLocalHostResult
    );
    assert(tInterruptCtx.iCheckCount > 0);
    assert(iHostCancelStatus == XWORK_ERROR_CANCELLED);
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec cancelled") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"cancelled\":true") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"cancelled\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stop_reason\":\"") != NULL);

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
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_STDERR_COMMAND "\"}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"merge_stderr\":true") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stderr\":\"\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_STDOUT_EXPECTED) != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_STDERR_EXPECTED) != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_STDERR_COMMAND "\","
            "\"merge_stderr\":false}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"merge_stderr\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stdout\":\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_STDOUT_EXPECTED) != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stderr\":\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_STDERR_EXPECTED) != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stdout_truncated\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stderr_truncated\":false") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_STDERR_COMMAND "\","
            "\"merge_stderr\":false,\"include_events\":true}",
            &tLocalHostResult
        ) == XWORK_OK
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"include_events\":true") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"event_count\":") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"events\":[") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"kind\":\"start\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"kind\":\"output\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"kind\":\"exit\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stream\":\"stdout\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stream\":\"stderr\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"text_truncated\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"exit_kind\":\"normal\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_STDOUT_EXPECTED) != NULL);
    assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_STDERR_EXPECTED) != NULL);
    if ( xrtProcessTerminalSupported() ) {
        assert(
            xwork_runtime_invoke_host_service(
                pLocalHostRuntime,
                XWORK_HOST_PROCESS,
                XWORK_HOST_PROCESS_EXEC,
                "{\"command\":\"" XWORK_TEST_PROCESS_TERMINAL_COMMAND "\","
                "\"use_terminal\":true,\"include_events\":true,"
                "\"terminal_cols\":100,\"terminal_rows\":32}",
                &tLocalHostResult
            ) == XWORK_OK
        );
        assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec ok") == 0);
        assert(tLocalHostResult.sOutputText != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"use_terminal\":true") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"terminal_cols\":100") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"terminal_rows\":32") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"terminal_output_captured\":") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"include_events\":true") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"event_count\":") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"events\":[") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"kind\":\"start\"") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"kind\":\"exit\"") != NULL);
        if ( strstr(tLocalHostResult.sOutputText, "\"terminal_output_captured\":true") != NULL ) {
            assert(strstr(tLocalHostResult.sOutputText, "\"kind\":\"output\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"stream\":\"terminal\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, XWORK_TEST_PROCESS_TERMINAL_EXPECTED) != NULL);
        }

        {
            xvalue tTerminalTable = NULL;
            const char *sTerminalSessionId = NULL;
            const char *sSecondaryTerminalSessionId = NULL;
            char *sOwnedTerminalSessionId = NULL;
            char *sOwnedSecondaryTerminalSessionId = NULL;
            char *sTerminalRequestJson = NULL;
            size_t iTerminalNextAfterSeq = 0u;
            size_t iTerminalEventEndSeq = 0u;
            size_t iTerminalSessionIndex = 0u;
            size_t iSecondaryTerminalSessionIndex = 0u;
            size_t iListNextAfterSessionIndex = 0u;
            bool bTerminalHasMoreEvents = false;
            bool bTerminalDone = false;
            bool bTerminalEventStreamDone = false;
            size_t iPoll;

            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_START_TERMINAL,
                    "{\"command\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND "\","
                    "\"session_name\":\"build-shell\","
                    "\"terminal_cols\":100,\"terminal_rows\":32}",
                    &tLocalHostResult
                ) == XWORK_OK
            );
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.start_terminal ok") == 0);
            assert(tLocalHostResult.sOutputText != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"ok\":true") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"terminal_output_captured\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"output_text\":\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"output_bytes\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_count\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"events\":[") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_end_seq\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"has_more_events\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_stream_done\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_name\":\"build-shell\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_index\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"stdin_closed\":false") != NULL);
            assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
            sTerminalSessionId = xwork_test_json_get_text(tTerminalTable, "session_id");
            assert(sTerminalSessionId != NULL);
            sOwnedTerminalSessionId = xwork__dup_cstr(sTerminalSessionId);
            assert(sOwnedTerminalSessionId != NULL);
            sTerminalSessionId = sOwnedTerminalSessionId;
            assert(xwork_test_json_get_size(tTerminalTable, "session_index", &iTerminalSessionIndex));
            assert(iTerminalSessionIndex > 0u);
            assert(xwork_test_json_get_size(tTerminalTable, "next_after_seq", &iTerminalNextAfterSeq));
            assert(xwork_test_json_get_size(tTerminalTable, "event_end_seq", &iTerminalEventEndSeq));
            assert(xwork_test_json_get_bool(tTerminalTable, "has_more_events", &bTerminalHasMoreEvents));
            assert(xwork_test_json_get_bool(tTerminalTable, "event_stream_done", &bTerminalEventStreamDone));
            assert(iTerminalEventEndSeq >= iTerminalNextAfterSeq);
            assert(!bTerminalEventStreamDone);
            xvoUnref(tTerminalTable);
            tTerminalTable = NULL;

            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_LIST_TERMINALS,
                    "{\"session_name\":\"build-shell\",\"running\":true,\"limit\":1}",
                    &tLocalHostResult
                ) == XWORK_OK
            );
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.list_terminals ok") == 0);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_count\":1") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"total_session_count\":1") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"sort\":\"session_index_asc\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"has_more_sessions\":false") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"next_after_session_index\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"sessions\":[") != NULL);
            assert(
                strstr(
                    tLocalHostResult.sOutputText,
                    "\"filters\":{\"session_name\":\"build-shell\",\"running\":true,\"done\":null,\"after_session_index\":null,\"limit\":1}"
                ) != NULL
            );
            assert(strstr(tLocalHostResult.sOutputText, "\"session_name\":\"build-shell\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"stdin_closed\":false") != NULL);
            assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
            {
                xvalue tSessions = xwork_test_json_get_value(tTerminalTable, "sessions");
                xvalue tSessionItem = NULL;
                assert(tSessions != NULL);
                assert(xvoType(tSessions) == XVO_DT_ARRAY);
                assert(xvoArrayItemCount(tSessions) == 1u);
                tSessionItem = xvoArrayGetValue(tSessions, 0u);
                assert(tSessionItem != NULL);
                assert(xvoType(tSessionItem) == XVO_DT_TABLE);
                assert(strcmp(xwork_test_json_get_text(tSessionItem, "session_id"), sTerminalSessionId) == 0);
                assert(strcmp(xwork_test_json_get_text(tSessionItem, "session_name"), "build-shell") == 0);
                assert(xwork_test_json_get_size(tSessionItem, "session_index", &iTerminalSessionIndex));
                assert(xwork_test_json_get_bool(tSessionItem, "running", &bTerminalDone));
                assert(bTerminalDone);
                assert(xwork_test_json_get_size(tTerminalTable, "next_after_session_index", &iListNextAfterSessionIndex));
                assert(iListNextAfterSessionIndex == iTerminalSessionIndex);
            }
            xvoUnref(tTerminalTable);
            tTerminalTable = NULL;

            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_LIST_TERMINALS,
                    "{\"session_name\":\"missing-shell\"}",
                    &tLocalHostResult
                ) == XWORK_OK
            );
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.list_terminals ok") == 0);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_count\":0") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"total_session_count\":1") != NULL);
            assert(
                strstr(
                    tLocalHostResult.sOutputText,
                    "\"filters\":{\"session_name\":\"missing-shell\",\"running\":null,\"done\":null,\"after_session_index\":null,\"limit\":null}"
                ) != NULL
            );
            assert(strstr(tLocalHostResult.sOutputText, "\"has_more_sessions\":false") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"next_after_session_index\":null") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"sessions\":[]") != NULL);

            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_START_TERMINAL,
                    "{\"command\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND "\","
                    "\"session_name\":\"logs-shell\","
                    "\"terminal_cols\":80,\"terminal_rows\":24}",
                    &tLocalHostResult
                ) == XWORK_OK
            );
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.start_terminal ok") == 0);
            assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
            sSecondaryTerminalSessionId = xwork_test_json_get_text(tTerminalTable, "session_id");
            assert(sSecondaryTerminalSessionId != NULL);
            sOwnedSecondaryTerminalSessionId = xwork__dup_cstr(sSecondaryTerminalSessionId);
            assert(sOwnedSecondaryTerminalSessionId != NULL);
            sSecondaryTerminalSessionId = sOwnedSecondaryTerminalSessionId;
            assert(xwork_test_json_get_size(tTerminalTable, "session_index", &iSecondaryTerminalSessionIndex));
            assert(iSecondaryTerminalSessionIndex > iTerminalSessionIndex);
            xvoUnref(tTerminalTable);
            tTerminalTable = NULL;

            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_LIST_TERMINALS,
                    "{\"running\":true,\"limit\":1}",
                    &tLocalHostResult
                ) == XWORK_OK
            );
            assert(strstr(tLocalHostResult.sOutputText, "\"session_count\":1") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"total_session_count\":2") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"sort\":\"session_index_asc\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"has_more_sessions\":true") != NULL);
            assert(
                strstr(
                    tLocalHostResult.sOutputText,
                    "\"filters\":{\"session_name\":null,\"running\":true,\"done\":null,\"after_session_index\":null,\"limit\":1}"
                ) != NULL
            );
            assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
            assert(xwork_test_json_get_size(tTerminalTable, "next_after_session_index", &iListNextAfterSessionIndex));
            assert(iListNextAfterSessionIndex == iTerminalSessionIndex);
            {
                xvalue tSessions = xwork_test_json_get_value(tTerminalTable, "sessions");
                xvalue tSessionItem = NULL;
                assert(tSessions != NULL);
                assert(xvoType(tSessions) == XVO_DT_ARRAY);
                assert(xvoArrayItemCount(tSessions) == 1u);
                tSessionItem = xvoArrayGetValue(tSessions, 0u);
                assert(tSessionItem != NULL);
                assert(strcmp(xwork_test_json_get_text(tSessionItem, "session_name"), "build-shell") == 0);
            }
            xvoUnref(tTerminalTable);
            tTerminalTable = NULL;

            sTerminalRequestJson = xwork__dup_printf(
                "{\"running\":true,\"limit\":1,\"after_session_index\":%zu}",
                iListNextAfterSessionIndex
            );
            assert(sTerminalRequestJson != NULL);
            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_LIST_TERMINALS,
                    sTerminalRequestJson,
                    &tLocalHostResult
                ) == XWORK_OK
            );
            free(sTerminalRequestJson);
            sTerminalRequestJson = NULL;
            assert(strstr(tLocalHostResult.sOutputText, "\"session_count\":1") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"total_session_count\":2") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"has_more_sessions\":false") != NULL);
            assert(
                strstr(
                    tLocalHostResult.sOutputText,
                    "\"filters\":{\"session_name\":null,\"running\":true,\"done\":null,\"after_session_index\":1,\"limit\":1}"
                ) != NULL
            );
            assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
            assert(xwork_test_json_get_size(tTerminalTable, "next_after_session_index", &iListNextAfterSessionIndex));
            assert(iListNextAfterSessionIndex == iSecondaryTerminalSessionIndex);
            {
                xvalue tSessions = xwork_test_json_get_value(tTerminalTable, "sessions");
                xvalue tSessionItem = NULL;
                assert(tSessions != NULL);
                assert(xvoType(tSessions) == XVO_DT_ARRAY);
                assert(xvoArrayItemCount(tSessions) == 1u);
                tSessionItem = xvoArrayGetValue(tSessions, 0u);
                assert(tSessionItem != NULL);
                assert(strcmp(xwork_test_json_get_text(tSessionItem, "session_name"), "logs-shell") == 0);
            }
            xvoUnref(tTerminalTable);
            tTerminalTable = NULL;

            sTerminalRequestJson = xwork__dup_printf(
                "{\"session_id\":\"%s\"}",
                sSecondaryTerminalSessionId
            );
            assert(sTerminalRequestJson != NULL);
            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_TERMINAL_STOP,
                    sTerminalRequestJson,
                    &tLocalHostResult
                ) == XWORK_OK
            );
            free(sTerminalRequestJson);
            sTerminalRequestJson = NULL;
            free(sOwnedSecondaryTerminalSessionId);
            sOwnedSecondaryTerminalSessionId = NULL;
            sSecondaryTerminalSessionId = NULL;

            sTerminalRequestJson = xwork__dup_printf(
                "{\"session_id\":\"%s\",\"terminal_cols\":140,\"terminal_rows\":40}",
                sTerminalSessionId
            );
            assert(sTerminalRequestJson != NULL);
            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_TERMINAL_RESIZE,
                    sTerminalRequestJson,
                    &tLocalHostResult
                ) == XWORK_OK
            );
            free(sTerminalRequestJson);
            sTerminalRequestJson = NULL;
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.terminal_resize ok") == 0);
            assert(strstr(tLocalHostResult.sOutputText, "\"resize_applied\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"terminal_cols\":140") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"terminal_rows\":40") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_index\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"stdin_closed\":false") != NULL);

            sTerminalRequestJson = xwork__dup_printf(
                iTerminalNextAfterSeq > 0u
                    ? "{\"session_id\":\"%s\",\"input_text\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "\","
                      "\"include_state\":true,\"after_seq\":%zu}"
                    : "{\"session_id\":\"%s\",\"input_text\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "\","
                      "\"include_state\":true}",
                sTerminalSessionId,
                iTerminalNextAfterSeq
            );
            assert(sTerminalRequestJson != NULL);
            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_TERMINAL_WRITE,
                    sTerminalRequestJson,
                    &tLocalHostResult
                ) == XWORK_OK
            );
            free(sTerminalRequestJson);
            sTerminalRequestJson = NULL;
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.terminal_write ok") == 0);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_index\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"bytes_written\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"write_eof\":false") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"stdin_closed\":false") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"output_text\":\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"output_bytes\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_count\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"events\":[") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_end_seq\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"has_more_events\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_stream_done\":") != NULL);
            assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
            assert(xwork_test_json_get_size(tTerminalTable, "next_after_seq", &iTerminalNextAfterSeq));
            assert(xwork_test_json_get_size(tTerminalTable, "event_end_seq", &iTerminalEventEndSeq));
            assert(xwork_test_json_get_bool(tTerminalTable, "has_more_events", &bTerminalHasMoreEvents));
            assert(xwork_test_json_get_bool(tTerminalTable, "event_stream_done", &bTerminalEventStreamDone));
            assert(iTerminalEventEndSeq >= iTerminalNextAfterSeq);
            xvoUnref(tTerminalTable);
            tTerminalTable = NULL;

            for ( iPoll = 0u; iPoll < 5u; ++iPoll ) {
                sTerminalRequestJson = xwork__dup_printf(
                    iTerminalNextAfterSeq > 0u
                        ? "{\"session_id\":\"%s\",\"after_seq\":%zu}"
                        : "{\"session_id\":\"%s\"}",
                    sTerminalSessionId,
                    iTerminalNextAfterSeq
                );
                assert(sTerminalRequestJson != NULL);
                assert(
                    xwork_runtime_invoke_host_service(
                        pLocalHostRuntime,
                        XWORK_HOST_PROCESS,
                        XWORK_HOST_PROCESS_TERMINAL_READ,
                        sTerminalRequestJson,
                        &tLocalHostResult
                    ) == XWORK_OK
                );
                free(sTerminalRequestJson);
                sTerminalRequestJson = NULL;
                assert(strcmp(tLocalHostResult.sVisibleSummary, "process.terminal_read ok") == 0);
                assert(strstr(tLocalHostResult.sOutputText, "\"session_index\":") != NULL);
                assert(strstr(tLocalHostResult.sOutputText, "\"stdin_closed\":false") != NULL);
                assert(strstr(tLocalHostResult.sOutputText, "\"terminal_output_captured\":") != NULL);
                assert(strstr(tLocalHostResult.sOutputText, "\"output_text\":\"") != NULL);
                assert(strstr(tLocalHostResult.sOutputText, "\"output_bytes\":") != NULL);
                assert(strstr(tLocalHostResult.sOutputText, "\"event_end_seq\":") != NULL);
                assert(strstr(tLocalHostResult.sOutputText, "\"has_more_events\":") != NULL);
                assert(strstr(tLocalHostResult.sOutputText, "\"event_stream_done\":") != NULL);
                assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
                assert(xwork_test_json_get_bool(tTerminalTable, "done", &bTerminalDone));
                assert(xwork_test_json_get_size(tTerminalTable, "next_after_seq", &iTerminalNextAfterSeq));
                assert(xwork_test_json_get_size(tTerminalTable, "event_end_seq", &iTerminalEventEndSeq));
                assert(xwork_test_json_get_bool(tTerminalTable, "has_more_events", &bTerminalHasMoreEvents));
                assert(xwork_test_json_get_bool(tTerminalTable, "event_stream_done", &bTerminalEventStreamDone));
                assert(iTerminalEventEndSeq >= iTerminalNextAfterSeq);
                if ( bTerminalHasMoreEvents ) {
                    assert(iTerminalEventEndSeq > iTerminalNextAfterSeq);
                }
                xvoUnref(tTerminalTable);
                tTerminalTable = NULL;
                if ( bTerminalDone ) {
                    assert(bTerminalEventStreamDone);
                    break;
                }
                xrtSleep(20u);
            }
            assert(strstr(tLocalHostResult.sOutputText, "\"event_count\":") != NULL);

            sTerminalRequestJson = xwork__dup_printf(
                "{\"session_id\":\"%s\"}",
                sTerminalSessionId
            );
            assert(sTerminalRequestJson != NULL);
            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_TERMINAL_STOP,
                    sTerminalRequestJson,
                    &tLocalHostResult
                ) == XWORK_OK
            );
            free(sTerminalRequestJson);
            sTerminalRequestJson = NULL;
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.terminal_stop ok") == 0);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_name\":\"build-shell\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_index\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"stdin_closed\":false") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"terminal_output_captured\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"output_text\":\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"output_bytes\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_end_seq\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"has_more_events\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_stream_done\":true") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"removed\":true") != NULL);

            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_LIST_TERMINALS,
                    "{}",
                    &tLocalHostResult
                ) == XWORK_OK
            );
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.list_terminals ok") == 0);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_count\":0") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"sessions\":[]") != NULL);

            sTerminalRequestJson = xwork__dup_printf(
                "{\"session_id\":\"%s\"}",
                sTerminalSessionId
            );
            assert(sTerminalRequestJson != NULL);
            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_TERMINAL_READ,
                    sTerminalRequestJson,
                    &tLocalHostResult
                ) == XWORK_ERROR_NOT_FOUND
            );
            free(sTerminalRequestJson);
            sTerminalRequestJson = NULL;
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.terminal_read failed") == 0);
            assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"not_found\"") != NULL);
            free(sOwnedTerminalSessionId);
            free(sOwnedSecondaryTerminalSessionId);

            sOwnedTerminalSessionId = NULL;
            sOwnedSecondaryTerminalSessionId = NULL;
            iTerminalNextAfterSeq = 0u;
            iTerminalEventEndSeq = 0u;
            bTerminalHasMoreEvents = false;
            bTerminalDone = false;
            bTerminalEventStreamDone = false;
            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_START_TERMINAL,
                    "{\"command\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND "\","
                    "\"session_name\":\"build-shell\","
                    "\"terminal_cols\":100,\"terminal_rows\":32}",
                    &tLocalHostResult
                ) == XWORK_OK
            );
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.start_terminal ok") == 0);
            assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
            sTerminalSessionId = xwork_test_json_get_text(tTerminalTable, "session_id");
            assert(sTerminalSessionId != NULL);
            sOwnedTerminalSessionId = xwork__dup_cstr(sTerminalSessionId);
            assert(sOwnedTerminalSessionId != NULL);
            sTerminalSessionId = sOwnedTerminalSessionId;
            assert(xwork_test_json_get_size(tTerminalTable, "session_index", &iTerminalSessionIndex));
            assert(iTerminalSessionIndex > 0u);
            assert(xwork_test_json_get_size(tTerminalTable, "next_after_seq", &iTerminalNextAfterSeq));
            xvoUnref(tTerminalTable);
            tTerminalTable = NULL;

            sTerminalRequestJson = xwork__dup_printf(
                iTerminalNextAfterSeq > 0u
                    ? "{\"session_id\":\"%s\",\"input_text\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "\","
                      "\"write_eof\":true,\"include_state\":true,\"after_seq\":%zu}"
                    : "{\"session_id\":\"%s\",\"input_text\":\"" XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT "\","
                      "\"write_eof\":true,\"include_state\":true}",
                sTerminalSessionId,
                iTerminalNextAfterSeq
            );
            assert(sTerminalRequestJson != NULL);
            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_TERMINAL_WRITE,
                    sTerminalRequestJson,
                    &tLocalHostResult
                ) == XWORK_OK
            );
            free(sTerminalRequestJson);
            sTerminalRequestJson = NULL;
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.terminal_write ok") == 0);
            assert(strstr(tLocalHostResult.sOutputText, "\"session_index\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"bytes_written\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"write_eof\":true") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"stdin_closed\":true") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"output_text\":\"") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"output_bytes\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_end_seq\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"has_more_events\":") != NULL);
            assert(strstr(tLocalHostResult.sOutputText, "\"event_stream_done\":") != NULL);
            assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
            assert(xwork_test_json_get_size(tTerminalTable, "next_after_seq", &iTerminalNextAfterSeq));
            xvoUnref(tTerminalTable);
            tTerminalTable = NULL;

            for ( iPoll = 0u; iPoll < 5u; ++iPoll ) {
                sTerminalRequestJson = xwork__dup_printf(
                    iTerminalNextAfterSeq > 0u
                        ? "{\"session_id\":\"%s\",\"after_seq\":%zu}"
                        : "{\"session_id\":\"%s\"}",
                    sTerminalSessionId,
                    iTerminalNextAfterSeq
                );
                assert(sTerminalRequestJson != NULL);
                assert(
                    xwork_runtime_invoke_host_service(
                        pLocalHostRuntime,
                        XWORK_HOST_PROCESS,
                        XWORK_HOST_PROCESS_TERMINAL_READ,
                        sTerminalRequestJson,
                        &tLocalHostResult
                    ) == XWORK_OK
                );
                free(sTerminalRequestJson);
                sTerminalRequestJson = NULL;
                assert(strcmp(tLocalHostResult.sVisibleSummary, "process.terminal_read ok") == 0);
                assert(strstr(tLocalHostResult.sOutputText, "\"session_index\":") != NULL);
                assert(strstr(tLocalHostResult.sOutputText, "\"stdin_closed\":true") != NULL);
                assert(xwork_test_parse_json_table(tLocalHostResult.sOutputText, &tTerminalTable));
                assert(xwork_test_json_get_bool(tTerminalTable, "done", &bTerminalDone));
                assert(xwork_test_json_get_size(tTerminalTable, "next_after_seq", &iTerminalNextAfterSeq));
                assert(xwork_test_json_get_size(tTerminalTable, "event_end_seq", &iTerminalEventEndSeq));
                assert(xwork_test_json_get_bool(tTerminalTable, "has_more_events", &bTerminalHasMoreEvents));
                assert(xwork_test_json_get_bool(tTerminalTable, "event_stream_done", &bTerminalEventStreamDone));
                xvoUnref(tTerminalTable);
                tTerminalTable = NULL;
                if ( bTerminalDone ) {
                    assert(bTerminalEventStreamDone);
                    assert(strstr(tLocalHostResult.sOutputText, "\"kind\":\"exit\"") != NULL);
                    break;
                }
                xrtSleep(20u);
            }
            assert(strstr(tLocalHostResult.sOutputText, "\"terminal_output_captured\":") != NULL);

            sTerminalRequestJson = xwork__dup_printf(
                "{\"session_id\":\"%s\"}",
                sTerminalSessionId
            );
            assert(sTerminalRequestJson != NULL);
            assert(
                xwork_runtime_invoke_host_service(
                    pLocalHostRuntime,
                    XWORK_HOST_PROCESS,
                    XWORK_HOST_PROCESS_TERMINAL_STOP,
                    sTerminalRequestJson,
                    &tLocalHostResult
                ) == XWORK_OK
            );
            free(sTerminalRequestJson);
            sTerminalRequestJson = NULL;
            assert(strcmp(tLocalHostResult.sVisibleSummary, "process.terminal_stop ok") == 0);
            assert(strstr(tLocalHostResult.sOutputText, "\"removed\":true") != NULL);

            free(sOwnedTerminalSessionId);
        }
    } else {
        assert(
            xwork_runtime_invoke_host_service(
                pLocalHostRuntime,
                XWORK_HOST_PROCESS,
                XWORK_HOST_PROCESS_EXEC,
                "{\"command\":\"" XWORK_TEST_PROCESS_TERMINAL_COMMAND "\","
                "\"use_terminal\":true,\"include_events\":true,"
                "\"terminal_cols\":100,\"terminal_rows\":32}",
                &tLocalHostResult
            ) == XWORK_ERROR_UNSUPPORTED
        );
        assert(
            strcmp(
                tLocalHostResult.sVisibleSummary,
                "process.exec terminal mode unsupported"
            ) == 0
        );
        assert(tLocalHostResult.sOutputText != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"ok\":false") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"use_terminal\":true") != NULL);
        assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"unsupported\"") != NULL);
        assert(
            strstr(
                tLocalHostResult.sOutputText,
                "\"error\":\"terminal mode is not supported on this platform\""
            ) != NULL
        );
    }
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
            "{\"command\":\"echo xwork-local-process-stderr-flag\","
            "\"merge_stderr\":\"bogus\"}",
            &tLocalHostResult
        ) == XWORK_ERROR_INVALID_ARGUMENT
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec invalid request") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"invalid_request\"") != NULL);
    assert(
        strstr(
            tLocalHostResult.sOutputText,
            "\"error\":\"merge_stderr must be boolean\""
        ) != NULL
    );
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_TIMEOUT_COMMAND "\","
            "\"timeout_ms\":" XWORK_TEST_STRINGIFY(XWORK_TEST_PROCESS_TIMEOUT_MS) "}",
            &tLocalHostResult
        ) == XWORK_ERROR_EXTERNAL_FAILURE
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec timed out") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"ok\":false") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"timeout\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"timeout_ms\":50") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"timed_out\":true") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"timeout_stop\":\"interrupt\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stop_reason\":\"") != NULL);
    assert(
        strstr(
            tLocalHostResult.sOutputText,
            "\"error\":\"process exceeded timeout_ms\""
        ) != NULL
    );
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"" XWORK_TEST_PROCESS_TIMEOUT_COMMAND "\","
            "\"timeout_ms\":" XWORK_TEST_STRINGIFY(XWORK_TEST_PROCESS_TIMEOUT_MS) ","
            "\"timeout_stop\":\"kill_tree\"}",
            &tLocalHostResult
        ) == XWORK_ERROR_EXTERNAL_FAILURE
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec timed out") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"timeout_stop\":\"kill_tree\"") != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"stop_reason\":\"") != NULL);
    assert(
        xwork_runtime_invoke_host_service(
            pLocalHostRuntime,
            XWORK_HOST_PROCESS,
            XWORK_HOST_PROCESS_EXEC,
            "{\"command\":\"echo xwork-local-process-timeout-stop\","
            "\"timeout_ms\":" XWORK_TEST_STRINGIFY(XWORK_TEST_PROCESS_TIMEOUT_MS) ","
            "\"timeout_stop\":\"bogus\"}",
            &tLocalHostResult
        ) == XWORK_ERROR_INVALID_ARGUMENT
    );
    assert(strcmp(tLocalHostResult.sVisibleSummary, "process.exec invalid request") == 0);
    assert(tLocalHostResult.sOutputText != NULL);
    assert(strstr(tLocalHostResult.sOutputText, "\"error_kind\":\"invalid_request\"") != NULL);
    assert(
        strstr(
            tLocalHostResult.sOutputText,
            "\"error\":\"timeout_stop must be one of interrupt, terminate, kill, kill_tree\""
        ) != NULL
    );
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

    memset(&tInterruptCtx, 0, sizeof(tInterruptCtx));
    tInterruptCtx.sInterruptPhase = "before_process_spawn";
    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-host-process-cancelled";
    tRunOptions.sInstruction = "Run the local process with timeout through host service.";
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
    tExecOptions.pfnShouldInterrupt = xwork_mock_interrupt_check;
    tExecOptions.pInterruptUserData = &tInterruptCtx;
    assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_ERROR_CANCELLED);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_CANCELLED);
    assert(tInterruptCtx.iCheckCount > 0);
    assert(xwork_run_get_last_checkpoint(pLocalHostRun, &tCheckpoint) == XWORK_OK);
    assert(tCheckpoint.eRunState == XWORK_RUN_CANCELLED);
    xwork_checkpoint_reset(&tCheckpoint);
    assert(xwork_run_get_last_event(pLocalHostRun, &tEvent) == XWORK_OK);
    assert(tEvent.eKind == XWORK_EVENT_RUN_CANCELLED);
    xwork_event_reset(&tEvent);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-host-process-async-cancelled";
    tRunOptions.sInstruction = "Run the local process for async cancellation through host service.";
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
    assert(xwork_run_execute_async(pLocalHostRun, &tExecOptions, &pAsync) == XWORK_OK);
    xrtSleep(500u);
    assert(xwork_run_async_cancel(pAsync, "async local process cancel") == XWORK_OK);
    assert(xwork_run_async_wait(pAsync) == XWORK_ERROR_CANCELLED);
    assert(xwork_run_async_get_status(pAsync, &iAsyncStatus, &bAsyncCompleted) == XWORK_OK);
    assert(bAsyncCompleted);
    assert(iAsyncStatus == XWORK_ERROR_CANCELLED);
    assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_CANCELLED);
    assert(tAdapterCtx.iTurnCount == 1);
    xwork_checkpoint_init(&tCheckpoint);
    assert(xwork_run_get_last_checkpoint(pLocalHostRun, &tCheckpoint) == XWORK_OK);
    assert(tCheckpoint.eRunState == XWORK_RUN_CANCELLED);
    assert(tCheckpoint.sPendingStep != NULL);
    assert(strcmp(tCheckpoint.sPendingStep, "tool_cancelled") == 0);
    xwork_checkpoint_reset(&tCheckpoint);
    xwork_run_async_destroy(pAsync);
    pAsync = NULL;
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

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
    xwork_test_assert_content_stats(&tArtifact, 20u, 1u);
    assert(tArtifact.bHasExitCode);
    assert(tArtifact.iExitCode == 0);
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 2u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
    xwork_test_assert_output_class(
        &tArtifact,
        XWORK_ARTIFACT_OUTPUT_FILE_CONTENT,
        XWORK_TOOL_FILESYSTEM_READ_TEXT
    );
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
    xwork_test_assert_output_class(
        &tArtifact,
        XWORK_ARTIFACT_OUTPUT_FILE_CHANGE,
        XWORK_TOOL_FILESYSTEM_WRITE_TEXT
    );
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
    if ( xrtProcessTerminalSupported() ) {
        tAdapterCtx.iTurnCount = 0;
        asWorkspaceIds[0] = "local-host-main";
        xwork_run_options_init(&tRunOptions);
        xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
        tRunOptions.sRunId = "run-local-process-terminal-tool";
        tRunOptions.sInstruction = "Run the local process in terminal mode through host service.";
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
                "Host service process terminal completed."
            ) == 0
        );
        assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
        assert(tRunSnapshot.sLastToolResultText != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"use_terminal\":true") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"terminal_cols\":100") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"terminal_rows\":32") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"terminal_output_captured\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"event_count\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"kind\":\"start\"") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"kind\":\"exit\"") != NULL);
        bTerminalOutputCaptured =
            strstr(tRunSnapshot.sLastToolResultText, "\"terminal_output_captured\":true") != NULL;
        if ( bTerminalOutputCaptured ) {
            assert(strstr(tRunSnapshot.sLastToolResultText, "\"kind\":\"output\"") != NULL);
            assert(strstr(tRunSnapshot.sLastToolResultText, "\"stream\":\"terminal\"") != NULL);
            assert(strstr(tRunSnapshot.sLastToolResultText, XWORK_TEST_PROCESS_TERMINAL_EXPECTED) != NULL);
        }
        xwork_run_snapshot_reset(&tRunSnapshot);
        assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
        assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
        assert(strcmp(tArtifact.sName, "process.exec.txt") == 0);
        assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_TERMINAL_COMMAND) == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strcmp(tArtifact.sStorageRef, ".") == 0);
        assert(tArtifact.sContentText != NULL);
        if ( bTerminalOutputCaptured ) {
            assert(strstr(tArtifact.sContentText, XWORK_TEST_PROCESS_TERMINAL_EXPECTED) != NULL);
        } else {
            assert(strcmp(tArtifact.sContentText, "") == 0);
        }
        assert(tArtifact.bHasExitCode);
        assert(tArtifact.iExitCode == 0);
        xwork_artifact_reset(&tArtifact);
        xwork_run_destroy(pLocalHostRun);
        pLocalHostRun = NULL;

        tAdapterCtx.iTurnCount = 0;
        asWorkspaceIds[0] = "local-host-main";
        xwork_run_options_init(&tRunOptions);
        xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
        tRunOptions.sRunId = "run-local-terminal-session-tool";
        tRunOptions.sInstruction = "Run the local interactive terminal session through host service.";
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
        tExecOptions.iMaxTurns = 8u;
        tExecOptions.bAutoApprove = true;
        assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
        assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
        assert(
            strcmp(
                xwork_run_get_last_output_text(pLocalHostRun),
                "Host service terminal session completed."
            ) == 0
        );
        assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
        assert(tRunSnapshot.sLastToolResultText != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"ok\":true") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"removed\":true") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"session_index\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"stdin_closed\":false") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"terminal_output_captured\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"output_text\":\"") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"output_bytes\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"event_end_seq\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"has_more_events\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"event_stream_done\":true") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"session_id\":\"terminal-session-") != NULL);
        bTerminalOutputCaptured =
            strstr(
                tRunSnapshot.sLastToolResultText,
                "\"terminal_output_captured\":true"
            ) != NULL;
        xwork_run_snapshot_reset(&tRunSnapshot);
        assert(
            xwork_run_get_artifact_count(pLocalHostRun) ==
            (bTerminalOutputCaptured ? 7u : 6u)
        );
        assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
        assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND) == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 1u, &tArtifact) == XWORK_OK);
        assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        xwork_test_assert_output_class(
            &tArtifact,
            XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE,
            XWORK_TOOL_PROCESS_TERMINAL_RESIZE
        );
        assert(strcmp(tArtifact.sName, "process.terminal_resize.json") == 0);
        assert(strcmp(tArtifact.sMimeType, "application/json") == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
        assert(tArtifact.sContentText != NULL);
        assert(strstr(tArtifact.sContentText, "\"resize_applied\":") != NULL);
        assert(strstr(tArtifact.sContentText, "\"terminal_cols\":140") != NULL);
        assert(strstr(tArtifact.sContentText, "\"terminal_rows\":40") != NULL);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 2u, &tArtifact) == XWORK_OK);
        assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT) == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 3u, &tArtifact) == XWORK_OK);
        assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        xwork_test_assert_output_class(
            &tArtifact,
            XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE,
            XWORK_TOOL_PROCESS_TERMINAL_WRITE
        );
        assert(strcmp(tArtifact.sName, "process.terminal_write.json") == 0);
        assert(strcmp(tArtifact.sMimeType, "application/json") == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
        assert(tArtifact.sContentText != NULL);
        assert(strstr(tArtifact.sContentText, "\"next_after_seq\":") != NULL);
        assert(strstr(tArtifact.sContentText, "\"event_end_seq\":") != NULL);
        assert(strstr(tArtifact.sContentText, "\"session_index\":") != NULL);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 4u, &tArtifact) == XWORK_OK);
        assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        xwork_test_assert_output_class(
            &tArtifact,
            XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE,
            XWORK_TOOL_PROCESS_TERMINAL_READ
        );
        assert(strcmp(tArtifact.sName, "process.terminal_read.json") == 0);
        assert(strcmp(tArtifact.sMimeType, "application/json") == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
        assert(tArtifact.sContentText != NULL);
        assert(strstr(tArtifact.sContentText, "\"event_end_seq\":") != NULL);
        assert(strstr(tArtifact.sContentText, "\"next_after_seq\":") != NULL);
        assert(strstr(tArtifact.sContentText, "\"session_index\":") != NULL);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 5u, &tArtifact) == XWORK_OK);
        assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        xwork_test_assert_output_class(
            &tArtifact,
            XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE,
            XWORK_TOOL_PROCESS_TERMINAL_STOP
        );
        assert(strcmp(tArtifact.sName, "process.terminal_stop.json") == 0);
        assert(strcmp(tArtifact.sMimeType, "application/json") == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
        assert(tArtifact.sContentText != NULL);
        assert(strstr(tArtifact.sContentText, "\"removed\":true") != NULL);
        assert(strstr(tArtifact.sContentText, "\"event_stream_done\":true") != NULL);
        xwork_artifact_reset(&tArtifact);
        if ( bTerminalOutputCaptured ) {
            assert(xwork_run_get_artifact(pLocalHostRun, 6u, &tArtifact) == XWORK_OK);
            xwork_test_assert_output_class(
                &tArtifact,
                XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE,
                XWORK_TOOL_PROCESS_TERMINAL_STOP
            );
            assert(tArtifact.sContentText != NULL);
            assert(strstr(tArtifact.sContentText, XWORK_TEST_PROCESS_TERMINAL_SESSION_EXPECTED) != NULL);
            assert(tArtifact.sStorageRef != NULL);
            assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
            xwork_artifact_reset(&tArtifact);
        }
        xwork_run_destroy(pLocalHostRun);
        pLocalHostRun = NULL;

        tAdapterCtx.iTurnCount = 0;
        asWorkspaceIds[0] = "local-host-main";
        xwork_run_options_init(&tRunOptions);
        xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
        tRunOptions.sRunId = "run-local-terminal-list-tool";
        tRunOptions.sInstruction = "Run the local interactive terminal session listing through host service.";
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
        tExecOptions.iMaxTurns = 8u;
        tExecOptions.bAutoApprove = true;
        assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
        assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
        assert(
            strcmp(
                xwork_run_get_last_output_text(pLocalHostRun),
                "Host service terminal list completed."
            ) == 0
        );
        assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
        assert(tRunSnapshot.sLastToolResultText != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"removed\":true") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"session_name\":\"build-shell\"") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"session_index\":") != NULL);
        xwork_run_snapshot_reset(&tRunSnapshot);
        assert(xwork_run_get_artifact_count(pLocalHostRun) >= 2u);
        assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
        assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND) == 0);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 1u, &tArtifact) == XWORK_OK);
        assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        xwork_test_assert_output_class(
            &tArtifact,
            XWORK_ARTIFACT_OUTPUT_TERMINAL_INVENTORY,
            XWORK_TOOL_PROCESS_LIST_TERMINALS
        );
        assert(strcmp(tArtifact.sName, "process.list_terminals.json") == 0);
        assert(strcmp(tArtifact.sMimeType, "application/json") == 0);
        assert(strcmp(tArtifact.sStorageRef, "terminal-sessions://active") == 0);
        assert(tArtifact.sContentText != NULL);
        assert(strstr(tArtifact.sContentText, "\"session_name\":\"build-shell\"") != NULL);
        assert(strstr(tArtifact.sContentText, "\"sort\":\"session_index_asc\"") != NULL);
        xwork_artifact_reset(&tArtifact);
        xwork_run_destroy(pLocalHostRun);
        pLocalHostRun = NULL;

        tAdapterCtx.iTurnCount = 0;
        asWorkspaceIds[0] = "local-host-main";
        xwork_run_options_init(&tRunOptions);
        xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
        tRunOptions.sRunId = "run-local-terminal-session-eof-tool";
        tRunOptions.sInstruction = "Run the local interactive terminal session with EOF through host service.";
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
        tExecOptions.iMaxTurns = 8u;
        tExecOptions.bAutoApprove = true;
        assert(xwork_run_execute(pLocalHostRun, &tExecOptions) == XWORK_OK);
        assert(xwork_run_get_state(pLocalHostRun) == XWORK_RUN_COMPLETED);
        assert(
            strcmp(
                xwork_run_get_last_output_text(pLocalHostRun),
                "Host service terminal EOF session completed."
            ) == 0
        );
        assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
        assert(tRunSnapshot.sLastToolResultText != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"ok\":true") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"done\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"event_stream_done\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"session_id\":\"terminal-session-") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"session_index\":") != NULL);
        assert(strstr(tRunSnapshot.sLastToolResultText, "\"stdin_closed\":true") != NULL);
        xwork_run_snapshot_reset(&tRunSnapshot);
        assert(xwork_run_get_artifact_count(pLocalHostRun) >= 5u);
        assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
        assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_TERMINAL_SESSION_COMMAND) == 0);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 1u, &tArtifact) == XWORK_OK);
        assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        assert(strcmp(tArtifact.sName, "process.terminal_resize.json") == 0);
        assert(strcmp(tArtifact.sMimeType, "application/json") == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
        assert(tArtifact.sContentText != NULL);
        assert(strstr(tArtifact.sContentText, "\"stdin_closed\":false") != NULL);
        assert(strstr(tArtifact.sContentText, "\"resize_applied\":") != NULL);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 2u, &tArtifact) == XWORK_OK);
        assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_TERMINAL_SESSION_INPUT) == 0);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 3u, &tArtifact) == XWORK_OK);
        assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        assert(strcmp(tArtifact.sName, "process.terminal_write.json") == 0);
        assert(strcmp(tArtifact.sMimeType, "application/json") == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
        assert(tArtifact.sContentText != NULL);
        assert(strstr(tArtifact.sContentText, "\"stdin_closed\":true") != NULL);
        assert(strstr(tArtifact.sContentText, "\"next_after_seq\":") != NULL);
        xwork_artifact_reset(&tArtifact);
        assert(xwork_run_get_artifact(pLocalHostRun, 4u, &tArtifact) == XWORK_OK);
        assert(tArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        assert(strcmp(tArtifact.sName, "process.terminal_read.json") == 0);
        assert(strcmp(tArtifact.sMimeType, "application/json") == 0);
        assert(tArtifact.sStorageRef != NULL);
        assert(strstr(tArtifact.sStorageRef, "terminal-session-") != NULL);
        assert(tArtifact.sContentText != NULL);
        assert(strstr(tArtifact.sContentText, "\"stdin_closed\":true") != NULL);
        assert(strstr(tArtifact.sContentText, "\"event_stream_done\":") != NULL);
        xwork_artifact_reset(&tArtifact);
        xwork_run_destroy(pLocalHostRun);
        pLocalHostRun = NULL;
    }

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-events-tool";
    tRunOptions.sInstruction = "Run the local process with ordered events through host service.";
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
            "Host service process events completed."
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"include_events\":true") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"event_count\":") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"kind\":\"start\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"kind\":\"output\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"kind\":\"exit\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"stream\":\"stdout\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"stream\":\"stderr\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, XWORK_TEST_PROCESS_STDOUT_EXPECTED) != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, XWORK_TEST_PROCESS_STDERR_EXPECTED) != NULL);
    xwork_run_snapshot_reset(&tRunSnapshot);
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sName, "process.exec.txt") == 0);
    assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_STDERR_COMMAND_RAW) == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, XWORK_TEST_PROCESS_STDOUT_EXPECTED) != NULL);
    assert(strstr(tArtifact.sContentText, XWORK_TEST_PROCESS_STDERR_EXPECTED) != NULL);
    assert(tArtifact.bHasCommandIoStats);
    assert(tArtifact.iStdoutByteCount >= strlen(XWORK_TEST_PROCESS_STDOUT_EXPECTED));
    assert(tArtifact.iStderrByteCount >= strlen(XWORK_TEST_PROCESS_STDERR_EXPECTED));
    assert(!tArtifact.bStdoutTruncated);
    assert(!tArtifact.bStderrTruncated);
    assert(tArtifact.bHasExitCode);
    assert(tArtifact.iExitCode == 0);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-stderr-tool";
    tRunOptions.sInstruction = "Run the local process with separate stderr through host service.";
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
            "Host service process stderr completed."
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"merge_stderr\":false") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, XWORK_TEST_PROCESS_STDOUT_EXPECTED) != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, XWORK_TEST_PROCESS_STDERR_EXPECTED) != NULL);
    xwork_run_snapshot_reset(&tRunSnapshot);
    assert(xwork_run_get_artifact_count(pLocalHostRun) == 1u);
    xwork_artifact_reset(&tArtifact);
    assert(xwork_run_get_artifact(pLocalHostRun, 0u, &tArtifact) == XWORK_OK);
    assert(tArtifact.eKind == XWORK_ARTIFACT_COMMAND);
    assert(strcmp(tArtifact.sName, "process.exec.txt") == 0);
    assert(strcmp(tArtifact.sCommandText, XWORK_TEST_PROCESS_STDERR_COMMAND_RAW) == 0);
    assert(tArtifact.sContentText != NULL);
    assert(strstr(tArtifact.sContentText, XWORK_TEST_PROCESS_STDOUT_EXPECTED) != NULL);
    assert(strstr(tArtifact.sContentText, XWORK_TEST_PROCESS_STDERR_EXPECTED) != NULL);
    assert(tArtifact.bHasCommandIoStats);
    assert(tArtifact.iStdoutByteCount >= strlen(XWORK_TEST_PROCESS_STDOUT_EXPECTED));
    assert(tArtifact.iStderrByteCount >= strlen(XWORK_TEST_PROCESS_STDERR_EXPECTED));
    assert(!tArtifact.bStdoutTruncated);
    assert(!tArtifact.bStderrTruncated);
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
    assert(tArtifact.bHasCommandIoStats);
    assert(tArtifact.iStdoutByteCount >= strlen(XWORK_TEST_PROCESS_STDIN_EXPECTED));
    assert(tArtifact.iStderrByteCount == 0u);
    assert(!tArtifact.bStdoutTruncated);
    assert(!tArtifact.bStderrTruncated);
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
    assert(tArtifact.bHasCommandIoStats);
    assert(tArtifact.iStdoutByteCount >= strlen(XWORK_TEST_PROCESS_NONZERO_EXPECTED));
    assert(tArtifact.iStderrByteCount == 0u);
    assert(!tArtifact.bStdoutTruncated);
    assert(!tArtifact.bStderrTruncated);
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
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-timeout-tool";
    tRunOptions.sInstruction = "Run the local process with timeout through host service.";
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
            "process.exec timed out"
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"ok\":false") != NULL);
    assert(
        strstr(
            tRunSnapshot.sLastToolResultText,
            "\"error_kind\":\"timeout\""
        ) != NULL
    );
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"timeout_ms\":50") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"timed_out\":true") != NULL);
    assert(
        strstr(
            tRunSnapshot.sLastToolResultText,
            "\"error\":\"process exceeded timeout_ms\""
        ) != NULL
    );
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"timeout_stop\":\"interrupt\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"stop_reason\":\"") != NULL);
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

    tAdapterCtx.iTurnCount = 0;
    asWorkspaceIds[0] = "local-host-main";
    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-local-process-timeout-kill-tree-tool";
    tRunOptions.sInstruction = "Run the local process with kill-tree timeout through host service.";
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
            "process.exec timed out"
        ) == 0
    );
    assert(xwork_run_get_snapshot(pLocalHostRun, &tRunSnapshot) == XWORK_OK);
    assert(tRunSnapshot.sLastToolResultText != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"error_kind\":\"timeout\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"timeout_stop\":\"kill_tree\"") != NULL);
    assert(strstr(tRunSnapshot.sLastToolResultText, "\"stop_reason\":\"") != NULL);
    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_run_destroy(pLocalHostRun);
    pLocalHostRun = NULL;

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
    xwork_test_assert_command_io_stats(&tArtifact, 12u, 0u, true, false);
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
    assert(tArtifact.bHasCommandIoStats);
    assert(tArtifact.iStdoutByteCount >= strlen(XWORK_TEST_PROCESS_ENV_EXPECTED));
    assert(tArtifact.iStderrByteCount == 0u);
    assert(!tArtifact.bStdoutTruncated);
    assert(!tArtifact.bStderrTruncated);
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
    xwork_test_assert_readme_patch_stats(&tArtifact);

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
    xwork_test_assert_readme_patch_stats(&tLoadedArtifact);
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
    xwork_test_assert_readme_patch_stats(&tLoadedArtifact);

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
    xwork_test_assert_readme_patch_stats(&tPersistedRunIndex.pItems[0].tLastArtifact);
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
    xwork_test_assert_readme_patch_stats(&tPersistedRunIndex.pItems[1].tLastArtifact);

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
    xwork_test_assert_readme_patch_stats(&tLoadedArtifact);
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
    xwork_test_assert_readme_patch_stats(&tLoadedArtifact);

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
    xwork_test_assert_readme_patch_stats(&tLoadedArtifact);

    xwork_artifact_summary_query_init(&tArtifactSummaryQuery);
    tArtifactSummaryQuery.bHasKind = true;
    tArtifactSummaryQuery.eKind = XWORK_ARTIFACT_PATCH;
    xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
    assert(
        xwork_runtime_query_persisted_artifact_summaries(
            pFileRecoverRuntime,
            "run-file-persistence",
            &tArtifactSummaryQuery,
            &tPersistedArtifactSummaries
        ) == XWORK_OK
    );
    assert(tPersistedArtifactSummaries.iCount == 1u);
    xwork_test_assert_readme_patch_summary_stats(&tPersistedArtifactSummaries.pItems[0]);

    xwork_run_options_init(&tRunOptions);
    xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
    tRunOptions.sRunId = "run-file-artifact-query";
    tRunOptions.sInstruction = "Persist a command artifact for metadata query coverage.";
    tRunOptions.sLlmProfileId = "mock-profile";
    tRunOptions.sSessionProfileId = "mock-session";
    tRunOptions.psWorkspaceIds = asWorkspaceIds;
    tRunOptions.iWorkspaceCount = 1u;
    assert(
        xwork_run_create(
            pFileRecoverRuntime,
            &tRunOptions,
            &pFileArtifactQueryRun
        ) == XWORK_OK
    );
    xwork_command_artifact_options_init(&tCommandArtifactOptions);
    tCommandArtifactOptions.sName = "query-command.txt";
    tCommandArtifactOptions.sStorageRef = "host://process/query-command";
    tCommandArtifactOptions.sSummary = "Command artifact query coverage.";
    tCommandArtifactOptions.sCommandText = "query-command";
    tCommandArtifactOptions.sOutputText = "query output\n";
    tCommandArtifactOptions.bHasCommandIoStats = true;
    tCommandArtifactOptions.iStdoutByteCount = 13u;
    tCommandArtifactOptions.iStderrByteCount = 0u;
    tCommandArtifactOptions.bStdoutTruncated = false;
    tCommandArtifactOptions.bStderrTruncated = false;
    tCommandArtifactOptions.bHasExitCode = true;
    tCommandArtifactOptions.iExitCode = 7;
    xwork_artifact_reset(&tArtifact);
    assert(
        xwork_run_emit_command_artifact(
            pFileArtifactQueryRun,
            &tCommandArtifactOptions,
            &tArtifact
        ) == XWORK_OK
    );
    assert(tArtifact.bHasExitCode);
    assert(tArtifact.iExitCode == 7);
    assert(tArtifact.iSequence == 1u);
    xwork_test_assert_content_stats(&tArtifact, 13u, 1u);
    xwork_test_assert_command_io_stats(&tArtifact, 13u, 0u, false, false);

    xwork_artifact_summary_query_init(&tArtifactSummaryQuery);
    tArtifactSummaryQuery.bHasKind = true;
    tArtifactSummaryQuery.eKind = XWORK_ARTIFACT_COMMAND;
    tArtifactSummaryQuery.sArtifactName = "query-command.txt";
    tArtifactSummaryQuery.sMimeType = "text/plain";
    tArtifactSummaryQuery.sStorageRef = "host://process/query-command";
    tArtifactSummaryQuery.bRequireExitCode = true;
    tArtifactSummaryQuery.bHasExitCodeValue = true;
    tArtifactSummaryQuery.iExitCode = 7;
    tArtifactSummaryQuery.bHasMinSequence = true;
    tArtifactSummaryQuery.iMinSequence = tArtifact.iSequence;
    tArtifactSummaryQuery.bHasMaxSequence = true;
    tArtifactSummaryQuery.iMaxSequence = tArtifact.iSequence;
    xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
    assert(
        xwork_file_persistence_query_artifact_summaries(
            &tFilePersistenceRecoverStore,
            "run-file-artifact-query",
            &tArtifactSummaryQuery,
            &tPersistedArtifactSummaries
        ) == XWORK_OK
    );
    assert(tPersistedArtifactSummaries.iCount == 1u);
    assert(tPersistedArtifactSummaries.bHasMore == false);
    assert(
        strcmp(tPersistedArtifactSummaries.pItems[0].sName, "query-command.txt") == 0
    );
    assert(tPersistedArtifactSummaries.pItems[0].bHasExitCode);
    assert(tPersistedArtifactSummaries.pItems[0].iExitCode == 7);
    assert(tPersistedArtifactSummaries.pItems[0].iSequence == tArtifact.iSequence);
    xwork_test_assert_summary_content_stats(
        &tPersistedArtifactSummaries.pItems[0],
        13u,
        1u
    );
    xwork_test_assert_summary_command_io_stats(
        &tPersistedArtifactSummaries.pItems[0],
        13u,
        0u,
        false,
        false
    );

    tArtifactSummaryQuery.iExitCode = 0;
    xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
    assert(
        xwork_runtime_query_persisted_artifact_summaries(
            pFileRecoverRuntime,
            "run-file-artifact-query",
            &tArtifactSummaryQuery,
            &tPersistedArtifactSummaries
        ) == XWORK_OK
    );
    assert(tPersistedArtifactSummaries.iCount == 0u);

    tArtifactSummaryQuery.iExitCode = 7;
    tArtifactSummaryQuery.iMinSequence = tArtifact.iSequence + 1u;
    xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
    assert(
        xwork_runtime_query_persisted_artifact_summaries(
            pFileRecoverRuntime,
            "run-file-artifact-query",
            &tArtifactSummaryQuery,
            &tPersistedArtifactSummaries
        ) == XWORK_OK
    );
    assert(tPersistedArtifactSummaries.iCount == 0u);

    xwork_report_artifact_options_init(&tReportArtifactOptions);
    tReportArtifactOptions.sName = "query-plan.md";
    tReportArtifactOptions.sStorageRef = "workspace://PLAN.md";
    tReportArtifactOptions.sSummary = "Plan report artifact query coverage.";
    tReportArtifactOptions.eOutputClass = XWORK_ARTIFACT_OUTPUT_TEXT;
    tReportArtifactOptions.sOutputRole = "report.plan";
    tReportArtifactOptions.eReportClass = XWORK_ARTIFACT_REPORT_PLAN;
    tReportArtifactOptions.sReportSubjectRef = "workspace://README.md";
    tReportArtifactOptions.sReportText = "# Plan\n\n- inspect\n";
    xwork_artifact_reset(&tArtifact);
    assert(
        xwork_run_emit_report_artifact(
            pFileArtifactQueryRun,
            &tReportArtifactOptions,
            &tArtifact
        ) == XWORK_OK
    );
    assert(tArtifact.eKind == XWORK_ARTIFACT_REPORT);
    assert(tArtifact.eOutputClass == XWORK_ARTIFACT_OUTPUT_TEXT);
    assert(tArtifact.iSequence == 2u);
    xwork_test_assert_report_class(
        &tArtifact,
        XWORK_ARTIFACT_REPORT_PLAN,
        "workspace://README.md"
    );
    xwork_test_assert_content_stats(&tArtifact, 18u, 3u);

    xwork_artifact_summary_query_init(&tArtifactSummaryQuery);
    tArtifactSummaryQuery.bHasKind = true;
    tArtifactSummaryQuery.eKind = XWORK_ARTIFACT_REPORT;
    tArtifactSummaryQuery.bHasReportClass = true;
    tArtifactSummaryQuery.eReportClass = XWORK_ARTIFACT_REPORT_PLAN;
    tArtifactSummaryQuery.sReportSubjectRefPrefix = "workspace://";
    tArtifactSummaryQuery.sOutputRole = "report.plan";
    xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
    assert(
        xwork_runtime_query_persisted_artifact_summaries(
            pFileRecoverRuntime,
            "run-file-artifact-query",
            &tArtifactSummaryQuery,
            &tPersistedArtifactSummaries
        ) == XWORK_OK
    );
    assert(tPersistedArtifactSummaries.iCount == 1u);
    assert(strcmp(tPersistedArtifactSummaries.pItems[0].sName, "query-plan.md") == 0);
    xwork_test_assert_summary_report_class(
        &tPersistedArtifactSummaries.pItems[0],
        XWORK_ARTIFACT_REPORT_PLAN,
        "workspace://README.md"
    );
    xwork_test_assert_summary_content_stats(
        &tPersistedArtifactSummaries.pItems[0],
        18u,
        3u
    );

    tArtifactSummaryQuery.eReportClass = XWORK_ARTIFACT_REPORT_REVIEW;
    xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
    assert(
        xwork_file_persistence_query_artifact_summaries(
            &tFilePersistenceRecoverStore,
            "run-file-artifact-query",
            &tArtifactSummaryQuery,
            &tPersistedArtifactSummaries
        ) == XWORK_OK
    );
    assert(tPersistedArtifactSummaries.iCount == 0u);
    xwork_artifact_reset(&tArtifact);
    xwork_run_destroy(pFileArtifactQueryRun);
    pFileArtifactQueryRun = NULL;

    if ( xrtProcessTerminalSupported() ) {
        xwork_runtime_destroy(pFileRecoverRuntime);
        pFileRecoverRuntime = NULL;
        tLocalHostOptions.sDefaultWorkingDirectory = ".";
        xwork_host_services_init(&tLocalHostServices);
        assert(
            xwork_local_host_configure_services(
                &tLocalHost,
                &tLocalHostOptions,
                &tLocalHostServices
            ) == XWORK_OK
        );
        xwork_runtime_options_init(&tFileRuntimeOptions);
        tFileRuntimeOptions.pLlmRuntime = pLlmRuntime;
        tFileRuntimeOptions.pHostServices = &tLocalHostServices;
        tFileRuntimeOptions.pPersistenceBackend = &tFilePersistenceBackend;
        tFileRuntimeOptions.tPolicy = tPolicyOptions;
        assert(xwork_runtime_create(&tFileRuntimeOptions, &pFileRecoverRuntime) == XWORK_OK);
        xwork_workspace_options_init(&tWorkspaceOptions);
        tWorkspaceOptions.sWorkspaceId = "local-host-main";
        tWorkspaceOptions.sRootPath = ".";
        assert(
            xwork_runtime_add_workspace(
                pFileRecoverRuntime,
                &tWorkspaceOptions,
                &pFileRecoverWorkspace
            ) == XWORK_OK
        );
        assert(
            xwork_runtime_register_builtin_tool(
                pFileRecoverRuntime,
                XWORK_TOOL_PROCESS_START_TERMINAL
            ) == XWORK_OK
        );
        assert(
            xwork_runtime_register_builtin_tool(
                pFileRecoverRuntime,
                XWORK_TOOL_PROCESS_TERMINAL_READ
            ) == XWORK_OK
        );
        assert(
            xwork_runtime_register_builtin_tool(
                pFileRecoverRuntime,
                XWORK_TOOL_PROCESS_TERMINAL_WRITE
            ) == XWORK_OK
        );
        assert(
            xwork_runtime_register_builtin_tool(
                pFileRecoverRuntime,
                XWORK_TOOL_PROCESS_TERMINAL_RESIZE
            ) == XWORK_OK
        );
        assert(
            xwork_runtime_register_builtin_tool(
                pFileRecoverRuntime,
                XWORK_TOOL_PROCESS_TERMINAL_STOP
            ) == XWORK_OK
        );

        tAdapterCtx.iTurnCount = 0;
        tAdapterCtx.sExpectedMemoryText = NULL;
        tAdapterCtx.iObservedMemoryTurns = 0;
        asWorkspaceIds[0] = "local-host-main";
        xwork_run_options_init(&tRunOptions);
        xwork_test_init_custom_session_policy(&tRunOptions.tSessionPolicy);
        tRunOptions.sRunId = "run-file-terminal-persistence";
        tRunOptions.sInstruction = "Run the local interactive terminal session through host service.";
        tRunOptions.sLlmProfileId = "mock-profile";
        tRunOptions.sSessionProfileId = "mock-session";
        tRunOptions.psWorkspaceIds = asWorkspaceIds;
        tRunOptions.iWorkspaceCount = 1u;
        assert(
            xwork_run_create(
                pFileRecoverRuntime,
                &tRunOptions,
                &pFileRecoveredRun
            ) == XWORK_OK
        );
        xwork_orchestrator_options_init(&tExecOptions);
        tExecOptions.iMaxTurns = 8u;
        tExecOptions.bAutoApprove = true;
        assert(xwork_run_execute(pFileRecoveredRun, &tExecOptions) == XWORK_OK);
        assert(xwork_run_get_state(pFileRecoveredRun) == XWORK_RUN_COMPLETED);
        assert(
            strcmp(
                xwork_run_get_last_output_text(pFileRecoveredRun),
                "Host service terminal session completed."
            ) == 0
        );
        assert(xwork_run_get_artifact_count(pFileRecoveredRun) >= 6u);

        xwork_string_list_reset(&tPersistedArtifactIds);
        assert(
            xwork_file_persistence_list_artifacts(
                &tFilePersistenceRecoverStore,
                "run-file-terminal-persistence",
                &tPersistedArtifactIds
            ) == XWORK_OK
        );
        assert(tPersistedArtifactIds.iCount >= 6u);

        xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
        assert(
            xwork_file_persistence_list_artifact_summaries(
                &tFilePersistenceRecoverStore,
                "run-file-terminal-persistence",
                &tPersistedArtifactSummaries
            ) == XWORK_OK
        );
        assert(tPersistedArtifactSummaries.iCount >= 6u);
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[1].sName,
                "process.terminal_resize.json"
            ) == 0
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[3].sName,
                "process.terminal_write.json"
            ) == 0
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[4].sName,
                "process.terminal_read.json"
            ) == 0
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[5].sName,
                "process.terminal_stop.json"
            ) == 0
        );

        xwork_artifact_summary_query_init(&tArtifactSummaryQuery);
        tArtifactSummaryQuery.bHasKind = true;
        tArtifactSummaryQuery.eKind = XWORK_ARTIFACT_OUTPUT;
        tArtifactSummaryQuery.bHasOutputClass = true;
        tArtifactSummaryQuery.eOutputClass = XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE;
        tArtifactSummaryQuery.sOutputRolePrefix = "process.terminal_";
        tArtifactSummaryQuery.sNamePrefix = "process.terminal_";
        xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
        assert(
            xwork_file_persistence_query_artifact_summaries(
                &tFilePersistenceRecoverStore,
                "run-file-terminal-persistence",
                &tArtifactSummaryQuery,
                &tPersistedArtifactSummaries
            ) == XWORK_OK
        );
        assert(tPersistedArtifactSummaries.iCount == 4u);
        xwork_test_assert_summary_output_class(
            &tPersistedArtifactSummaries.pItems[0],
            XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE,
            XWORK_TOOL_PROCESS_TERMINAL_RESIZE
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[0].sName,
                "process.terminal_resize.json"
            ) == 0
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[1].sName,
                "process.terminal_write.json"
            ) == 0
        );
        xwork_test_assert_summary_output_class(
            &tPersistedArtifactSummaries.pItems[2],
            XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE,
            XWORK_TOOL_PROCESS_TERMINAL_READ
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[2].sName,
                "process.terminal_read.json"
            ) == 0
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[3].sName,
                "process.terminal_stop.json"
            ) == 0
        );
        assert(tPersistedArtifactSummaries.bHasMore == false);
        assert(
            tPersistedArtifactSummaries.iNextAfterSequence ==
            tPersistedArtifactSummaries.pItems[3].iSequence
        );

        tArtifactSummaryQuery.iLimit = 2u;
        xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
        assert(
            xwork_file_persistence_query_artifact_summaries(
                &tFilePersistenceRecoverStore,
                "run-file-terminal-persistence",
                &tArtifactSummaryQuery,
                &tPersistedArtifactSummaries
            ) == XWORK_OK
        );
        assert(tPersistedArtifactSummaries.iCount == 2u);
        assert(tPersistedArtifactSummaries.bHasMore == true);
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[0].sName,
                "process.terminal_resize.json"
            ) == 0
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[1].sName,
                "process.terminal_write.json"
            ) == 0
        );
        tArtifactSummaryQuery.bHasAfterSequence = true;
        tArtifactSummaryQuery.iAfterSequence = tPersistedArtifactSummaries.iNextAfterSequence;

        xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
        assert(
            xwork_runtime_query_persisted_artifact_summaries(
                pFileRecoverRuntime,
                "run-file-terminal-persistence",
                &tArtifactSummaryQuery,
                &tPersistedArtifactSummaries
            ) == XWORK_OK
        );
        assert(tPersistedArtifactSummaries.iCount == 2u);
        assert(tPersistedArtifactSummaries.bHasMore == false);
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[0].sName,
                "process.terminal_read.json"
            ) == 0
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[1].sName,
                "process.terminal_stop.json"
            ) == 0
        );
        assert(
            tPersistedArtifactSummaries.pItems[0].eKind == XWORK_ARTIFACT_OUTPUT
        );
        assert(
            tPersistedArtifactSummaries.pItems[0].sStorageRef != NULL
        );
        assert(
            tPersistedArtifactSummaries.pItems[0].sStorageRef[0] != '\0'
        );

        xwork_artifact_summary_query_init(&tArtifactSummaryQuery);
        tArtifactSummaryQuery.bHasKind = true;
        tArtifactSummaryQuery.eKind = XWORK_ARTIFACT_OUTPUT;
        tArtifactSummaryQuery.sArtifactName = "process.terminal_read.json";
        xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
        assert(
            xwork_runtime_query_persisted_artifact_summaries(
                pFileRecoverRuntime,
                "run-file-terminal-persistence",
                &tArtifactSummaryQuery,
                &tPersistedArtifactSummaries
            ) == XWORK_OK
        );
        assert(tPersistedArtifactSummaries.iCount == 1u);
        assert(tPersistedArtifactSummaries.bHasMore == false);
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[0].sName,
                "process.terminal_read.json"
            ) == 0
        );
        assert(
            tPersistedArtifactSummaries.pItems[0].sStorageRef != NULL &&
            tPersistedArtifactSummaries.pItems[0].sStorageRef[0] != '\0'
        );
        assert(
            strlen(tPersistedArtifactSummaries.pItems[0].sStorageRef) <
            sizeof(sTerminalStorageRefPrefix)
        );
        strcpy(
            sTerminalStorageRefPrefix,
            tPersistedArtifactSummaries.pItems[0].sStorageRef
        );

        tArtifactSummaryQuery.sArtifactName = NULL;
        tArtifactSummaryQuery.sMimeTypePrefix = "application/";
        tArtifactSummaryQuery.sStorageRefPrefix = sTerminalStorageRefPrefix;
        tArtifactSummaryQuery.sNamePrefix = "process.terminal_";
        xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
        assert(
            xwork_runtime_query_persisted_artifact_summaries(
                pFileRecoverRuntime,
                "run-file-terminal-persistence",
                &tArtifactSummaryQuery,
                &tPersistedArtifactSummaries
            ) == XWORK_OK
        );
        assert(tPersistedArtifactSummaries.iCount == 4u);
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[0].sName,
                "process.terminal_resize.json"
            ) == 0
        );
        assert(
            strcmp(
                tPersistedArtifactSummaries.pItems[0].sMimeType,
                "application/json"
            ) == 0
        );

        xwork_artifact_reset(&tLoadedArtifact);
        assert(
            xwork_file_persistence_find_artifact_by_name(
                &tFilePersistenceRecoverStore,
                "run-file-terminal-persistence",
                "process.terminal_resize.json",
                &tLoadedArtifact
            ) == XWORK_OK
        );
        assert(tLoadedArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        xwork_test_assert_output_class(
            &tLoadedArtifact,
            XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE,
            XWORK_TOOL_PROCESS_TERMINAL_RESIZE
        );
        assert(strcmp(tLoadedArtifact.sName, "process.terminal_resize.json") == 0);
        assert(tLoadedArtifact.sContentText != NULL);
        assert(strstr(tLoadedArtifact.sContentText, "\"resize_applied\":") != NULL);

        xwork_artifact_reset(&tLoadedArtifact);
        assert(
            xwork_file_persistence_find_artifact_by_name(
                &tFilePersistenceRecoverStore,
                "run-file-terminal-persistence",
                "process.terminal_write.json",
                &tLoadedArtifact
            ) == XWORK_OK
        );
        assert(tLoadedArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        assert(strcmp(tLoadedArtifact.sName, "process.terminal_write.json") == 0);
        assert(tLoadedArtifact.sContentText != NULL);
        assert(strstr(tLoadedArtifact.sContentText, "\"next_after_seq\":") != NULL);

        xwork_artifact_reset(&tLoadedArtifact);
        assert(
            xwork_runtime_find_persisted_artifact_by_name(
                pFileRecoverRuntime,
                "run-file-terminal-persistence",
                "process.terminal_read.json",
                &tLoadedArtifact
            ) == XWORK_OK
        );
        assert(tLoadedArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        xwork_test_assert_output_class(
            &tLoadedArtifact,
            XWORK_ARTIFACT_OUTPUT_TERMINAL_STATE,
            XWORK_TOOL_PROCESS_TERMINAL_READ
        );
        assert(strcmp(tLoadedArtifact.sName, "process.terminal_read.json") == 0);
        assert(tLoadedArtifact.sContentText != NULL);
        assert(strstr(tLoadedArtifact.sContentText, "\"event_end_seq\":") != NULL);

        xwork_artifact_reset(&tLoadedArtifact);
        assert(
            xwork_runtime_find_persisted_artifact_by_name(
                pFileRecoverRuntime,
                "run-file-terminal-persistence",
                "process.terminal_stop.json",
                &tLoadedArtifact
            ) == XWORK_OK
        );
        assert(tLoadedArtifact.eKind == XWORK_ARTIFACT_OUTPUT);
        assert(strcmp(tLoadedArtifact.sName, "process.terminal_stop.json") == 0);
        assert(tLoadedArtifact.sContentText != NULL);
        assert(strstr(tLoadedArtifact.sContentText, "\"removed\":true") != NULL);

        xwork_run_destroy(pFileRecoveredRun);
        pFileRecoveredRun = NULL;
    }

    xwork_run_snapshot_reset(&tRunSnapshot);
    xwork_run_snapshot_reset(&tPersistenceCtx.tSnapshot);
    xwork_run_destroy(pStreamCancelledRun);
    xwork_run_destroy(pStreamRun);
    xwork_run_destroy(pInterruptedRun);
    xwork_run_destroy(pFileArtifactQueryRun);
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
    xwork_artifact_summary_list_reset(&tPersistedArtifactSummaries);
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
    (void)remove(sMemorySyncWorkspaceFile);
    xwork_test_remove_empty_directory(sLocalHostCreateDirsDir);
    xwork_test_remove_empty_directory(sLocalHostOrchestratorCreateDirsDir);
    xwork_test_remove_empty_directory(sLocalHostCreateDirsParentDir);
    xwork_test_remove_empty_directory(sMemorySyncWorkspaceDir);
    free(sPendingBeforeToolCheckpointId);
    free(sAfterToolCheckpointId);
    free(sFileBeforeToolCheckpointId);
    free(sFirstArtifactId);
    xllm_memory_destroy(pFileWorkspaceMemory);
    xllm_memory_destroy(pWorkspaceMemory);
    xllm_runtime_destroy(pLlmRuntime);
    return 0;
}

