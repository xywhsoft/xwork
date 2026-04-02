#include "../xwork_core/xwork_internal.h"

static const xwork_tool_def *xwork__get_builtin_tool_def(const char *sToolId)
{
    static const xwork_tool_def tFilesystemReadText = {
        XWORK_TOOL_FILESYSTEM_READ_TEXT,
        "Filesystem Read Text",
        "Read text from a workspace file.",
        XWORK_TOOL_HOST_SERVICE,
        XWORK_HOST_FILESYSTEM,
        XWORK_HOST_FILESYSTEM_READ_TEXT,
        XWORK_SIDE_EFFECT_READ_ONLY,
        XWORK_APPROVAL_DEFAULT,
        false
    };
    static const xwork_tool_def tFilesystemWriteText = {
        XWORK_TOOL_FILESYSTEM_WRITE_TEXT,
        "Filesystem Write Text",
        "Write text into a workspace file.",
        XWORK_TOOL_HOST_SERVICE,
        XWORK_HOST_FILESYSTEM,
        XWORK_HOST_FILESYSTEM_WRITE_TEXT,
        XWORK_SIDE_EFFECT_WORKSPACE_WRITE,
        XWORK_APPROVAL_ALWAYS,
        false
    };
    static const xwork_tool_def tProcessExec = {
        XWORK_TOOL_PROCESS_EXEC,
        "Process Exec",
        "Run a process command and capture output.",
        XWORK_TOOL_HOST_SERVICE,
        XWORK_HOST_PROCESS,
        XWORK_HOST_PROCESS_EXEC,
        XWORK_SIDE_EFFECT_PROCESS_EXEC,
        XWORK_APPROVAL_ALWAYS,
        false
    };
    static const xwork_tool_def tProcessStartTerminal = {
        XWORK_TOOL_PROCESS_START_TERMINAL,
        "Process Start Terminal",
        "Start an interactive terminal-backed process session.",
        XWORK_TOOL_HOST_SERVICE,
        XWORK_HOST_PROCESS,
        XWORK_HOST_PROCESS_START_TERMINAL,
        XWORK_SIDE_EFFECT_PROCESS_EXEC,
        XWORK_APPROVAL_ALWAYS,
        false
    };
    static const xwork_tool_def tProcessTerminalRead = {
        XWORK_TOOL_PROCESS_TERMINAL_READ,
        "Process Terminal Read",
        "Read ordered events from an interactive terminal session.",
        XWORK_TOOL_HOST_SERVICE,
        XWORK_HOST_PROCESS,
        XWORK_HOST_PROCESS_TERMINAL_READ,
        XWORK_SIDE_EFFECT_READ_ONLY,
        XWORK_APPROVAL_DEFAULT,
        false
    };
    static const xwork_tool_def tProcessTerminalWrite = {
        XWORK_TOOL_PROCESS_TERMINAL_WRITE,
        "Process Terminal Write",
        "Write text into an interactive terminal session.",
        XWORK_TOOL_HOST_SERVICE,
        XWORK_HOST_PROCESS,
        XWORK_HOST_PROCESS_TERMINAL_WRITE,
        XWORK_SIDE_EFFECT_PROCESS_EXEC,
        XWORK_APPROVAL_ALWAYS,
        false
    };
    static const xwork_tool_def tProcessTerminalResize = {
        XWORK_TOOL_PROCESS_TERMINAL_RESIZE,
        "Process Terminal Resize",
        "Resize an interactive terminal session.",
        XWORK_TOOL_HOST_SERVICE,
        XWORK_HOST_PROCESS,
        XWORK_HOST_PROCESS_TERMINAL_RESIZE,
        XWORK_SIDE_EFFECT_READ_ONLY,
        XWORK_APPROVAL_DEFAULT,
        false
    };
    static const xwork_tool_def tProcessTerminalStop = {
        XWORK_TOOL_PROCESS_TERMINAL_STOP,
        "Process Terminal Stop",
        "Stop and remove an interactive terminal session.",
        XWORK_TOOL_HOST_SERVICE,
        XWORK_HOST_PROCESS,
        XWORK_HOST_PROCESS_TERMINAL_STOP,
        XWORK_SIDE_EFFECT_PROCESS_EXEC,
        XWORK_APPROVAL_ALWAYS,
        false
    };
    static const xwork_tool_def tVcsStatus = {
        XWORK_TOOL_VCS_STATUS,
        "VCS Status",
        "Read version-control status for a workspace path.",
        XWORK_TOOL_HOST_SERVICE,
        XWORK_HOST_VCS,
        XWORK_HOST_VCS_STATUS,
        XWORK_SIDE_EFFECT_READ_ONLY,
        XWORK_APPROVAL_DEFAULT,
        false
    };

    if ( !sToolId || !sToolId[0] ) {
        return NULL;
    }
    if ( strcmp(sToolId, XWORK_TOOL_FILESYSTEM_READ_TEXT) == 0 ) {
        return &tFilesystemReadText;
    }
    if ( strcmp(sToolId, XWORK_TOOL_FILESYSTEM_WRITE_TEXT) == 0 ) {
        return &tFilesystemWriteText;
    }
    if ( strcmp(sToolId, XWORK_TOOL_PROCESS_EXEC) == 0 ) {
        return &tProcessExec;
    }
    if ( strcmp(sToolId, XWORK_TOOL_PROCESS_START_TERMINAL) == 0 ) {
        return &tProcessStartTerminal;
    }
    if ( strcmp(sToolId, XWORK_TOOL_PROCESS_TERMINAL_READ) == 0 ) {
        return &tProcessTerminalRead;
    }
    if ( strcmp(sToolId, XWORK_TOOL_PROCESS_TERMINAL_WRITE) == 0 ) {
        return &tProcessTerminalWrite;
    }
    if ( strcmp(sToolId, XWORK_TOOL_PROCESS_TERMINAL_RESIZE) == 0 ) {
        return &tProcessTerminalResize;
    }
    if ( strcmp(sToolId, XWORK_TOOL_PROCESS_TERMINAL_STOP) == 0 ) {
        return &tProcessTerminalStop;
    }
    if ( strcmp(sToolId, XWORK_TOOL_VCS_STATUS) == 0 ) {
        return &tVcsStatus;
    }
    return NULL;
}

xwork_status xwork_runtime_register_tool(
    xwork_runtime *pRuntime,
    const xwork_tool_def *pDef
)
{
    xwork_tool_record *pTool;

    if ( !pRuntime || !pDef || !pDef->sToolId || !pDef->sToolId[0] ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( pDef->eHostService != XWORK_HOST_NONE &&
         (!pDef->sOperationId || !pDef->sOperationId[0]) ) {
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

const xwork_tool_def *xwork_get_builtin_tool_def(const char *sToolId)
{
    return xwork__get_builtin_tool_def(sToolId);
}

xwork_status xwork_runtime_register_builtin_tool(
    xwork_runtime *pRuntime,
    const char *sToolId
)
{
    const xwork_tool_def *pDef = xwork__get_builtin_tool_def(sToolId);

    if ( !pDef ) {
        return XWORK_ERROR_NOT_FOUND;
    }
    return xwork_runtime_register_tool(pRuntime, pDef);
}
