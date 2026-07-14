#include "../xwork.c"
#include "../../xllm/xllm.c"
#include "../../xllm/xllm-session.c"

static int g_iFailures = 0;

#define CHECK(expr, name) do { \
    bool xwork_ok__ = !!(expr); \
    printf("  %-62s %s\n", (name), xwork_ok__ ? "PASS" : "FAIL"); \
    if ( !xwork_ok__ ) ++g_iFailures; \
} while (0)

typedef struct mock_model {
    uint32_t uCompactionCalls;
    uint32_t uAgentCalls;
    bool bSawTools;
    bool bSawParallel;
    bool bSawToolResults;
    bool bSawCompactionSummary;
    bool bSawVerificationGate;
    uint32_t uPermissionCalls;
    bool bSawPathPermission;
    bool bSawCommandPermission;
    bool bSawHighRiskPermission;
    uint32_t uBeforeToolHooks;
    uint32_t uAfterToolHooks;
} mock_model;

typedef struct test_events {
    uint32_t uTextDeltas;
    uint32_t uToolStarts;
    uint32_t uToolDone;
    uint32_t uToolSuccess;
    uint32_t uToolFailure;
    uint32_t uCompactions;
    uint32_t uErrors;
    char sLastArtifact[512];
} test_events;

static char* test_strdup(const char* sText)
{
    size_t iLen = strlen(sText);
    char* sCopy = (char*)malloc(iLen + 1u);
    if ( sCopy ) memcpy(sCopy, sText, iLen + 1u);
    return sCopy;
}

static xllm_response* mock_response(const char* sContent, size_t iToolCount)
{
    xllm_response* pResponse = (xllm_response*)calloc(1u, sizeof(*pResponse));
    if ( !pResponse ) return NULL;
    pResponse->sContent = test_strdup(sContent ? sContent : "");
    pResponse->sFinishReason = test_strdup(iToolCount ? "tool_calls" : "stop");
    if ( iToolCount ) pResponse->pToolCalls = (xllm_tool_call*)calloc(iToolCount, sizeof(*pResponse->pToolCalls));
    if ( !pResponse->sContent || !pResponse->sFinishReason || (iToolCount && !pResponse->pToolCalls) ) {
        xllmResponseDestroy(pResponse);
        return NULL;
    }
    pResponse->iToolCallCount = iToolCount;
    pResponse->iToolCallCap = iToolCount;
    pResponse->tUsage.uInputTokens = 100u;
    pResponse->tUsage.uOutputTokens = 20u;
    pResponse->tUsage.uTotalTokens = 120u;
    return pResponse;
}

static bool mock_set_call(xllm_response* pResponse, size_t i, const char* sId, const char* sName, const char* sArgs)
{
    pResponse->pToolCalls[i].sId = test_strdup(sId);
    pResponse->pToolCalls[i].sName = test_strdup(sName);
    pResponse->pToolCalls[i].sArgumentsJson = test_strdup(sArgs);
    return pResponse->pToolCalls[i].sId && pResponse->pToolCalls[i].sName && pResponse->pToolCalls[i].sArgumentsJson;
}

static bool request_has_role(const xllm_request* pRequest, xllm_role eRole, size_t iAtLeast)
{
    size_t i;
    size_t iCount = 0u;
    for ( i = 0u; i < pRequest->iMessageCount; ++i ) {
        if ( pRequest->pMessages[i].eRole == eRole ) ++iCount;
    }
    return iCount >= iAtLeast;
}

static bool request_has_text(const xllm_request* pRequest, const char* sNeedle)
{
    size_t i;
    for ( i = 0u; i < pRequest->iMessageCount; ++i ) {
        const char* sText = pRequest->pMessages[i].sContent;
        if ( sText && strstr(sText, sNeedle) ) return true;
    }
    return false;
}

static xllm_result mock_complete(
    void* pUserData,
    const xllm_request* pRequest,
    const xllm_stream_callbacks* pCallbacks,
    xllm_response** ppResponse,
    xllm_error* pError
)
{
    mock_model* pMock = (mock_model*)pUserData;
    xllm_response* pResponse = NULL;
    char* sLargeArgs = NULL;
    size_t i;
    (void)pError;
    *ppResponse = NULL;
    if ( pRequest->iToolCount == 0u ) {
        ++pMock->uCompactionCalls;
        pResponse = mock_response(
            "Objective: test the xwork tool loop after compaction. Completed: old synthetic rounds. Constraints: remain inside the workspace. Next: execute the requested file workflow.",
            0u
        );
    } else {
        ++pMock->uAgentCalls;
        pMock->bSawTools = pRequest->iToolCount == 11u;
        pMock->bSawParallel = pRequest->bParallelToolCalls;
        if ( pMock->uAgentCalls > 1u && request_has_role(pRequest, XLLM_ROLE_TOOL, 1u) ) pMock->bSawToolResults = true;
        if ( request_has_text(pRequest, "Objective: test the xwork tool loop after compaction") ) pMock->bSawCompactionSummary = true;
        if ( request_has_text(pRequest, "Completion verification gate") ) pMock->bSawVerificationGate = true;
        if ( pMock->uAgentCalls == 1u ) {
            xwork_buf tArgs = {0};
            pResponse = mock_response("", 6u);
            if ( !pResponse ) return XLLM_RESULT_ERROR;
            if ( !xwork__buf_append_cstr(&tArgs, "{\"path\":\"sandbox/note.txt\",\"content\":\"hello-") ) goto oom;
            for ( i = 0u; i < 1600u; ++i ) if ( !xwork__buf_append_char(&tArgs, (char)('a' + (i % 26u))) ) goto oom;
            if ( !xwork__buf_append_cstr(&tArgs, "\",\"mode\":\"create\",\"create_dirs\":true}") ) goto oom;
            sLargeArgs = xwork__buf_detach(&tArgs);
            if ( !sLargeArgs ||
                 !mock_set_call(pResponse, 0u, "call_write", "write_file", sLargeArgs) ||
                 !mock_set_call(pResponse, 1u, "call_read", "read_file", "{\"path\":\"sandbox/note.txt\",\"max_lines\":20}") ||
                 !mock_set_call(pResponse, 2u, "call_escape", "read_file", "{\"path\":\"../outside.txt\"}") ||
                 !mock_set_call(pResponse, 3u, "call_list", "list_files", "{\"path\":\"sandbox\",\"recursive\":true}") ||
                 !mock_set_call(pResponse, 4u, "call_search", "search_text", "{\"query\":\"hello-\",\"path\":\"sandbox\",\"pattern\":\"*.txt\"}") ||
                 !mock_set_call(pResponse, 5u, "call_replace", "replace_text", "{\"path\":\"sandbox/note.txt\",\"old_text\":\"hello-\",\"new_text\":\"HELLO-\"}") ) goto oom;
        } else if ( pMock->uAgentCalls == 2u ) {
            pResponse = mock_response("", 1u);
            if ( !pResponse || !mock_set_call(pResponse, 0u, "call_patch", "apply_patch",
                    "{\"changes\":[{\"path\":\"sandbox/note.txt\",\"operation\":\"replace\",\"old_text\":\"HELLO-\",\"new_text\":\"PATCHED-\"},{\"path\":\"sandbox/extra.txt\",\"operation\":\"create\",\"content\":\"transaction created this file\\n\"}]}") ) goto oom;
        } else if ( pMock->uAgentCalls == 3u ) {
            pResponse = mock_response("The requested edits are complete.", 0u);
        } else if ( pMock->uAgentCalls == 4u ) {
            pResponse = mock_response("", 1u);
            if ( !pResponse || !mock_set_call(pResponse, 0u, "call_exec", "exec_command",
                    "{\"command\":\"type sandbox\\\\note.txt\",\"timeout_ms\":10000}") ) goto oom;
        } else {
            pResponse = mock_response("Implemented, inspected, edited, and verified the workspace file successfully.", 0u);
        }
    }
    free(sLargeArgs);
    if ( !pResponse ) return XLLM_RESULT_ERROR;
    if ( pCallbacks && pCallbacks->OnEvent && pResponse->sContent && pResponse->sContent[0] ) {
        xllm_event tEvent;
        memset(&tEvent, 0, sizeof(tEvent));
        tEvent.eKind = XLLM_EVENT_TEXT_DELTA;
        tEvent.as.tText.sData = pResponse->sContent;
        tEvent.as.tText.iLen = strlen(pResponse->sContent);
        if ( !pCallbacks->OnEvent(pCallbacks->pUserData, &tEvent) ) {
            xllmResponseDestroy(pResponse);
            return XLLM_RESULT_CANCELLED;
        }
    }
    *ppResponse = pResponse;
    return XLLM_RESULT_OK;
oom:
    free(sLargeArgs);
    xllmResponseDestroy(pResponse);
    return XLLM_RESULT_ERROR;
}

static bool on_event(void* pUserData, const xwork_event* pEvent)
{
    test_events* pEvents = (test_events*)pUserData;
    switch ( pEvent->eKind ) {
        case XWORK_EVENT_MODEL_TEXT_DELTA: ++pEvents->uTextDeltas; break;
        case XWORK_EVENT_TOOL_START: ++pEvents->uToolStarts; break;
        case XWORK_EVENT_TOOL_DONE:
            ++pEvents->uToolDone;
            if ( pEvent->bSuccess ) ++pEvents->uToolSuccess;
            else ++pEvents->uToolFailure;
            if ( pEvent->sArtifactPath ) snprintf(pEvents->sLastArtifact, sizeof(pEvents->sLastArtifact), "%s", pEvent->sArtifactPath);
            break;
        case XWORK_EVENT_COMPACTION_DONE: ++pEvents->uCompactions; break;
        case XWORK_EVENT_ERROR: ++pEvents->uErrors; break;
        default: break;
    }
    return true;
}

static xwork_permission_decision on_permission(void* pUserData, const xwork_permission_request* pRequest)
{
    mock_model* pMock = (mock_model*)pUserData;
    ++pMock->uPermissionCalls;
    if ( pRequest->eResourceKind == XWORK_RESOURCE_PATH && pRequest->sResource && strstr(pRequest->sResource, "sandbox") ) {
        pMock->bSawPathPermission = true;
    }
    if ( pRequest->eResourceKind == XWORK_RESOURCE_COMMAND && pRequest->sResource && strstr(pRequest->sResource, "type sandbox") ) {
        pMock->bSawCommandPermission = true;
    }
    if ( pRequest->eRisk == XWORK_RISK_HIGH ) pMock->bSawHighRiskPermission = true;
    return XWORK_PERMISSION_DEFAULT;
}

static xwork_hook_action on_hook(void* pUserData, const xwork_hook_event* pEvent)
{
    mock_model* pMock = (mock_model*)pUserData;
    if ( pEvent->ePhase == XWORK_HOOK_BEFORE_TOOL ) ++pMock->uBeforeToolHooks;
    else if ( pEvent->ePhase == XWORK_HOOK_AFTER_TOOL ) {
        ++pMock->uAfterToolHooks;
        if ( !pEvent->sOutput ) return XWORK_HOOK_CANCEL;
    }
    return XWORK_HOOK_CONTINUE;
}

static char* make_text(size_t iSize, char ch)
{
    char* sText = (char*)malloc(iSize + 1u);
    if ( !sText ) return NULL;
    memset(sText, ch, iSize);
    sText[iSize] = '\0';
    return sText;
}

static void test_process_text_normalization(void)
{
    static const unsigned char sOverlong[] = {0xC0u, 0xAFu};
    static const unsigned char sSurrogate[] = {0xEDu, 0xA0u, 0x80u};
    static const unsigned char sTooHigh[] = {0xF4u, 0x90u, 0x80u, 0x80u};
    static const unsigned char sTruncated[] = {0xE2u, 0x82u};
    static const unsigned char sLocalBytes[] = {
        0xCFu, 0xB5u, 0xCDu, 0xB3u, 0xD5u, 0xD2u, 0xB2u, 0xBBu,
        0xB5u, 0xBDu, 0xD6u, 0xB8u, 0xB6u, 0xA8u, 0xB5u, 0xC4u
    };
    static const unsigned char sBinary[] = {'a', 0u, 0xFFu, 'b'};
    xwork_buf tText = {0};

    CHECK(xrtIsUTF8((str)"valid UTF-8: \xE4\xB8\xAD\xE6\x96\x87", 0u), "strict UTF-8 accepts valid Unicode text");
    CHECK(!xrtIsUTF8((str)sOverlong, sizeof(sOverlong)), "strict UTF-8 rejects overlong encodings");
    CHECK(!xrtIsUTF8((str)sSurrogate, sizeof(sSurrogate)), "strict UTF-8 rejects surrogate code points");
    CHECK(!xrtIsUTF8((str)sTooHigh, sizeof(sTooHigh)), "strict UTF-8 rejects code points above U+10FFFF");
    CHECK(!xrtIsUTF8((str)sTruncated, sizeof(sTruncated)), "strict UTF-8 rejects truncated sequences");

    CHECK(xwork__buf_append_process_text(&tText, sLocalBytes, sizeof(sLocalBytes)), "local process bytes normalize without loss of control flow");
    CHECK(tText.pData && xrtIsUTF8((str)tText.pData, tText.iLen), "normalized process output is valid UTF-8");
    xwork__buf_unit(&tText);

    CHECK(xwork__buf_append_process_text(&tText, sBinary, sizeof(sBinary)), "binary-like process output is safely represented");
    CHECK(tText.pData && strstr(tText.pData, "\\x00") && strstr(tText.pData, "\\xFF"), "binary bytes are escaped instead of entering JSON");
    CHECK(tText.pData && xrtIsUTF8((str)tText.pData, tText.iLen), "escaped binary representation remains valid UTF-8");
    xwork__buf_unit(&tText);
}

static void test_agent_loop(void)
{
    static const char sWorkspace[] = "tests/tmp_xwork";
    static const char sSessionPath[] = "tests/tmp_xwork/.xcode/session.json";
    xllm_session_config tSessionConfig;
    xllm_session* pSession;
    xllm_error tLlmError;
    xwork_agent_config tAgentConfig;
    xwork_agent* pAgent;
    xwork_error tError;
    xwork_run_result tResult;
    mock_model tMock;
    test_events tEvents;
    char* sOld = make_text(1300u, 'x');
    char* sFile = NULL;
    char* sArtifactAbsolute = NULL;
    size_t iFileSize = 0u;
    uint32_t uCompactionsBeforeExplicit = 0u;
    const xwork_tool_entry* pPatchTool;
    const xwork_tool_entry* pStartTool;
    const xwork_tool_entry* pWriteProcessTool;
    const xwork_tool_entry* pPollTool;
    xwork_tool_context tPatchContext;
    xwork_tool_output tPatchOutput;
    xwork_tool_output tProcessOutput;
    unsigned long long uManagedId = 0u;
    uint64_t uInterruptedTurn = 0u;
    unsigned i;

    memset(&tMock, 0, sizeof(tMock));
    memset(&tEvents, 0, sizeof(tEvents));
    memset(&tResult, 0, sizeof(tResult));
    (void)xrtDirDelete((str)sWorkspace);
    CHECK(xrtDirCreateAll((str)sWorkspace), "test workspace created");

    xllmSessionConfigInit(&tSessionConfig);
    tSessionConfig.uContextWindowTokens = 5000u;
    tSessionConfig.uMaxOutputTokens = 600u;
    tSessionConfig.uSafetyReserveTokens = 100u;
    tSessionConfig.uRecentTurnsToKeep = 1u;
    tSessionConfig.uToolPruneBytes = 512u;
    tSessionConfig.uSummaryMaxTokens = 400u;
    tSessionConfig.fPruneTrigger = 0.20;
    tSessionConfig.fCompactTrigger = 0.32;
    pSession = xllmSessionCreate(&tSessionConfig, &tLlmError);
    CHECK(pSession != NULL, "small session created for deterministic compaction");
    if ( !pSession || !sOld ) goto cleanup;
    CHECK(xllmSessionAddText(pSession, 0u, XLLM_ROLE_SYSTEM, "Pinned test agent contract.", XLLM_SESSION_ENTRY_PINNED), "pinned system message added");
    for ( i = 0u; i < 7u; ++i ) {
        uint64_t uTurn = xllmSessionBeginTurn(pSession);
        CHECK(xllmSessionAddText(pSession, uTurn, XLLM_ROLE_USER, sOld, 0u), "old user context added");
        CHECK(xllmSessionAddText(pSession, uTurn, XLLM_ROLE_ASSISTANT, sOld, 0u), "old assistant context added");
    }

    xworkAgentConfigInit(&tAgentConfig);
    CHECK(tAgentConfig.uMaxAgentTurns == 0u, "agent turns are unlimited by default");
    tAgentConfig.pSession = pSession;
    tAgentConfig.sWorkspaceRoot = sWorkspace;
    tAgentConfig.sSessionPath = sSessionPath;
    tAgentConfig.OnModelComplete = mock_complete;
    tAgentConfig.pModelUserData = &tMock;
    tAgentConfig.OnEvent = on_event;
    tAgentConfig.pEventUserData = &tEvents;
    tAgentConfig.OnPermission = on_permission;
    tAgentConfig.pPermissionUserData = &tMock;
    tAgentConfig.OnHook = on_hook;
    tAgentConfig.pHookUserData = &tMock;
    tAgentConfig.iMaxInlineToolBytes = 300u;
    pAgent = xworkAgentCreate(&tAgentConfig, &tError);
    CHECK(pAgent != NULL, "agent creates with injected model boundary");
    CHECK(pAgent && xworkAgentToolCount(pAgent) == 11u, "eleven practical builtin tools registered");
    if ( !pAgent ) goto cleanup;

    CHECK(xworkAgentRun(pAgent, "Create and verify the requested note file.", &tResult, &tError) == XWORK_RESULT_OK, "multi-turn tool loop completes");
    CHECK(tResult.sFinalText && strstr(tResult.sFinalText, "verified"), "final assistant response returned");
    CHECK(tResult.uAgentTurns == 5u && tResult.uToolCalls == 8u, "verification gate adds one model turn while eight tool calls run");
    CHECK(tResult.uCompactions >= 1u && tMock.uCompactionCalls == tResult.uCompactions, "context compaction occurred before or during tool work");
    CHECK(tEvents.uCompactions == tResult.uCompactions && tEvents.uErrors == 0u, "compaction events emitted without agent errors");
    CHECK(tMock.bSawTools && tMock.bSawParallel && tMock.bSawToolResults, "tool definitions, parallel flag, and continuity reach model");
    CHECK(tMock.bSawCompactionSummary, "post-compaction model turns receive the summary checkpoint");
    CHECK(tMock.bSawVerificationGate, "premature completion receives a durable verification-gate prompt");
    CHECK(tMock.uPermissionCalls == 8u && tMock.bSawPathPermission && tMock.bSawCommandPermission && tMock.bSawHighRiskPermission,
        "structured permission callback sees every tool plus path, command, and risk metadata");
    CHECK(tMock.uBeforeToolHooks == 8u && tMock.uAfterToolHooks == 8u, "before/after tool hooks bracket every executed tool");
    CHECK(tEvents.uToolStarts == 8u && tEvents.uToolDone == 8u, "tool lifecycle events are balanced");
    CHECK(tEvents.uToolSuccess == 7u && tEvents.uToolFailure == 1u, "seven builtin operations succeed and escape is a tool-level failure");
    CHECK(tEvents.sLastArtifact[0] != '\0', "oversized tool output spills to an artifact");
    CHECK(xrtFileExists((str)sSessionPath), "session autosaves atomically during the run");

    memset(&tPatchContext, 0, sizeof(tPatchContext));
    xworkToolOutputInit(&tPatchOutput);
    pPatchTool = xwork__find_tool(pAgent, "apply_patch");
    tPatchContext.pAgent = pAgent;
    CHECK(pPatchTool && pPatchTool->OnExecute(pPatchTool->pUserData, &tPatchContext,
        "{\"changes\":[{\"path\":\"sandbox/note.txt\",\"operation\":\"replace\",\"old_text\":\"PATCHED-\",\"new_text\":\"BROKEN-\"},{\"path\":\"sandbox/note.txt/child.txt\",\"operation\":\"create\",\"content\":\"must fail\"}]}",
        &tPatchOutput, &tError) == XWORK_RESULT_OK && !tPatchOutput.bSuccess,
        "failed multi-file transaction is reported as a tool-level failure");
    CHECK(tPatchOutput.sContent && strstr(tPatchOutput.sContent, "rollback completed"), "failed transaction reports successful rollback");
    xworkToolOutputUnit(&tPatchOutput);

    sFile = (char*)xrtFileReadAll((str)"tests/tmp_xwork/sandbox/note.txt", XRT_CP_UTF8, &iFileSize);
    CHECK(sFile && iFileSize > 1600u && strncmp(sFile, "PATCHED-", 8u) == 0, "workspace file was created then edited by both edit tools");
    CHECK(xrtFileExists((str)"tests/tmp_xwork/sandbox/extra.txt"), "multi-file patch transaction created its second target");
    CHECK(!xrtFileExists((str)"tests/tmp_xwork/sandbox/note.txt/child.txt"), "failed transaction left no partial target behind");

    pStartTool = xwork__find_tool(pAgent, "start_process");
    pWriteProcessTool = xwork__find_tool(pAgent, "write_process");
    pPollTool = xwork__find_tool(pAgent, "poll_process");
    xworkToolOutputInit(&tProcessOutput);
#if defined(_WIN32)
    CHECK(pStartTool && pStartTool->OnExecute(pStartTool->pUserData, &tPatchContext,
        "{\"command\":\"findstr persistent\",\"wait_ms\":0}", &tProcessOutput, &tError) == XWORK_RESULT_OK &&
        tProcessOutput.bSuccess && tProcessOutput.sContent && sscanf(tProcessOutput.sContent, "process_id: %llu", &uManagedId) == 1,
        "managed process starts and returns a stable process id");
#else
    CHECK(pStartTool && pStartTool->OnExecute(pStartTool->pUserData, &tPatchContext,
        "{\"command\":\"grep persistent\",\"wait_ms\":0}", &tProcessOutput, &tError) == XWORK_RESULT_OK &&
        tProcessOutput.bSuccess && tProcessOutput.sContent && sscanf(tProcessOutput.sContent, "process_id: %llu", &uManagedId) == 1,
        "managed process starts and returns a stable process id");
#endif
    xworkToolOutputUnit(&tProcessOutput);
    xworkToolOutputInit(&tProcessOutput);
    if ( uManagedId ) {
        char sProcessArgs[512];
        snprintf(sProcessArgs, sizeof(sProcessArgs),
            "{\"process_id\":%llu,\"input\":\"persistent hello\",\"append_newline\":true,\"close_stdin\":true}", uManagedId);
        CHECK(pWriteProcessTool && pWriteProcessTool->OnExecute(pWriteProcessTool->pUserData, &tPatchContext,
            sProcessArgs, &tProcessOutput, &tError) == XWORK_RESULT_OK && tProcessOutput.bSuccess,
            "managed process accepts stdin and an explicit stdin close");
        xworkToolOutputUnit(&tProcessOutput);
        xworkToolOutputInit(&tProcessOutput);
        snprintf(sProcessArgs, sizeof(sProcessArgs),
            "{\"process_id\":%llu,\"wait_ms\":5000,\"release\":true}", uManagedId);
        CHECK(pPollTool && pPollTool->OnExecute(pPollTool->pUserData, &tPatchContext,
            sProcessArgs, &tProcessOutput, &tError) == XWORK_RESULT_OK && tProcessOutput.bSuccess &&
            tProcessOutput.sContent && strstr(tProcessOutput.sContent, "persistent hello") && strstr(tProcessOutput.sContent, "state: exited"),
            "managed process poll returns incremental output and final exit state");
        CHECK(pAgent->iProcessCount == 0u, "released managed process leaves no live registry entry");
    }
    xworkToolOutputUnit(&tProcessOutput);

    if ( tEvents.sLastArtifact[0] ) {
        sArtifactAbsolute = (char*)xrtPathJoin(2u, sWorkspace, tEvents.sLastArtifact);
        CHECK(sArtifactAbsolute && xrtFileExists((str)sArtifactAbsolute), "full oversized output artifact exists");
    }
    CHECK(!xrtFileExists((str)"tests/outside.txt"), "workspace escape tool call could not access outside path");
    CHECK(tError.eCode == XWORK_ERROR_NONE, "successful run does not leak a stale recoverable tool error");
    uCompactionsBeforeExplicit = tMock.uCompactionCalls;
    CHECK(xworkAgentCompact(pAgent, &tError) == XWORK_RESULT_OK, "explicit safe-prefix compaction completes after the run");
    CHECK(tMock.uCompactionCalls == uCompactionsBeforeExplicit + 1u && tEvents.uCompactions == tMock.uCompactionCalls,
        "explicit compaction adds one model summary and lifecycle event");

    {
        xllm_tool_call arrInterruptedCalls[2];
        xllm_response tInterruptedResponse;
        xllm_session_stats tInterruptedStats;
        const char* sVerifyArgs;
        memset(arrInterruptedCalls, 0, sizeof(arrInterruptedCalls));
        memset(&tInterruptedResponse, 0, sizeof(tInterruptedResponse));
        arrInterruptedCalls[0].sId = "call_recovered_list";
        arrInterruptedCalls[0].sName = "list_files";
        arrInterruptedCalls[0].sArgumentsJson = "{\"path\":\"sandbox\"}";
        arrInterruptedCalls[1].sId = "call_recovered_verify";
        arrInterruptedCalls[1].sName = "exec_command";
#if defined(_WIN32)
        sVerifyArgs = "{\"command\":\"type sandbox\\\\note.txt\"}";
#else
        sVerifyArgs = "{\"command\":\"cat sandbox/note.txt\"}";
#endif
        arrInterruptedCalls[1].sArgumentsJson = (char*)sVerifyArgs;
        tInterruptedResponse.pToolCalls = arrInterruptedCalls;
        tInterruptedResponse.iToolCallCount = 2u;
        uInterruptedTurn = xllmSessionBeginTurn(pSession);
        CHECK(uInterruptedTurn &&
            xllmSessionAddText(pSession, uInterruptedTurn, XLLM_ROLE_USER, "Finish this interrupted verification batch.", 0u) &&
            xllmSessionAddAssistantResponse(pSession, uInterruptedTurn, &tInterruptedResponse) &&
            xllmSessionAddToolResult(pSession, uInterruptedTurn, "call_recovered_list", "status: success"),
            "interrupted batch fixture records one completed and one pending tool");
        CHECK(xllmSessionGetStats(pSession, &tInterruptedStats) && tInterruptedStats.uPendingToolCalls == 1u,
            "interrupted batch exposes one pending tool before resume");
        xworkRunResultUnit(&tResult);
        CHECK(xworkAgentRun(pAgent, "This prompt must not be appended.", &tResult, &tError) == XWORK_RESULT_ERROR &&
            tError.eCode == XWORK_ERROR_CONTEXT && xllmSessionCurrentTurn(pSession) == uInterruptedTurn,
            "normal run refuses to duplicate a prompt over interrupted work");
        CHECK(xworkAgentResume(pAgent, &tResult, &tError) == XWORK_RESULT_OK,
            "resume executes the pending tool and continues the model loop");
        CHECK(tResult.uToolCalls == 1u && tResult.uModelCalls == 1u &&
            tResult.sFinalText && strstr(tResult.sFinalText, "verified"),
            "resumed run reports only newly recovered work and returns final text");
        CHECK(xllmSessionGetStats(pSession, &tInterruptedStats) && tInterruptedStats.uPendingToolCalls == 0u,
            "resumed run durably resolves the pending tool call");
    }

    xworkRunResultUnit(&tResult);
    xworkAgentDestroy(pAgent);
cleanup:
    if ( sFile && iFileSize ) xrtFree(sFile);
    if ( sArtifactAbsolute ) xrtFree(sArtifactAbsolute);
    free(sOld);
    xllmSessionDestroy(pSession);
    (void)xrtDirDelete((str)sWorkspace);
}

int main(void)
{
    printf("xwork v2 tests\n");
    if ( !xrtInit() ) {
        fprintf(stderr, "xrtInit failed\n");
        return 1;
    }
    test_process_text_normalization();
    test_agent_loop();
    xrtNetSyncShutdownHiddenEngine();
    xrtUnit();
    printf("xwork v2: %s (%d failures)\n", g_iFailures ? "FAIL" : "PASS", g_iFailures);
    return g_iFailures ? 1 : 0;
}
