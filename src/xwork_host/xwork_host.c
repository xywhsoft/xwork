#include "../xwork_core/xwork_internal.h"
#include "../../lib/xrt.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define XWORK__HOST_POPEN _popen
#define XWORK__HOST_PCLOSE _pclose
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#define XWORK__HOST_POPEN popen
#define XWORK__HOST_PCLOSE pclose
#endif

#define XWORK__LOCAL_HOST_DEFAULT_READ_BYTES (64u * 1024u)
#define XWORK__LOCAL_HOST_DEFAULT_PROCESS_INPUT_BYTES (64u * 1024u)
#define XWORK__LOCAL_HOST_DEFAULT_PROCESS_ENV_ENTRIES 16u
#define XWORK__LOCAL_HOST_DEFAULT_PROCESS_OUTPUT_BYTES (64u * 1024u)
#define XWORK__LOCAL_HOST_DEFAULT_TERMINAL_COLS 120u
#define XWORK__LOCAL_HOST_DEFAULT_TERMINAL_ROWS 30u
#define XWORK__LOCAL_HOST_PROCESS_TIMEOUT_GRACE_MS 100u

static const xwork_host_service *xwork__runtime_get_host_service_slot(
    const xwork_runtime *pRuntime,
    xwork_host_service_kind eKind
)
{
    if ( !pRuntime ) {
        return NULL;
    }

    switch ( eKind ) {
        case XWORK_HOST_NONE:
            return NULL;
        case XWORK_HOST_FILESYSTEM:
            return &pRuntime->tHostServices.tFilesystem;
        case XWORK_HOST_PROCESS:
            return &pRuntime->tHostServices.tProcess;
        case XWORK_HOST_VCS:
            return &pRuntime->tHostServices.tVcs;
        case XWORK_HOST_DIAGNOSTICS:
            return &pRuntime->tHostServices.tDiagnostics;
        case XWORK_HOST_EDITOR:
            return &pRuntime->tHostServices.tEditor;
        default:
            return NULL;
    }
}

static bool xwork__local_host_is_absolute_path(const char *sPath)
{
    if ( !sPath || !sPath[0] ) {
        return false;
    }

#ifdef _WIN32
    if ( ((unsigned char)sPath[0] != 0u) &&
         isalpha((unsigned char)sPath[0]) &&
         sPath[1] == ':' &&
         (sPath[2] == '/' || sPath[2] == '\\') ) {
        return true;
    }
    if ( sPath[0] == '\\' && sPath[1] == '\\' ) {
        return true;
    }
#endif

    return sPath[0] == '/';
}

static char *xwork__local_host_join_path(const char *sBasePath, const char *sPath)
{
    size_t iBaseLength;

    if ( !sBasePath || !sBasePath[0] ) {
        return xwork__dup_cstr(sPath);
    }
    if ( !sPath || !sPath[0] ) {
        return xwork__dup_cstr(sBasePath);
    }

    iBaseLength = strlen(sBasePath);
    if ( sBasePath[iBaseLength - 1u] == '/' || sBasePath[iBaseLength - 1u] == '\\' ) {
        return xwork__dup_printf("%s%s", sBasePath, sPath);
    }
    return xwork__dup_printf("%s/%s", sBasePath, sPath);
}

static char *xwork__local_host_resolve_path(
    const xwork_local_host *pHost,
    const char *sPath
)
{
    if ( !sPath || !sPath[0] ) {
        return NULL;
    }
    if ( xwork__local_host_is_absolute_path(sPath) ) {
        return xwork__dup_cstr(sPath);
    }
    return xwork__local_host_join_path(
        pHost ? pHost->sDefaultWorkingDirectory : NULL,
        sPath
    );
}

static xwork_status xwork__local_host_json_escape(
    const char *sText,
    char **ppsEscaped
)
{
    char *sEscaped;
    size_t iLength = 0u;
    size_t i;

    if ( !ppsEscaped ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppsEscaped = NULL;
    if ( !sText ) {
        sText = "";
    }

    for ( i = 0u; sText[i] != '\0'; ++i ) {
        unsigned char c = (unsigned char)sText[i];

        switch ( c ) {
            case '"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                iLength += 2u;
                break;
            default:
                if ( c < 0x20u ) {
                    iLength += 6u;
                } else {
                    ++iLength;
                }
                break;
        }
    }

    sEscaped = (char *)malloc(iLength + 1u);
    if ( !sEscaped ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    iLength = 0u;
    for ( i = 0u; sText[i] != '\0'; ++i ) {
        unsigned char c = (unsigned char)sText[i];

        switch ( c ) {
            case '"':
                sEscaped[iLength++] = '\\';
                sEscaped[iLength++] = '"';
                break;
            case '\\':
                sEscaped[iLength++] = '\\';
                sEscaped[iLength++] = '\\';
                break;
            case '\b':
                sEscaped[iLength++] = '\\';
                sEscaped[iLength++] = 'b';
                break;
            case '\f':
                sEscaped[iLength++] = '\\';
                sEscaped[iLength++] = 'f';
                break;
            case '\n':
                sEscaped[iLength++] = '\\';
                sEscaped[iLength++] = 'n';
                break;
            case '\r':
                sEscaped[iLength++] = '\\';
                sEscaped[iLength++] = 'r';
                break;
            case '\t':
                sEscaped[iLength++] = '\\';
                sEscaped[iLength++] = 't';
                break;
            default:
                if ( c < 0x20u ) {
                    (void)snprintf(
                        &sEscaped[iLength],
                        7u,
                        "\\u%04x",
                        (unsigned)c
                    );
                    iLength += 6u;
                } else {
                    sEscaped[iLength++] = (char)c;
                }
                break;
        }
    }

    sEscaped[iLength] = '\0';
    *ppsEscaped = sEscaped;
    return XWORK_OK;
}

static xwork_status xwork__local_host_string_append(
    char **ppsText,
    size_t *piLength,
    const char *sChunk
)
{
    char *sNextText;
    size_t iChunkLength;

    if ( !ppsText || !piLength || !sChunk ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iChunkLength = strlen(sChunk);
    sNextText = (char *)realloc(*ppsText, *piLength + iChunkLength + 1u);
    if ( !sNextText ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    *ppsText = sNextText;
    memcpy(*ppsText + *piLength, sChunk, iChunkLength + 1u);
    *piLength += iChunkLength;
    return XWORK_OK;
}

static xwork_status xwork__local_host_parse_request_json(
    const char *sRequestJson,
    xvalue *ptRequest
)
{
    xvalue tRequest;

    if ( !ptRequest ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ptRequest = NULL;
    if ( !sRequestJson || !sRequestJson[0] ) {
        return XWORK_OK;
    }

    tRequest = xrtParseJSON((str)sRequestJson, strlen(sRequestJson));
    if ( !tRequest || xvoType(tRequest) != XVO_DT_TABLE ) {
        if ( tRequest ) {
            xvoUnref(tRequest);
        }
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ptRequest = tRequest;
    return XWORK_OK;
}

static const char *xwork__local_host_request_get_text(
    xvalue tRequest,
    const char *sKey
)
{
    xvalue tValue;

    if ( !tRequest || !sKey ) {
        return NULL;
    }

    tValue = xvoTableGetValue(tRequest, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) == XVO_DT_NULL ) {
        return NULL;
    }
    if ( xvoType(tValue) != XVO_DT_TEXT ) {
        return NULL;
    }
    return (const char *)xvoGetText(tValue);
}

static xwork_status xwork__local_host_request_get_list(
    xvalue tRequest,
    const char *sKey,
    xvalue *ptValue
)
{
    xvalue tValue;

    if ( !ptValue ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ptValue = NULL;
    if ( !tRequest || !sKey ) {
        return XWORK_OK;
    }

    tValue = xvoTableGetValue(tRequest, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) == XVO_DT_NULL ) {
        return XWORK_OK;
    }
    if ( xvoType(tValue) != XVO_DT_LIST && xvoType(tValue) != XVO_DT_ARRAY ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ptValue = tValue;
    return XWORK_OK;
}

static bool xwork__local_host_request_get_size(
    xvalue tRequest,
    const char *sKey,
    size_t *piValue
)
{
    xvalue tValue;
    int64 iValue;

    if ( !piValue ) {
        return false;
    }

    *piValue = 0u;
    if ( !tRequest || !sKey ) {
        return false;
    }

    tValue = xvoTableGetValue(tRequest, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) == XVO_DT_NULL ) {
        return false;
    }
    if ( xvoType(tValue) != XVO_DT_INT ) {
        return false;
    }

    iValue = xvoGetInt(tValue);
    if ( iValue <= 0 ) {
        return false;
    }

    *piValue = (size_t)iValue;
    return true;
}

static xwork_status xwork__local_host_request_get_positive_size_strict(
    xvalue tRequest,
    const char *sKey,
    bool *pbHasValue,
    size_t *piValue
)
{
    xvalue tValue;
    int64 iValue;

    if ( !pbHasValue || !piValue ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *pbHasValue = false;
    *piValue = 0u;
    if ( !tRequest || !sKey ) {
        return XWORK_OK;
    }

    tValue = xvoTableGetValue(tRequest, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) == XVO_DT_NULL ) {
        return XWORK_OK;
    }
    if ( xvoType(tValue) != XVO_DT_INT ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iValue = xvoGetInt(tValue);
    if ( iValue <= 0 ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *pbHasValue = true;
    *piValue = (size_t)iValue;
    return XWORK_OK;
}

static xwork_status xwork__local_host_request_get_bool(
    xvalue tRequest,
    const char *sKey,
    bool *pbHasValue,
    bool *pbValue
)
{
    xvalue tValue;

    if ( !pbHasValue || !pbValue ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *pbHasValue = false;
    if ( !tRequest || !sKey ) {
        return XWORK_OK;
    }

    tValue = xvoTableGetValue(tRequest, sKey, (uint32)strlen(sKey));
    if ( !tValue || xvoType(tValue) == XVO_DT_NULL ) {
        return XWORK_OK;
    }
    if ( xvoType(tValue) != XVO_DT_BOOL ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *pbHasValue = true;
    *pbValue = xvoGetBool(tValue);
    return XWORK_OK;
}

static size_t xwork__local_host_effective_limit(size_t iConfigured, size_t iRequested)
{
    size_t iLimit;

    iLimit = iConfigured;
    if ( iLimit == 0u ) {
        iLimit = iRequested;
    } else if ( iRequested > 0u && iRequested < iLimit ) {
        iLimit = iRequested;
    }
    if ( iLimit == 0u ) {
        iLimit = XWORK__LOCAL_HOST_DEFAULT_PROCESS_OUTPUT_BYTES;
    }
    return iLimit;
}

static xwork_status xwork__local_host_read_text_file(
    const char *sPath,
    size_t iOffsetBytes,
    size_t iMaxBytes,
    char **ppsText,
    size_t *piFileSizeBytes,
    size_t *piBytesRead,
    bool *pbTruncated,
    bool *pbEof
)
{
    FILE *pFile = NULL;
    char *sText = NULL;
    long long iFileSizeLong;
    size_t iFileSize;
    size_t iReadableBytes;
    size_t iReadBytes;

    if ( !sPath || !sPath[0] || !ppsText || !piFileSizeBytes || !piBytesRead ||
         !pbTruncated || !pbEof ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppsText = NULL;
    *piFileSizeBytes = 0u;
    *piBytesRead = 0u;
    *pbTruncated = false;
    *pbEof = false;

    pFile = fopen(sPath, "rb");
    if ( !pFile ) {
        return (errno == ENOENT) ? XWORK_ERROR_NOT_FOUND : XWORK_ERROR_EXTERNAL_FAILURE;
    }

    if ( fseek(pFile, 0, SEEK_END) != 0 ) {
        fclose(pFile);
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }
    iFileSizeLong = ftell(pFile);
    if ( iFileSizeLong < 0 ) {
        fclose(pFile);
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }
    iFileSize = (size_t)iFileSizeLong;
    if ( iOffsetBytes > iFileSize ) {
        iOffsetBytes = iFileSize;
    }
    if ( fseek(pFile, (long)iOffsetBytes, SEEK_SET) != 0 ) {
        fclose(pFile);
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }

    iReadableBytes = iFileSize - iOffsetBytes;
    iReadBytes = iReadableBytes;
    if ( iReadBytes > iMaxBytes ) {
        iReadBytes = iMaxBytes;
        *pbTruncated = true;
    }

    sText = (char *)malloc(iReadBytes + 1u);
    if ( !sText ) {
        fclose(pFile);
        return XWORK_ERROR_NO_MEMORY;
    }

    if ( iReadBytes > 0u && fread(sText, 1u, iReadBytes, pFile) != iReadBytes ) {
        fclose(pFile);
        free(sText);
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }

    sText[iReadBytes] = '\0';
    fclose(pFile);
    *ppsText = sText;
    *piFileSizeBytes = iFileSize;
    *piBytesRead = iReadBytes;
    *pbEof = ((iOffsetBytes + iReadBytes) >= iFileSize);
    return XWORK_OK;
}

static xwork_status xwork__local_host_write_text_file(
    const char *sPath,
    const char *sText,
    bool bAppend,
    size_t *piBytesWritten
);

static bool xwork__local_host_text_file_exists(const char *sPath)
{
    FILE *pFile;

    if ( !sPath || !sPath[0] ) {
        return false;
    }

    pFile = fopen(sPath, "rb");
    if ( !pFile ) {
        return false;
    }

    (void)fclose(pFile);
    return true;
}

static bool xwork__local_host_directory_exists(const char *sPath)
{
#ifdef _WIN32
    struct _stat tStat;

    if ( !sPath || !sPath[0] ) {
        return false;
    }
    if ( _stat(sPath, &tStat) != 0 ) {
        return false;
    }
    return (tStat.st_mode & _S_IFDIR) != 0;
#else
    struct stat tStat;

    if ( !sPath || !sPath[0] ) {
        return false;
    }
    if ( stat(sPath, &tStat) != 0 ) {
        return false;
    }
    return S_ISDIR(tStat.st_mode);
#endif
}

typedef struct {
    char *sText;
    size_t iLength;
    size_t iCapacity;
    size_t iLimit;
    bool bTruncated;
    xwork_status iStatus;
} xwork__local_host_process_capture;

typedef struct {
    xwork__local_host_process_capture *pStdoutCapture;
    xwork__local_host_process_capture *pStderrCapture;
} xwork__local_host_process_capture_set;

typedef struct xwork__local_host_terminal_session {
    char *sSessionId;
    char *sSessionName;
    char *sCommand;
    char *sResolvedCwd;
    xprocess *pProcess;
    size_t iSessionIndex;
    size_t iTerminalCols;
    size_t iTerminalRows;
    bool bStdinClosed;
    struct xwork__local_host_terminal_session *pNext;
} xwork__local_host_terminal_session;

static int xwork__local_host_process_stop_best_effort(
    xprocess *pProcess,
    int iInitialStopReason
);

static void xwork__local_host_process_capture_init(
    xwork__local_host_process_capture *pCapture,
    size_t iLimit
)
{
    if ( !pCapture ) {
        return;
    }

    memset(pCapture, 0, sizeof(*pCapture));
    pCapture->iLimit = iLimit;
    pCapture->iStatus = XWORK_OK;
}

static void xwork__local_host_process_capture_reset(
    xwork__local_host_process_capture *pCapture
)
{
    if ( !pCapture ) {
        return;
    }

    free(pCapture->sText);
    memset(pCapture, 0, sizeof(*pCapture));
}

static xwork_status xwork__local_host_process_capture_append(
    xwork__local_host_process_capture *pCapture,
    const void *pData,
    size_t iSize
)
{
    size_t iWritable;

    if ( !pCapture || (iSize > 0u && !pData) ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( pCapture->iStatus != XWORK_OK || iSize == 0u ) {
        return pCapture->iStatus;
    }

    if ( pCapture->iLength >= pCapture->iLimit ) {
        pCapture->bTruncated = true;
        return XWORK_OK;
    }

    iWritable = iSize;
    if ( pCapture->iLength + iWritable > pCapture->iLimit ) {
        iWritable = pCapture->iLimit - pCapture->iLength;
        pCapture->bTruncated = true;
    }

    if ( iWritable > 0u ) {
        if ( pCapture->iLength + iWritable + 1u > pCapture->iCapacity ) {
            char *sNextText;
            size_t iNextCapacity = pCapture->iLength + iWritable + 1u;

            sNextText = (char *)realloc(pCapture->sText, iNextCapacity);
            if ( !sNextText ) {
                pCapture->iStatus = XWORK_ERROR_NO_MEMORY;
                return pCapture->iStatus;
            }
            pCapture->sText = sNextText;
            pCapture->iCapacity = iNextCapacity;
        }

        memcpy(
            pCapture->sText + pCapture->iLength,
            pData,
            iWritable
        );
        pCapture->iLength += iWritable;
        pCapture->sText[pCapture->iLength] = '\0';
    }

    if ( iWritable < iSize ) {
        pCapture->bTruncated = true;
    }

    return XWORK_OK;
}

static char *xwork__local_host_process_capture_take_text(
    xwork__local_host_process_capture *pCapture
)
{
    char *sText;

    if ( !pCapture ) {
        return NULL;
    }
    if ( !pCapture->sText ) {
        return xwork__dup_cstr("");
    }

    sText = pCapture->sText;
    pCapture->sText = NULL;
    pCapture->iLength = 0u;
    pCapture->iCapacity = 0u;
    return sText;
}

static char *xwork__local_host_copy_xrt_text(const void *pData, size_t iSize)
{
    char *sText;

    sText = (char *)malloc(iSize + 1u);
    if ( !sText ) {
        return NULL;
    }
    if ( pData && iSize > 0u ) {
        memcpy(sText, pData, iSize);
    }
    sText[iSize] = '\0';
    return sText;
}

static xwork__local_host_terminal_session *xwork__local_host_find_terminal_session(
    xwork_local_host *pHost,
    const char *sSessionId
)
{
    xwork__local_host_terminal_session *pSession;

    if ( !pHost || !sSessionId || !sSessionId[0] ) {
        return NULL;
    }

    for ( pSession = (xwork__local_host_terminal_session *)pHost->pTerminalSessions;
          pSession;
          pSession = pSession->pNext ) {
        if ( pSession->sSessionId &&
             strcmp(pSession->sSessionId, sSessionId) == 0 ) {
            return pSession;
        }
    }
    return NULL;
}

static size_t xwork__local_host_terminal_session_count(const xwork_local_host *pHost)
{
    const xwork__local_host_terminal_session *pSession;
    size_t iCount = 0u;

    if ( !pHost ) {
        return 0u;
    }

    for ( pSession = (const xwork__local_host_terminal_session *)pHost->pTerminalSessions;
          pSession;
          pSession = pSession->pNext ) {
        ++iCount;
    }
    return iCount;
}

static void xwork__local_host_free_terminal_session(
    xwork__local_host_terminal_session *pSession
)
{
    if ( !pSession ) {
        return;
    }

    if ( pSession->pProcess ) {
        if ( xrtProcessIsRunning(pSession->pProcess) ) {
            (void)xwork__local_host_process_stop_best_effort(
                pSession->pProcess,
                XPROC_STOP_INTERRUPT
            );
        }
        xrtProcessDestroy(pSession->pProcess);
    }
    free(pSession->sSessionId);
    free(pSession->sSessionName);
    free(pSession->sCommand);
    free(pSession->sResolvedCwd);
    free(pSession);
}

static void xwork__local_host_reset_terminal_sessions(xwork_local_host *pHost)
{
    xwork__local_host_terminal_session *pSession;

    if ( !pHost ) {
        return;
    }

    pSession = (xwork__local_host_terminal_session *)pHost->pTerminalSessions;
    while ( pSession ) {
        xwork__local_host_terminal_session *pNext = pSession->pNext;
        xwork__local_host_free_terminal_session(pSession);
        pSession = pNext;
    }
    pHost->pTerminalSessions = NULL;
    pHost->iNextTerminalSessionId = 0u;
}

static void xwork__local_host_remove_terminal_session(
    xwork_local_host *pHost,
    xwork__local_host_terminal_session *pSession
)
{
    xwork__local_host_terminal_session **ppCursor;

    if ( !pHost || !pSession ) {
        return;
    }

    ppCursor = (xwork__local_host_terminal_session **)&pHost->pTerminalSessions;
    while ( *ppCursor ) {
        if ( *ppCursor == pSession ) {
            *ppCursor = pSession->pNext;
            xwork__local_host_free_terminal_session(pSession);
            return;
        }
        ppCursor = &(*ppCursor)->pNext;
    }
}

static void xwork__local_host_process_stdout_cb(
    xprocess *pProcess,
    const void *pData,
    size_t iSize,
    ptr pUserData
)
{
    xwork__local_host_process_capture_set *pCaptureSet =
        (xwork__local_host_process_capture_set *)pUserData;
    xwork__local_host_process_capture *pCapture;

    (void)pProcess;
    if ( !pCaptureSet || !pCaptureSet->pStdoutCapture ) {
        return;
    }

    pCapture = pCaptureSet->pStdoutCapture;
    (void)xwork__local_host_process_capture_append(pCapture, pData, iSize);
}

static void xwork__local_host_process_stderr_cb(
    xprocess *pProcess,
    const void *pData,
    size_t iSize,
    ptr pUserData
)
{
    xwork__local_host_process_capture_set *pCaptureSet =
        (xwork__local_host_process_capture_set *)pUserData;
    xwork__local_host_process_capture *pCapture;

    (void)pProcess;
    if ( !pCaptureSet || !pCaptureSet->pStderrCapture ) {
        return;
    }

    pCapture = pCaptureSet->pStderrCapture;
    (void)xwork__local_host_process_capture_append(pCapture, pData, iSize);
}

static const char *xwork__local_host_process_stop_reason_name(int iStopReason)
{
    switch ( iStopReason ) {
        case XPROC_STOP_INTERRUPT:
            return "interrupt";
        case XPROC_STOP_TERMINATE:
            return "terminate";
        case XPROC_STOP_KILL:
            return "kill";
        case XPROC_STOP_KILL_TREE:
            return "kill_tree";
        case XPROC_STOP_NONE:
        default:
            return "none";
    }
}

static const char *xwork__local_host_process_event_kind_name(int iKind)
{
    switch ( iKind ) {
        case XPROC_EVENT_START:
            return "start";
        case XPROC_EVENT_OUTPUT:
            return "output";
        case XPROC_EVENT_EXIT:
            return "exit";
        case XPROC_EVENT_NONE:
        default:
            return "none";
    }
}

static const char *xwork__local_host_process_stream_name(int iStream)
{
    switch ( iStream ) {
        case XPROC_STREAM_STDOUT:
            return "stdout";
        case XPROC_STREAM_STDERR:
            return "stderr";
        case XPROC_STREAM_TERMINAL:
            return "terminal";
        case XPROC_STREAM_NONE:
        default:
            return "none";
    }
}

static const char *xwork__local_host_process_exit_kind_name(int iKind)
{
    switch ( iKind ) {
        case XPROC_EXIT_NORMAL:
            return "normal";
        case XPROC_EXIT_SIGNAL:
            return "signal";
        case XPROC_EXIT_SPAWN_FAILED:
            return "spawn_failed";
        case XPROC_EXIT_WAIT_FAILED:
            return "wait_failed";
        case XPROC_EXIT_NONE:
        default:
            return "none";
    }
}

static const char *xwork__local_host_process_exit_stage_name(int iStage)
{
    switch ( iStage ) {
        case XPROC_STAGE_SPAWN:
            return "spawn";
        case XPROC_STAGE_WORKDIR:
            return "workdir";
        case XPROC_STAGE_ENV:
            return "env";
        case XPROC_STAGE_STDIN:
            return "stdin";
        case XPROC_STAGE_STDOUT:
            return "stdout";
        case XPROC_STAGE_STDERR:
            return "stderr";
        case XPROC_STAGE_EXEC:
            return "exec";
        case XPROC_STAGE_WAIT:
            return "wait";
        case XPROC_STAGE_NONE:
        default:
            return "none";
    }
}

static xwork_status xwork__local_host_build_process_events_json(
    const xprocessevent *pEvents,
    uint32 iEventCount,
    const char *sStdout,
    const char *sStderr,
    char **ppsEventsJson
)
{
    char *sEventsJson = NULL;
    size_t iEventsJsonLength = 0u;
    char *sItemJson = NULL;
    char *sEventText = NULL;
    char *sEscapedEventText = NULL;
    uint32 i;
    xwork_status iStatus;

    if ( !ppsEventsJson ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppsEventsJson = NULL;
    if ( iEventCount > 0u && !pEvents ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__local_host_string_append(
        &sEventsJson,
        &iEventsJsonLength,
        "["
    );
    if ( iStatus != XWORK_OK ) {
        goto cleanup;
    }

    for ( i = 0u; i < iEventCount; ++i ) {
        const xprocessevent *pEvent = &pEvents[i];
        const char *sKind = xwork__local_host_process_event_kind_name(pEvent->iKind);
        const char *sStream = xwork__local_host_process_stream_name(pEvent->iStream);

        if ( i > 0u ) {
            iStatus = xwork__local_host_string_append(
                &sEventsJson,
                &iEventsJsonLength,
                ","
            );
            if ( iStatus != XWORK_OK ) {
                goto cleanup;
            }
        }

        if ( pEvent->iKind == XPROC_EVENT_OUTPUT ) {
            const char *sSource =
                (pEvent->iStream == XPROC_STREAM_STDERR)
                    ? (sStderr ? sStderr : "")
                    : (sStdout ? sStdout : "");
            size_t iSourceLength = strlen(sSource);
            size_t iOffsetBytes =
                (pEvent->iOffset > (uint64)SIZE_MAX)
                    ? SIZE_MAX
                    : (size_t)pEvent->iOffset;
            size_t iRequestedSize =
                (pEvent->iSize > (uint64)SIZE_MAX)
                    ? SIZE_MAX
                    : (size_t)pEvent->iSize;
            size_t iAvailableBytes = 0u;
            bool bTextTruncated = false;

            if ( iOffsetBytes < iSourceLength ) {
                iAvailableBytes = iSourceLength - iOffsetBytes;
                if ( iAvailableBytes > iRequestedSize ) {
                    iAvailableBytes = iRequestedSize;
                }
            }
            if ( iAvailableBytes < iRequestedSize ) {
                bTextTruncated = true;
            }

            sEventText = (char *)malloc(iAvailableBytes + 1u);
            if ( !sEventText ) {
                iStatus = XWORK_ERROR_NO_MEMORY;
                goto cleanup;
            }
            if ( iAvailableBytes > 0u ) {
                memcpy(sEventText, sSource + iOffsetBytes, iAvailableBytes);
            }
            sEventText[iAvailableBytes] = '\0';

            iStatus = xwork__local_host_json_escape(
                sEventText,
                &sEscapedEventText
            );
            if ( iStatus != XWORK_OK ) {
                goto cleanup;
            }

            sItemJson = xwork__dup_printf(
                "{\"seq\":%llu,\"kind\":\"%s\",\"stream\":\"%s\","
                "\"offset_bytes\":%llu,\"size_bytes\":%llu,\"time_ms\":%llu,"
                "\"text\":\"%s\",\"text_truncated\":%s}",
                (unsigned long long)pEvent->iSeq,
                sKind,
                sStream,
                (unsigned long long)pEvent->iOffset,
                (unsigned long long)pEvent->iSize,
                (unsigned long long)pEvent->tTimeMs,
                sEscapedEventText ? sEscapedEventText : "",
                bTextTruncated ? "true" : "false"
            );
        } else if ( pEvent->iKind == XPROC_EVENT_EXIT ) {
            sItemJson = xwork__dup_printf(
                "{\"seq\":%llu,\"kind\":\"%s\",\"stream\":\"%s\","
                "\"offset_bytes\":%llu,\"size_bytes\":%llu,\"time_ms\":%llu,"
                "\"exit_kind\":\"%s\",\"exit_code\":%d,\"signal\":%d,"
                "\"stage\":\"%s\",\"os_error\":%d,"
                "\"stop_reason\":\"%s\",\"timed_out\":%s,\"cancelled\":%s}",
                (unsigned long long)pEvent->iSeq,
                sKind,
                sStream,
                (unsigned long long)pEvent->iOffset,
                (unsigned long long)pEvent->iSize,
                (unsigned long long)pEvent->tTimeMs,
                xwork__local_host_process_exit_kind_name(pEvent->ExitInfo.iKind),
                pEvent->ExitInfo.iExitCode,
                pEvent->ExitInfo.iSignal,
                xwork__local_host_process_exit_stage_name(pEvent->ExitInfo.iStage),
                pEvent->ExitInfo.iOsError,
                xwork__local_host_process_stop_reason_name(
                    pEvent->ExitInfo.iStopReason
                ),
                pEvent->ExitInfo.bTimedOut ? "true" : "false",
                pEvent->ExitInfo.bCancelled ? "true" : "false"
            );
        } else {
            sItemJson = xwork__dup_printf(
                "{\"seq\":%llu,\"kind\":\"%s\",\"stream\":\"%s\","
                "\"offset_bytes\":%llu,\"size_bytes\":%llu,\"time_ms\":%llu}",
                (unsigned long long)pEvent->iSeq,
                sKind,
                sStream,
                (unsigned long long)pEvent->iOffset,
                (unsigned long long)pEvent->iSize,
                (unsigned long long)pEvent->tTimeMs
            );
        }
        if ( !sItemJson ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto cleanup;
        }

        iStatus = xwork__local_host_string_append(
            &sEventsJson,
            &iEventsJsonLength,
            sItemJson
        );
        if ( iStatus != XWORK_OK ) {
            goto cleanup;
        }

        free(sItemJson);
        sItemJson = NULL;
        free(sEventText);
        sEventText = NULL;
        free(sEscapedEventText);
        sEscapedEventText = NULL;
    }

    iStatus = xwork__local_host_string_append(
        &sEventsJson,
        &iEventsJsonLength,
        "]"
    );
    if ( iStatus != XWORK_OK ) {
        goto cleanup;
    }

    *ppsEventsJson = sEventsJson;
    sEventsJson = NULL;
    iStatus = XWORK_OK;

cleanup:
    free(sItemJson);
    free(sEventText);
    free(sEscapedEventText);
    free(sEventsJson);
    return iStatus;
}

static xwork_status xwork__local_host_build_process_events_output_text(
    const xprocessevent *pEvents,
    uint32 iEventCount,
    const char *sStdout,
    const char *sStderr,
    char **ppsOutputText,
    size_t *piOutputBytes
)
{
    char *sOutputText = NULL;
    size_t iOutputBytes = 0u;
    uint32 i;

    if ( ppsOutputText ) {
        *ppsOutputText = NULL;
    }
    if ( piOutputBytes ) {
        *piOutputBytes = 0u;
    }
    if ( !ppsOutputText || !piOutputBytes ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    for ( i = 0u; i < iEventCount; ++i ) {
        const xprocessevent *pEvent = &pEvents[i];
        const char *sSource;
        size_t iSourceLength;
        size_t iOffsetBytes;
        size_t iRequestedSize;
        size_t iWritableBytes = 0u;
        char *sNext;

        if ( pEvent->iKind != XPROC_EVENT_OUTPUT ) {
            continue;
        }

        sSource =
            (pEvent->iStream == XPROC_STREAM_STDERR)
                ? (sStderr ? sStderr : "")
                : (sStdout ? sStdout : "");
        iSourceLength = strlen(sSource);
        iOffsetBytes =
            (pEvent->iOffset > (uint64)SIZE_MAX)
                ? SIZE_MAX
                : (size_t)pEvent->iOffset;
        iRequestedSize =
            (pEvent->iSize > (uint64)SIZE_MAX)
                ? SIZE_MAX
                : (size_t)pEvent->iSize;
        if ( iOffsetBytes < iSourceLength ) {
            iWritableBytes = iSourceLength - iOffsetBytes;
            if ( iWritableBytes > iRequestedSize ) {
                iWritableBytes = iRequestedSize;
            }
        }
        if ( iWritableBytes == 0u ) {
            continue;
        }

        sNext = (char *)realloc(sOutputText, iOutputBytes + iWritableBytes + 1u);
        if ( !sNext ) {
            free(sOutputText);
            return XWORK_ERROR_NO_MEMORY;
        }
        sOutputText = sNext;
        memcpy(sOutputText + iOutputBytes, sSource + iOffsetBytes, iWritableBytes);
        iOutputBytes += iWritableBytes;
        sOutputText[iOutputBytes] = '\0';
    }

    *ppsOutputText = sOutputText;
    *piOutputBytes = iOutputBytes;
    return XWORK_OK;
}

static xwork_status xwork__local_host_parse_process_stop_reason(
    const char *sText,
    int *piStopReason
)
{
    if ( !piStopReason ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( !sText || !sText[0] || strcmp(sText, "interrupt") == 0 ) {
        *piStopReason = XPROC_STOP_INTERRUPT;
        return XWORK_OK;
    }
    if ( strcmp(sText, "terminate") == 0 ) {
        *piStopReason = XPROC_STOP_TERMINATE;
        return XWORK_OK;
    }
    if ( strcmp(sText, "kill") == 0 ) {
        *piStopReason = XPROC_STOP_KILL;
        return XWORK_OK;
    }
    if ( strcmp(sText, "kill_tree") == 0 ) {
        *piStopReason = XPROC_STOP_KILL_TREE;
        return XWORK_OK;
    }
    return XWORK_ERROR_INVALID_ARGUMENT;
}

static bool xwork__local_host_process_request_stop(
    xprocess *pProcess,
    int iStopReason
)
{
    switch ( iStopReason ) {
        case XPROC_STOP_INTERRUPT:
            return xrtProcessInterrupt(pProcess);
        case XPROC_STOP_TERMINATE:
            return xrtProcessTerminate(pProcess);
        case XPROC_STOP_KILL:
            return xrtProcessKill(pProcess);
        case XPROC_STOP_KILL_TREE:
            return xrtProcessKillTree(pProcess);
        default:
            return false;
    }
}

static int xwork__local_host_process_stop_best_effort(
    xprocess *pProcess,
    int iInitialStopReason
)
{
    if ( !pProcess ) {
        return XPROC_STOP_NONE;
    }

    if ( iInitialStopReason == XPROC_STOP_NONE ) {
        iInitialStopReason = XPROC_STOP_INTERRUPT;
    }

    (void)xwork__local_host_process_request_stop(pProcess, iInitialStopReason);
    if ( xrtProcessWaitTimeout(pProcess, XWORK__LOCAL_HOST_PROCESS_TIMEOUT_GRACE_MS) ==
         XRT_WAIT_OK ) {
        return iInitialStopReason;
    }

    if ( iInitialStopReason != XPROC_STOP_TERMINATE &&
         iInitialStopReason != XPROC_STOP_KILL &&
         iInitialStopReason != XPROC_STOP_KILL_TREE ) {
        (void)xrtProcessTerminate(pProcess);
        if ( xrtProcessWaitTimeout(pProcess, XWORK__LOCAL_HOST_PROCESS_TIMEOUT_GRACE_MS) ==
             XRT_WAIT_OK ) {
            return XPROC_STOP_TERMINATE;
        }
    }

    if ( iInitialStopReason == XPROC_STOP_KILL ) {
        (void)xrtProcessWait(pProcess);
        return XPROC_STOP_KILL;
    }

    (void)xrtProcessKillTree(pProcess);
    if ( xrtProcessWaitTimeout(pProcess, XWORK__LOCAL_HOST_PROCESS_TIMEOUT_GRACE_MS) ==
         XRT_WAIT_OK ) {
        return XPROC_STOP_KILL_TREE;
    }

    (void)xrtProcessKill(pProcess);
    (void)xrtProcessWait(pProcess);
    return XPROC_STOP_KILL;
}

static xwork_status xwork__local_host_write_text_file(
    const char *sPath,
    const char *sText,
    bool bAppend,
    size_t *piBytesWritten
)
{
    FILE *pFile = NULL;
    size_t iLength;

    if ( !sPath || !sPath[0] || !sText || !piBytesWritten ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *piBytesWritten = 0u;
    iLength = strlen(sText);

    pFile = fopen(sPath, bAppend ? "ab" : "wb");
    if ( !pFile ) {
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }

    if ( iLength > 0u && fwrite(sText, 1u, iLength, pFile) != iLength ) {
        fclose(pFile);
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }
    if ( fclose(pFile) != 0 ) {
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }

    *piBytesWritten = iLength;
    return XWORK_OK;
}

static bool xwork__local_host_is_path_separator(char c)
{
    return c == '/' || c == '\\';
}

static char *xwork__local_host_dup_prefix(const char *sText, size_t iLength)
{
    char *sCopy;

    if ( !sText ) {
        return NULL;
    }

    sCopy = (char *)malloc(iLength + 1u);
    if ( !sCopy ) {
        return NULL;
    }

    if ( iLength > 0u ) {
        memcpy(sCopy, sText, iLength);
    }
    sCopy[iLength] = '\0';
    return sCopy;
}

static char *xwork__local_host_parent_directory(const char *sPath)
{
    size_t iLength;
    const char *sLastSeparator = NULL;
    size_t i;

    if ( !sPath || !sPath[0] ) {
        return NULL;
    }

    iLength = strlen(sPath);
    for ( i = 0u; i < iLength; ++i ) {
        if ( xwork__local_host_is_path_separator(sPath[i]) ) {
            sLastSeparator = &sPath[i];
        }
    }
    if ( !sLastSeparator ) {
        return NULL;
    }

#ifdef _WIN32
    if ( sLastSeparator == sPath + 2u &&
         isalpha((unsigned char)sPath[0]) &&
         sPath[1] == ':' &&
         xwork__local_host_is_path_separator(sPath[2]) ) {
        return xwork__local_host_dup_prefix(sPath, 3u);
    }
#endif

    if ( sLastSeparator == sPath ) {
        return xwork__local_host_dup_prefix(sPath, 1u);
    }

    return xwork__local_host_dup_prefix(
        sPath,
        (size_t)(sLastSeparator - sPath)
    );
}

static xwork_status xwork__local_host_create_directory(const char *sPath)
{
    if ( !sPath || !sPath[0] ) {
        return XWORK_OK;
    }

    errno = 0;
#ifdef _WIN32
    if ( _mkdir(sPath) == 0 ) {
        return XWORK_OK;
    }
#else
    if ( mkdir(sPath, 0777) == 0 ) {
        return XWORK_OK;
    }
#endif
    if ( errno == EEXIST ) {
        return XWORK_OK;
    }
    return XWORK_ERROR_EXTERNAL_FAILURE;
}

static xwork_status xwork__local_host_ensure_directory_tree(const char *sDirectoryPath)
{
    char *sMutablePath;
    size_t iStart = 0u;
    size_t i;
    xwork_status iStatus = XWORK_OK;

    if ( !sDirectoryPath || !sDirectoryPath[0] ) {
        return XWORK_OK;
    }

    sMutablePath = xwork__dup_cstr(sDirectoryPath);
    if ( !sMutablePath ) {
        return XWORK_ERROR_NO_MEMORY;
    }

#ifdef _WIN32
    if ( isalpha((unsigned char)sMutablePath[0]) &&
         sMutablePath[1] == ':' ) {
        iStart = 2u;
        if ( xwork__local_host_is_path_separator(sMutablePath[2]) ) {
            iStart = 3u;
        }
    } else if ( xwork__local_host_is_path_separator(sMutablePath[0]) &&
                xwork__local_host_is_path_separator(sMutablePath[1]) ) {
        iStart = 2u;
    }
#else
    if ( xwork__local_host_is_path_separator(sMutablePath[0]) ) {
        iStart = 1u;
    }
#endif

    for ( i = iStart; sMutablePath[i] != '\0'; ++i ) {
        if ( !xwork__local_host_is_path_separator(sMutablePath[i]) ) {
            continue;
        }

        sMutablePath[i] = '\0';
        if ( sMutablePath[0] != '\0' ) {
            iStatus = xwork__local_host_create_directory(sMutablePath);
            if ( iStatus != XWORK_OK ) {
                free(sMutablePath);
                return iStatus;
            }
        }
        sMutablePath[i] = '/';
    }

    iStatus = xwork__local_host_create_directory(sMutablePath);
    free(sMutablePath);
    return iStatus;
}

static xwork_status xwork__local_host_ensure_parent_directories(const char *sPath)
{
    char *sParentDirectory;
    xwork_status iStatus;

    sParentDirectory = xwork__local_host_parent_directory(sPath);
    if ( !sParentDirectory ) {
        return XWORK_OK;
    }

    iStatus = xwork__local_host_ensure_directory_tree(sParentDirectory);
    free(sParentDirectory);
    return iStatus;
}

static char *xwork__local_host_quote_shell_arg(const char *sText)
{
    if ( !sText ) {
        return NULL;
    }

#ifdef _WIN32
    if ( strchr(sText, '"') != NULL ) {
        return NULL;
    }
    return xwork__dup_printf("\"%s\"", sText);
#else
    char *sEscaped = NULL;
    char *sQuoted = NULL;
    size_t i;
    size_t iExtra = 0u;
    size_t iPos = 0u;

    for ( i = 0u; sText[i] != '\0'; ++i ) {
        if ( sText[i] == '\'' ) {
            iExtra += 3u;
        }
    }
    sEscaped = (char *)malloc(strlen(sText) + iExtra + 1u);
    if ( !sEscaped ) {
        return NULL;
    }
    for ( i = 0u; sText[i] != '\0'; ++i ) {
        if ( sText[i] == '\'' ) {
            sEscaped[iPos++] = '\'';
            sEscaped[iPos++] = '\\';
            sEscaped[iPos++] = '\'';
            sEscaped[iPos++] = '\'';
        } else {
            sEscaped[iPos++] = sText[i];
        }
    }
    sEscaped[iPos] = '\0';
    sQuoted = xwork__dup_printf("'%s'", sEscaped);
    free(sEscaped);
    return sQuoted;
#endif
}

static bool xwork__local_host_is_valid_env_name(
    const char *sName,
    size_t iLength
)
{
    size_t i;

    if ( !sName || iLength == 0u ) {
        return false;
    }
    if ( !(isalpha((unsigned char)sName[0]) || sName[0] == '_') ) {
        return false;
    }
    for ( i = 1u; i < iLength; ++i ) {
        if ( !(isalnum((unsigned char)sName[i]) || sName[i] == '_') ) {
            return false;
        }
    }

    return true;
}

static void xwork__local_host_free_process_env_entries(
    char ***ppsEntries,
    size_t iEntryCount
)
{
    size_t i;
    char **psEntries;

    if ( !ppsEntries || !*ppsEntries ) {
        return;
    }

    psEntries = *ppsEntries;
    for ( i = 0u; i < iEntryCount; ++i ) {
        free(psEntries[i]);
    }
    free(psEntries);
    *ppsEntries = NULL;
}

static xwork_status xwork__local_host_build_process_env_entries(
    xvalue tEnvList,
    size_t iMaxEnvEntries,
    char ***ppsEntries,
    size_t *piEnvCount
)
{
    char **psEntries = NULL;
    uint32 iCount = 0u;
    uint32 i;

    if ( !ppsEntries ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppsEntries = NULL;
    if ( piEnvCount ) {
        *piEnvCount = 0u;
    }
    if ( !tEnvList ) {
        return XWORK_OK;
    }

    if ( xvoType(tEnvList) == XVO_DT_ARRAY ) {
        iCount = xvoArrayItemCount(tEnvList);
    } else {
        iCount = xvoListItemCount(tEnvList);
    }
    if ( iMaxEnvEntries > 0u && (size_t)iCount > iMaxEnvEntries ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( iCount == 0u ) {
        return XWORK_OK;
    }

    psEntries = (char **)calloc((size_t)iCount, sizeof(*psEntries));
    if ( !psEntries ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    for ( i = 0u; i < iCount; ++i ) {
        xvalue tItem = (xvoType(tEnvList) == XVO_DT_ARRAY)
            ? xvoArrayGetValue(tEnvList, i)
            : xvoListGetValue(tEnvList, (int64)i);
        const char *sEntry;
        const char *sEquals;
        size_t iNameLength;

        if ( !tItem || xvoType(tItem) != XVO_DT_TEXT ) {
            xwork__local_host_free_process_env_entries(&psEntries, (size_t)iCount);
            return XWORK_ERROR_INVALID_ARGUMENT;
        }

        sEntry = (const char *)xvoGetText(tItem);
        if ( !sEntry ) {
            xwork__local_host_free_process_env_entries(&psEntries, (size_t)iCount);
            return XWORK_ERROR_INVALID_ARGUMENT;
        }

        sEquals = strchr(sEntry, '=');
        if ( !sEquals || sEquals == sEntry ) {
            xwork__local_host_free_process_env_entries(&psEntries, (size_t)iCount);
            return XWORK_ERROR_INVALID_ARGUMENT;
        }

        iNameLength = (size_t)(sEquals - sEntry);
        if ( !xwork__local_host_is_valid_env_name(sEntry, iNameLength) ) {
            xwork__local_host_free_process_env_entries(&psEntries, (size_t)iCount);
            return XWORK_ERROR_INVALID_ARGUMENT;
        }

        psEntries[i] = xwork__dup_cstr(sEntry);
        if ( !psEntries[i] ) {
            xwork__local_host_free_process_env_entries(&psEntries, (size_t)iCount);
            return XWORK_ERROR_NO_MEMORY;
        }
    }

    *ppsEntries = psEntries;
    if ( piEnvCount ) {
        *piEnvCount = (size_t)iCount;
    }
    return XWORK_OK;
}

static xwork_status xwork__local_host_capture_command_output(
    const char *sShellCommand,
    size_t iMaxBytes,
    char **ppsOutput,
    int *piExitCode,
    bool *pbTruncated
)
{
    FILE *pPipe = NULL;
    char *sOutput = NULL;
    size_t iCapacity = 0u;
    size_t iLength = 0u;
    int iCloseStatus;

    if ( !sShellCommand || !sShellCommand[0] || !ppsOutput || !piExitCode || !pbTruncated ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppsOutput = NULL;
    *piExitCode = -1;
    *pbTruncated = false;

    pPipe = XWORK__HOST_POPEN(sShellCommand, "r");
    if ( !pPipe ) {
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }

    iCapacity = (iMaxBytes > 0u && iMaxBytes < 4096u) ? iMaxBytes + 1u : 4097u;
    sOutput = (char *)malloc(iCapacity);
    if ( !sOutput ) {
        (void)XWORK__HOST_PCLOSE(pPipe);
        return XWORK_ERROR_NO_MEMORY;
    }

    for (;;) {
        char sChunk[1024];
        size_t iChunkBytes;

        iChunkBytes = fread(sChunk, 1u, sizeof(sChunk), pPipe);
        if ( iChunkBytes == 0u ) {
            break;
        }

        if ( iLength < iMaxBytes ) {
            size_t iWritable = iChunkBytes;

            if ( iLength + iWritable > iMaxBytes ) {
                iWritable = iMaxBytes - iLength;
                *pbTruncated = true;
            }
            if ( iLength + iWritable + 1u > iCapacity ) {
                char *sNextOutput;
                size_t iNextCapacity = iLength + iWritable + 1u;

                sNextOutput = (char *)realloc(sOutput, iNextCapacity);
                if ( !sNextOutput ) {
                    free(sOutput);
                    (void)XWORK__HOST_PCLOSE(pPipe);
                    return XWORK_ERROR_NO_MEMORY;
                }
                sOutput = sNextOutput;
                iCapacity = iNextCapacity;
            }
            if ( iWritable > 0u ) {
                memcpy(sOutput + iLength, sChunk, iWritable);
                iLength += iWritable;
            }
        } else {
            *pbTruncated = true;
        }
    }

    sOutput[iLength] = '\0';
    iCloseStatus = XWORK__HOST_PCLOSE(pPipe);

#ifdef _WIN32
    *piExitCode = iCloseStatus;
#else
    if ( WIFEXITED(iCloseStatus) ) {
        *piExitCode = WEXITSTATUS(iCloseStatus);
    } else {
        *piExitCode = iCloseStatus;
    }
#endif

    *ppsOutput = sOutput;
    return XWORK_OK;
}

static xwork_status xwork__local_host_set_result(
    xwork_local_host *pHost,
    const char *sOutputText,
    const char *sVisibleSummary,
    xwork_tool_result *pResult
)
{
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__replace_cstr(&pHost->sLastOutputText, sOutputText);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    iStatus = xwork__replace_cstr(&pHost->sLastVisibleSummary, sVisibleSummary);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    pResult->sOutputText = pHost->sLastOutputText;
    pResult->sVisibleSummary = pHost->sLastVisibleSummary;
    pResult->bRetryable = false;
    return XWORK_OK;
}

static size_t xwork__local_host_list_count(xvalue tList)
{
    if ( !tList ) {
        return 0u;
    }
    if ( xvoType(tList) == XVO_DT_ARRAY ) {
        return (size_t)xvoArrayItemCount(tList);
    }
    return (size_t)xvoListItemCount(tList);
}

static xwork_status xwork__local_host_set_process_result(
    xwork_local_host *pHost,
    const char *sCommand,
    const char *sResolvedCwd,
    const char *sStdout,
    const char *sStderr,
    int iExitCode,
    bool bTruncated,
    bool bStdoutTruncated,
    bool bStderrTruncated,
    bool bMergeStderr,
    bool bUseTerminal,
    size_t iTerminalCols,
    size_t iTerminalRows,
    size_t iEnvCount,
    size_t iStdinBytes,
    size_t iTimeoutMs,
    bool bTimedOut,
    int iTimeoutStopReason,
    int iStopReason,
    bool bAllowNonZeroExit,
    bool bIncludeEvents,
    uint32 iEventCount,
    const char *sEventsJson,
    bool bOk,
    const char *sVisibleSummary,
    const char *sErrorKind,
    const char *sErrorMessage,
    xwork_tool_result *pResult
)
{
    char *sEscapedCommand = NULL;
    char *sEscapedStdout = NULL;
    char *sEscapedStderr = NULL;
    char *sEscapedCwd = NULL;
    char *sEscapedErrorKind = NULL;
    char *sEscapedErrorMessage = NULL;
    char *sEventsFragment = NULL;
    char *sOutputText = NULL;
    const char *sTimeoutStopName;
    const char *sStopReasonName;
    bool bTerminalOutputCaptured = false;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    sTimeoutStopName = xwork__local_host_process_stop_reason_name(iTimeoutStopReason);
    sStopReasonName = xwork__local_host_process_stop_reason_name(iStopReason);
    bTerminalOutputCaptured = bUseTerminal &&
        ((sStdout && sStdout[0]) ||
         (sStderr && sStderr[0]) ||
         (sEventsJson && strstr(sEventsJson, "\"stream\":\"terminal\"") != NULL));

    iStatus = xwork__local_host_json_escape(sCommand ? sCommand : "", &sEscapedCommand);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(sStdout ? sStdout : "", &sEscapedStdout);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(sStderr ? sStderr : "", &sEscapedStderr);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sResolvedCwd ? sResolvedCwd : "",
        &sEscapedCwd
    );
    if ( iStatus != XWORK_OK ) goto cleanup;
    if ( bIncludeEvents ) {
        sEventsFragment = xwork__dup_printf(
            ",\"include_events\":true,\"event_count\":%u,\"events\":%s",
            (unsigned)iEventCount,
            sEventsJson ? sEventsJson : "[]"
        );
        if ( !sEventsFragment ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto cleanup;
        }
    }

    if ( bOk ) {
        sOutputText = xwork__dup_printf(
            "{\"ok\":true,\"command\":\"%s\",\"cwd\":\"%s\",\"merge_stderr\":%s,"
            "\"use_terminal\":%s,\"terminal_cols\":%zu,\"terminal_rows\":%zu,"
            "\"terminal_output_captured\":%s,"
            "\"stdout\":\"%s\",\"stderr\":\"%s\"%s,\"exit_code\":%d,"
            "\"truncated\":%s,\"stdout_truncated\":%s,\"stderr_truncated\":%s,"
            "\"env_count\":%zu,\"stdin_bytes\":%zu,"
            "\"timeout_ms\":%zu,\"timed_out\":%s,\"timeout_stop\":\"%s\","
            "\"stop_reason\":\"%s\",\"allow_nonzero_exit\":%s}",
            sEscapedCommand,
            sEscapedCwd,
            bMergeStderr ? "true" : "false",
            bUseTerminal ? "true" : "false",
            iTerminalCols,
            iTerminalRows,
            bTerminalOutputCaptured ? "true" : "false",
            sEscapedStdout,
            sEscapedStderr,
            sEventsFragment ? sEventsFragment : "",
            iExitCode,
            bTruncated ? "true" : "false",
            bStdoutTruncated ? "true" : "false",
            bStderrTruncated ? "true" : "false",
            iEnvCount,
            iStdinBytes,
            iTimeoutMs,
            bTimedOut ? "true" : "false",
            sTimeoutStopName,
            sStopReasonName,
            bAllowNonZeroExit ? "true" : "false"
        );
    } else {
        iStatus = xwork__local_host_json_escape(
            sErrorKind ? sErrorKind : "external_failure",
            &sEscapedErrorKind
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        iStatus = xwork__local_host_json_escape(
            sErrorMessage ? sErrorMessage : "process.exec failed",
            &sEscapedErrorMessage
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        sOutputText = xwork__dup_printf(
            "{\"ok\":false,\"command\":\"%s\",\"cwd\":\"%s\",\"merge_stderr\":%s,"
            "\"use_terminal\":%s,\"terminal_cols\":%zu,\"terminal_rows\":%zu,"
            "\"terminal_output_captured\":%s,"
            "\"stdout\":\"%s\",\"stderr\":\"%s\"%s,\"exit_code\":%d,"
            "\"truncated\":%s,\"stdout_truncated\":%s,\"stderr_truncated\":%s,"
            "\"env_count\":%zu,\"stdin_bytes\":%zu,"
            "\"timeout_ms\":%zu,\"timed_out\":%s,\"timeout_stop\":\"%s\","
            "\"stop_reason\":\"%s\",\"allow_nonzero_exit\":%s,"
            "\"error_kind\":\"%s\",\"error\":\"%s\"}",
            sEscapedCommand,
            sEscapedCwd,
            bMergeStderr ? "true" : "false",
            bUseTerminal ? "true" : "false",
            iTerminalCols,
            iTerminalRows,
            bTerminalOutputCaptured ? "true" : "false",
            sEscapedStdout,
            sEscapedStderr,
            sEventsFragment ? sEventsFragment : "",
            iExitCode,
            bTruncated ? "true" : "false",
            bStdoutTruncated ? "true" : "false",
            bStderrTruncated ? "true" : "false",
            iEnvCount,
            iStdinBytes,
            iTimeoutMs,
            bTimedOut ? "true" : "false",
            sTimeoutStopName,
            sStopReasonName,
            bAllowNonZeroExit ? "true" : "false",
            sEscapedErrorKind,
            sEscapedErrorMessage
        );
    }
    if ( !sOutputText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sOutputText,
        sVisibleSummary ? sVisibleSummary : (bOk ? "process.exec ok" : "process.exec failed"),
        pResult
    );

cleanup:
    free(sEscapedCommand);
    free(sEscapedStdout);
    free(sEscapedStderr);
    free(sEscapedCwd);
    free(sEscapedErrorKind);
    free(sEscapedErrorMessage);
    free(sEventsFragment);
    free(sOutputText);
    return iStatus;
}

static xwork_status xwork__local_host_set_terminal_state_result(
    xwork_local_host *pHost,
    const char *sSessionId,
    const char *sSessionName,
    const char *sCommand,
    const char *sResolvedCwd,
    size_t iSessionIndex,
    const char *sOutputText,
    size_t iOutputBytes,
    size_t iTerminalCols,
    size_t iTerminalRows,
    bool bStdinClosed,
    bool bTerminalOutputCaptured,
    bool bRunning,
    bool bEventStreamDone,
    bool bRemoved,
    uint64 iNextAfterSeq,
    uint64 iEventEndSeq,
    uint32 iEventCount,
    const char *sEventsJson,
    const char *sExtraJsonFields,
    const xprocessexitinfo *pExitInfo,
    bool bOk,
    const char *sVisibleSummary,
    const char *sErrorKind,
    const char *sErrorMessage,
    xwork_tool_result *pResult
)
{
    xprocessexitinfo tExitInfo;
    char *sEscapedSessionId = NULL;
    char *sEscapedSessionName = NULL;
    char *sEscapedCommand = NULL;
    char *sEscapedCwd = NULL;
    char *sEscapedOutputText = NULL;
    char *sEscapedErrorKind = NULL;
    char *sEscapedErrorMessage = NULL;
    char *sResultText = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    memset(&tExitInfo, 0, sizeof(tExitInfo));
    if ( pExitInfo ) {
        tExitInfo = *pExitInfo;
    }

    iStatus = xwork__local_host_json_escape(
        sSessionId ? sSessionId : "",
        &sEscapedSessionId
    );
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sSessionName ? sSessionName : "",
        &sEscapedSessionName
    );
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sCommand ? sCommand : "",
        &sEscapedCommand
    );
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sResolvedCwd ? sResolvedCwd : "",
        &sEscapedCwd
    );
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sOutputText ? sOutputText : "",
        &sEscapedOutputText
    );
    if ( iStatus != XWORK_OK ) goto cleanup;

    if ( bOk ) {
        sResultText = xwork__dup_printf(
            "{\"ok\":true,\"session_id\":\"%s\",\"session_name\":\"%s\",\"command\":\"%s\",\"cwd\":\"%s\","
            "\"session_index\":%zu,\"stdin_closed\":%s,"
            "\"running\":%s,\"done\":%s,\"removed\":%s,"
            "\"terminal_cols\":%zu,\"terminal_rows\":%zu,"
            "\"terminal_output_captured\":%s,"
            "\"output_text\":\"%s\",\"output_bytes\":%zu,"
            "\"event_count\":%u,\"events\":%s,\"next_after_seq\":%llu,"
            "\"event_end_seq\":%llu,\"has_more_events\":%s,\"event_stream_done\":%s,%s"
            "\"exit_kind\":\"%s\",\"exit_code\":%d,\"signal\":%d,"
            "\"stage\":\"%s\",\"os_error\":%d,"
            "\"stop_reason\":\"%s\",\"timed_out\":%s,\"cancelled\":%s}",
            sEscapedSessionId,
            sEscapedSessionName,
            sEscapedCommand,
            sEscapedCwd,
            iSessionIndex,
            bStdinClosed ? "true" : "false",
            bRunning ? "true" : "false",
            bRunning ? "false" : "true",
            bRemoved ? "true" : "false",
            iTerminalCols,
            iTerminalRows,
            bTerminalOutputCaptured ? "true" : "false",
            sEscapedOutputText ? sEscapedOutputText : "",
            iOutputBytes,
            (unsigned)iEventCount,
            sEventsJson ? sEventsJson : "[]",
            (unsigned long long)iNextAfterSeq,
            (unsigned long long)iEventEndSeq,
            (iNextAfterSeq < iEventEndSeq) ? "true" : "false",
            bEventStreamDone ? "true" : "false",
            sExtraJsonFields ? sExtraJsonFields : "",
            xwork__local_host_process_exit_kind_name(tExitInfo.iKind),
            tExitInfo.iExitCode,
            tExitInfo.iSignal,
            xwork__local_host_process_exit_stage_name(tExitInfo.iStage),
            tExitInfo.iOsError,
            xwork__local_host_process_stop_reason_name(tExitInfo.iStopReason),
            tExitInfo.bTimedOut ? "true" : "false",
            tExitInfo.bCancelled ? "true" : "false"
        );
    } else {
        iStatus = xwork__local_host_json_escape(
            sErrorKind ? sErrorKind : "external_failure",
            &sEscapedErrorKind
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        iStatus = xwork__local_host_json_escape(
            sErrorMessage ? sErrorMessage : "terminal session operation failed",
            &sEscapedErrorMessage
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        sResultText = xwork__dup_printf(
            "{\"ok\":false,\"session_id\":\"%s\",\"session_name\":\"%s\",\"command\":\"%s\",\"cwd\":\"%s\","
            "\"session_index\":%zu,\"stdin_closed\":%s,"
            "\"running\":%s,\"done\":%s,\"removed\":%s,"
            "\"terminal_cols\":%zu,\"terminal_rows\":%zu,"
            "\"terminal_output_captured\":%s,"
            "\"output_text\":\"%s\",\"output_bytes\":%zu,"
            "\"event_count\":%u,\"events\":%s,\"next_after_seq\":%llu,"
            "\"event_end_seq\":%llu,\"has_more_events\":%s,\"event_stream_done\":%s,%s"
            "\"exit_kind\":\"%s\",\"exit_code\":%d,\"signal\":%d,"
            "\"stage\":\"%s\",\"os_error\":%d,"
            "\"stop_reason\":\"%s\",\"timed_out\":%s,\"cancelled\":%s,"
            "\"error_kind\":\"%s\",\"error\":\"%s\"}",
            sEscapedSessionId,
            sEscapedSessionName,
            sEscapedCommand,
            sEscapedCwd,
            iSessionIndex,
            bStdinClosed ? "true" : "false",
            bRunning ? "true" : "false",
            bRunning ? "false" : "true",
            bRemoved ? "true" : "false",
            iTerminalCols,
            iTerminalRows,
            bTerminalOutputCaptured ? "true" : "false",
            sEscapedOutputText ? sEscapedOutputText : "",
            iOutputBytes,
            (unsigned)iEventCount,
            sEventsJson ? sEventsJson : "[]",
            (unsigned long long)iNextAfterSeq,
            (unsigned long long)iEventEndSeq,
            (iNextAfterSeq < iEventEndSeq) ? "true" : "false",
            bEventStreamDone ? "true" : "false",
            sExtraJsonFields ? sExtraJsonFields : "",
            xwork__local_host_process_exit_kind_name(tExitInfo.iKind),
            tExitInfo.iExitCode,
            tExitInfo.iSignal,
            xwork__local_host_process_exit_stage_name(tExitInfo.iStage),
            tExitInfo.iOsError,
            xwork__local_host_process_stop_reason_name(tExitInfo.iStopReason),
            tExitInfo.bTimedOut ? "true" : "false",
            tExitInfo.bCancelled ? "true" : "false",
            sEscapedErrorKind,
            sEscapedErrorMessage
        );
    }
    if ( !sResultText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sResultText,
        sVisibleSummary
            ? sVisibleSummary
            : (bOk ? "process.terminal session ok" : "process.terminal session failed"),
        pResult
    );

cleanup:
    free(sEscapedSessionId);
    free(sEscapedSessionName);
    free(sEscapedCommand);
    free(sEscapedCwd);
    free(sEscapedOutputText);
    free(sEscapedErrorKind);
    free(sEscapedErrorMessage);
    free(sResultText);
    return iStatus;
}

static xwork_status xwork__local_host_set_terminal_write_result(
    xwork_local_host *pHost,
    const char *sSessionId,
    const char *sSessionName,
    size_t iSessionIndex,
    size_t iBytesWritten,
    bool bWriteEof,
    bool bStdinClosed,
    bool bHasStateSnapshot,
    const char *sCommand,
    const char *sResolvedCwd,
    const char *sOutputText,
    size_t iOutputBytes,
    size_t iTerminalCols,
    size_t iTerminalRows,
    bool bTerminalOutputCaptured,
    bool bRunning,
    bool bEventStreamDone,
    uint64 iNextAfterSeq,
    uint64 iEventEndSeq,
    uint32 iEventCount,
    const char *sEventsJson,
    bool bOk,
    const char *sVisibleSummary,
    const char *sErrorKind,
    const char *sErrorMessage,
    xwork_tool_result *pResult
)
{
    char sExtraJson[128];
    char *sEscapedSessionId = NULL;
    char *sEscapedSessionName = NULL;
    char *sEscapedErrorKind = NULL;
    char *sEscapedErrorMessage = NULL;
    char *sResultText = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__local_host_json_escape(
        sSessionId ? sSessionId : "",
        &sEscapedSessionId
    );
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sSessionName ? sSessionName : "",
        &sEscapedSessionName
    );
    if ( iStatus != XWORK_OK ) goto cleanup;

    if ( bHasStateSnapshot ) {
        snprintf(
            sExtraJson,
            sizeof(sExtraJson),
            "\"bytes_written\":%zu,\"write_eof\":%s,\"stdin_closed\":%s,",
            iBytesWritten,
            bWriteEof ? "true" : "false",
            bStdinClosed ? "true" : "false"
        );
        iStatus = xwork__local_host_set_terminal_state_result(
            pHost,
            sSessionId,
            sSessionName,
            sCommand,
            sResolvedCwd,
            iSessionIndex,
            sOutputText,
            iOutputBytes,
            iTerminalCols,
            iTerminalRows,
            bStdinClosed,
            bTerminalOutputCaptured,
            bRunning,
            bEventStreamDone,
            false,
            iNextAfterSeq,
            iEventEndSeq,
            iEventCount,
            sEventsJson,
            sExtraJson,
            NULL,
            bOk,
            sVisibleSummary,
            sErrorKind,
            sErrorMessage,
            pResult
        );
        goto cleanup;
    }

    if ( bOk ) {
        sResultText = xwork__dup_printf(
            "{\"ok\":true,\"session_id\":\"%s\",\"session_name\":\"%s\",\"session_index\":%zu,\"bytes_written\":%zu,"
            "\"write_eof\":%s,\"stdin_closed\":%s}",
            sEscapedSessionId,
            sEscapedSessionName,
            iSessionIndex,
            iBytesWritten,
            bWriteEof ? "true" : "false",
            bStdinClosed ? "true" : "false"
        );
    } else {
        iStatus = xwork__local_host_json_escape(
            sErrorKind ? sErrorKind : "external_failure",
            &sEscapedErrorKind
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        iStatus = xwork__local_host_json_escape(
            sErrorMessage ? sErrorMessage : "terminal write failed",
            &sEscapedErrorMessage
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        sResultText = xwork__dup_printf(
            "{\"ok\":false,\"session_id\":\"%s\",\"session_name\":\"%s\",\"session_index\":%zu,\"bytes_written\":%zu,"
            "\"write_eof\":%s,\"stdin_closed\":%s,"
            "\"error_kind\":\"%s\",\"error\":\"%s\"}",
            sEscapedSessionId,
            sEscapedSessionName,
            iSessionIndex,
            iBytesWritten,
            bWriteEof ? "true" : "false",
            bStdinClosed ? "true" : "false",
            sEscapedErrorKind,
            sEscapedErrorMessage
        );
    }
    if ( !sResultText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sResultText,
        sVisibleSummary
            ? sVisibleSummary
            : (bOk ? "process.terminal_write ok" : "process.terminal_write failed"),
        pResult
    );

cleanup:
    free(sEscapedSessionId);
    free(sEscapedSessionName);
    free(sEscapedErrorKind);
    free(sEscapedErrorMessage);
    free(sResultText);
    return iStatus;
}

static xwork_status xwork__local_host_set_terminal_list_result(
    xwork_local_host *pHost,
    size_t iSessionCount,
    const char *sSessionsJson,
    bool bOk,
    const char *sVisibleSummary,
    const char *sErrorKind,
    const char *sErrorMessage,
    xwork_tool_result *pResult
)
{
    char *sEscapedErrorKind = NULL;
    char *sEscapedErrorMessage = NULL;
    char *sResultText = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( bOk ) {
        sResultText = xwork__dup_printf(
            "{\"ok\":true,\"session_count\":%zu,\"sessions\":%s}",
            iSessionCount,
            sSessionsJson ? sSessionsJson : "[]"
        );
    } else {
        iStatus = xwork__local_host_json_escape(
            sErrorKind ? sErrorKind : "external_failure",
            &sEscapedErrorKind
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        iStatus = xwork__local_host_json_escape(
            sErrorMessage ? sErrorMessage : "failed to list terminal sessions",
            &sEscapedErrorMessage
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        sResultText = xwork__dup_printf(
            "{\"ok\":false,\"session_count\":%zu,\"sessions\":%s,"
            "\"error_kind\":\"%s\",\"error\":\"%s\"}",
            iSessionCount,
            sSessionsJson ? sSessionsJson : "[]",
            sEscapedErrorKind,
            sEscapedErrorMessage
        );
    }
    if ( !sResultText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sResultText,
        sVisibleSummary
            ? sVisibleSummary
            : (bOk ? "process.list_terminals ok" : "process.list_terminals failed"),
        pResult
    );

cleanup:
    free(sEscapedErrorKind);
    free(sEscapedErrorMessage);
    free(sResultText);
    return iStatus;
}

static xwork_status xwork__local_host_set_filesystem_read_result(
    xwork_local_host *pHost,
    const char *sPath,
    const char *sResolvedPath,
    const char *sText,
    size_t iOffsetBytes,
    size_t iFileSizeBytes,
    size_t iBytesRead,
    bool bTruncated,
    bool bEof,
    bool bOk,
    const char *sVisibleSummary,
    const char *sErrorKind,
    const char *sErrorMessage,
    xwork_tool_result *pResult
)
{
    char *sEscapedPath = NULL;
    char *sEscapedResolvedPath = NULL;
    char *sEscapedText = NULL;
    char *sEscapedErrorKind = NULL;
    char *sEscapedErrorMessage = NULL;
    char *sOutputText = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__local_host_json_escape(sPath ? sPath : "", &sEscapedPath);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sResolvedPath ? sResolvedPath : "",
        &sEscapedResolvedPath
    );
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(sText ? sText : "", &sEscapedText);
    if ( iStatus != XWORK_OK ) goto cleanup;

    if ( bOk ) {
        sOutputText = xwork__dup_printf(
            "{\"ok\":true,\"path\":\"%s\",\"resolved_path\":\"%s\",\"text\":\"%s\","
            "\"offset_bytes\":%zu,\"file_size_bytes\":%zu,\"bytes_read\":%zu,"
            "\"next_offset_bytes\":%zu,\"remaining_bytes\":%zu,"
            "\"truncated\":%s,\"eof\":%s}",
            sEscapedPath,
            sEscapedResolvedPath,
            sEscapedText,
            iOffsetBytes,
            iFileSizeBytes,
            iBytesRead,
            iOffsetBytes + iBytesRead,
            (iOffsetBytes + iBytesRead <= iFileSizeBytes)
                ? (iFileSizeBytes - (iOffsetBytes + iBytesRead))
                : 0u,
            bTruncated ? "true" : "false",
            bEof ? "true" : "false"
        );
    } else {
        iStatus = xwork__local_host_json_escape(
            sErrorKind ? sErrorKind : "external_failure",
            &sEscapedErrorKind
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        iStatus = xwork__local_host_json_escape(
            sErrorMessage ? sErrorMessage : "filesystem.read_text failed",
            &sEscapedErrorMessage
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        sOutputText = xwork__dup_printf(
            "{\"ok\":false,\"path\":\"%s\",\"resolved_path\":\"%s\",\"text\":\"%s\","
            "\"offset_bytes\":%zu,\"file_size_bytes\":%zu,\"bytes_read\":%zu,"
            "\"next_offset_bytes\":%zu,\"remaining_bytes\":%zu,"
            "\"truncated\":%s,\"eof\":%s,"
            "\"error_kind\":\"%s\",\"error\":\"%s\"}",
            sEscapedPath,
            sEscapedResolvedPath,
            sEscapedText,
            iOffsetBytes,
            iFileSizeBytes,
            iBytesRead,
            iOffsetBytes + iBytesRead,
            (iOffsetBytes + iBytesRead <= iFileSizeBytes)
                ? (iFileSizeBytes - (iOffsetBytes + iBytesRead))
                : 0u,
            bTruncated ? "true" : "false",
            bEof ? "true" : "false",
            sEscapedErrorKind,
            sEscapedErrorMessage
        );
    }
    if ( !sOutputText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sOutputText,
        sVisibleSummary ? sVisibleSummary : (bOk ? "filesystem.read_text ok" : "filesystem.read_text failed"),
        pResult
    );

cleanup:
    free(sEscapedPath);
    free(sEscapedResolvedPath);
    free(sEscapedText);
    free(sEscapedErrorKind);
    free(sEscapedErrorMessage);
    free(sOutputText);
    return iStatus;
}

static xwork_status xwork__local_host_set_filesystem_write_result(
    xwork_local_host *pHost,
    const char *sPath,
    const char *sResolvedPath,
    const char *sMode,
    bool bCreateDirs,
    size_t iBytesWritten,
    bool bOk,
    const char *sVisibleSummary,
    const char *sErrorKind,
    const char *sErrorMessage,
    xwork_tool_result *pResult
)
{
    char *sEscapedPath = NULL;
    char *sEscapedResolvedPath = NULL;
    char *sEscapedMode = NULL;
    char *sEscapedErrorKind = NULL;
    char *sEscapedErrorMessage = NULL;
    char *sOutputText = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__local_host_json_escape(sPath ? sPath : "", &sEscapedPath);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sResolvedPath ? sResolvedPath : "",
        &sEscapedResolvedPath
    );
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sMode ? sMode : "overwrite",
        &sEscapedMode
    );
    if ( iStatus != XWORK_OK ) goto cleanup;

    if ( bOk ) {
        sOutputText = xwork__dup_printf(
            "{\"ok\":true,\"path\":\"%s\",\"resolved_path\":\"%s\",\"mode\":\"%s\","
            "\"create_dirs\":%s,\"bytes_written\":%llu}",
            sEscapedPath,
            sEscapedResolvedPath,
            sEscapedMode,
            bCreateDirs ? "true" : "false",
            (unsigned long long)iBytesWritten
        );
    } else {
        iStatus = xwork__local_host_json_escape(
            sErrorKind ? sErrorKind : "external_failure",
            &sEscapedErrorKind
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        iStatus = xwork__local_host_json_escape(
            sErrorMessage ? sErrorMessage : "filesystem.write_text failed",
            &sEscapedErrorMessage
        );
        if ( iStatus != XWORK_OK ) goto cleanup;
        sOutputText = xwork__dup_printf(
            "{\"ok\":false,\"path\":\"%s\",\"resolved_path\":\"%s\",\"mode\":\"%s\","
            "\"create_dirs\":%s,\"bytes_written\":%llu,"
            "\"error_kind\":\"%s\",\"error\":\"%s\"}",
            sEscapedPath,
            sEscapedResolvedPath,
            sEscapedMode,
            bCreateDirs ? "true" : "false",
            (unsigned long long)iBytesWritten,
            sEscapedErrorKind,
            sEscapedErrorMessage
        );
    }
    if ( !sOutputText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sOutputText,
        sVisibleSummary ? sVisibleSummary : (bOk ? "filesystem.write_text ok" : "filesystem.write_text failed"),
        pResult
    );

cleanup:
    free(sEscapedPath);
    free(sEscapedResolvedPath);
    free(sEscapedMode);
    free(sEscapedErrorKind);
    free(sEscapedErrorMessage);
    free(sOutputText);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_filesystem(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    const char *sPath;
    char *sResolvedPath = NULL;
    char *sText = NULL;
    bool bTruncated = false;
    bool bEof = false;
    size_t iFileSizeBytes = 0u;
    size_t iRequestOffsetBytes = 0u;
    size_t iRequestMaxBytes = 0u;
    size_t iBytesRead = 0u;
    size_t iReadLimit;
    const char *sFailureKind = NULL;
    const char *sFailureSummary = NULL;
    const char *sFailureMessage = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !pHost->bEnableFilesystemReadText ) {
        return XWORK_ERROR_UNSUPPORTED;
    }

    iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    sPath = xwork__local_host_request_get_text(tRequest, "path");
    if ( !sPath || !sPath[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "filesystem.read_text invalid request";
        sFailureMessage = "path is required";
        goto cleanup;
    }

    (void)xwork__local_host_request_get_size(tRequest, "offset_bytes", &iRequestOffsetBytes);
    (void)xwork__local_host_request_get_size(tRequest, "max_bytes", &iRequestMaxBytes);
    iReadLimit = xwork__local_host_effective_limit(
        pHost->iMaxReadBytes ? pHost->iMaxReadBytes : XWORK__LOCAL_HOST_DEFAULT_READ_BYTES,
        iRequestMaxBytes
    );

    sResolvedPath = xwork__local_host_resolve_path(pHost, sPath);
    if ( !sResolvedPath ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_read_text_file(
        sResolvedPath,
        iRequestOffsetBytes,
        iReadLimit,
        &sText,
        &iFileSizeBytes,
        &iBytesRead,
        &bTruncated,
        &bEof
    );
    if ( iStatus != XWORK_OK ) {
        if ( iStatus == XWORK_ERROR_NOT_FOUND ) {
            sFailureKind = "not_found";
            sFailureSummary = "filesystem.read_text not found";
            sFailureMessage = "path does not exist";
        } else if ( iStatus == XWORK_ERROR_EXTERNAL_FAILURE ) {
            sFailureKind = "read_failed";
            sFailureSummary = "filesystem.read_text failed";
            sFailureMessage = "failed to read file";
        }
        goto cleanup;
    }

    iStatus = xwork__local_host_set_filesystem_read_result(
        pHost,
        sPath,
        sResolvedPath,
        sText,
        iRequestOffsetBytes,
        iFileSizeBytes,
        iBytesRead,
        bTruncated,
        bEof,
        true,
        "filesystem.read_text ok",
        NULL,
        NULL,
        pResult
    );

cleanup:
    if ( iStatus != XWORK_OK && sFailureKind ) {
        (void)xwork__local_host_set_filesystem_read_result(
            pHost,
            sPath ? sPath : "",
            sResolvedPath ? sResolvedPath : sPath,
            "",
            iRequestOffsetBytes,
            iFileSizeBytes,
            iBytesRead,
            bTruncated,
            bEof,
            false,
            sFailureSummary,
            sFailureKind,
            sFailureMessage,
            pResult
        );
    }
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sResolvedPath);
    free(sText);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_filesystem_write_text(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    const char *sPath;
    const char *sText;
    const char *sMode;
    char *sResolvedPath = NULL;
    size_t iBytesWritten = 0u;
    bool bAppend = false;
    bool bCreate = false;
    bool bHasCreateDirs = false;
    bool bCreateDirs = false;
    const char *sModeText = "overwrite";
    const char *sFailureKind = NULL;
    const char *sFailureSummary = NULL;
    const char *sFailureMessage = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !pHost->bEnableFilesystemWriteText ) {
        return XWORK_ERROR_UNSUPPORTED;
    }

    iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    sPath = xwork__local_host_request_get_text(tRequest, "path");
    sText = xwork__local_host_request_get_text(tRequest, "text");
    sMode = xwork__local_host_request_get_text(tRequest, "mode");
    if ( !sPath || !sPath[0] || !sText ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "filesystem.write_text invalid request";
        sFailureMessage = "path and text are required";
        goto cleanup;
    }
    if ( sMode && sMode[0] ) {
        if ( strcmp(sMode, "append") == 0 ) {
            bAppend = true;
            sModeText = "append";
        } else if ( strcmp(sMode, "create") == 0 ) {
            bCreate = true;
            sModeText = "create";
        } else if ( strcmp(sMode, "overwrite") != 0 ) {
            iStatus = XWORK_ERROR_INVALID_ARGUMENT;
            sFailureKind = "invalid_request";
            sFailureSummary = "filesystem.write_text invalid request";
            sFailureMessage = "mode must be overwrite, append, or create";
            goto cleanup;
        }
    }
    iStatus = xwork__local_host_request_get_bool(
        tRequest,
        "create_dirs",
        &bHasCreateDirs,
        &bCreateDirs
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "filesystem.write_text invalid request";
        sFailureMessage = "create_dirs must be boolean";
        goto cleanup;
    }

    sResolvedPath = xwork__local_host_resolve_path(pHost, sPath);
    if ( !sResolvedPath ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }
    if ( bHasCreateDirs && bCreateDirs ) {
        iStatus = xwork__local_host_ensure_parent_directories(sResolvedPath);
        if ( iStatus != XWORK_OK ) {
            sFailureKind = "create_dirs_failed";
            sFailureSummary = "filesystem.write_text failed (create_dirs)";
            sFailureMessage = "failed to create parent directories";
            goto cleanup;
        }
    } else {
        char *sParentDirectory = xwork__local_host_parent_directory(sResolvedPath);

        if ( sParentDirectory ) {
            if ( !xwork__local_host_directory_exists(sParentDirectory) ) {
                free(sParentDirectory);
                iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
                sFailureKind = "parent_not_found";
                sFailureSummary = "filesystem.write_text failed (parent directory not found)";
                sFailureMessage = "parent directory does not exist";
                goto cleanup;
            }
            free(sParentDirectory);
        }
    }
    if ( bCreate && xwork__local_host_text_file_exists(sResolvedPath) ) {
        iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
        sFailureKind = "already_exists";
        sFailureSummary = "filesystem.write_text failed (target exists)";
        sFailureMessage = "path already exists";
        goto cleanup;
    }

    iStatus = xwork__local_host_write_text_file(
        sResolvedPath,
        sText,
        bAppend,
        &iBytesWritten
    );
    if ( iStatus != XWORK_OK ) {
        if ( iStatus == XWORK_ERROR_EXTERNAL_FAILURE ) {
            sFailureKind = "write_failed";
            sFailureSummary = "filesystem.write_text failed";
            sFailureMessage = "failed to write file";
        }
        goto cleanup;
    }

    iStatus = xwork__local_host_set_filesystem_write_result(
        pHost,
        sPath,
        sResolvedPath,
        sModeText,
        bHasCreateDirs && bCreateDirs,
        iBytesWritten,
        true,
        "filesystem.write_text ok",
        NULL,
        NULL,
        pResult
    );

cleanup:
    if ( iStatus != XWORK_OK && sFailureKind ) {
        (void)xwork__local_host_set_filesystem_write_result(
            pHost,
            sPath ? sPath : "",
            sResolvedPath ? sResolvedPath : sPath,
            sModeText,
            bHasCreateDirs && bCreateDirs,
            iBytesWritten,
            false,
            sFailureSummary,
            sFailureKind,
            sFailureMessage,
            pResult
        );
    }
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sResolvedPath);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_process(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    xvalue tEnvList = NULL;
    const char *sCommand;
    const char *sRequestedCwd = NULL;
    const char *sInputText = NULL;
    char *sResolvedCwd = NULL;
    char **psEnvEntries = NULL;
    char *sStdout = NULL;
    char *sStderr = NULL;
    char *sDynamicFailureSummary = NULL;
    char *sDynamicFailureMessage = NULL;
    xwork__local_host_process_capture tStdoutCapture = {0};
    xwork__local_host_process_capture tStderrCapture = {0};
    xwork__local_host_process_capture_set tCaptureSet = {0};
    xprocessconfig tProcessConfig = {0};
    xprocessevents tProcessEvents = {0};
    xprocessexitinfo tExitInfo = {0};
    xprocessevent *pProcessEventsSnapshot = NULL;
    xprocess *pProcess = NULL;
    bool bHasAllowNonZeroExit = false;
    bool bAllowNonZeroExit = false;
    bool bHasIncludeEvents = false;
    bool bIncludeEvents = false;
    bool bHasUseTerminal = false;
    bool bUseTerminal = false;
    bool bHasTimeoutMs = false;
    bool bTimedOut = false;
    bool bTruncated = false;
    bool bHasMergeStderr = false;
    bool bMergeStderr = true;
    bool bStdoutTruncated = false;
    bool bStderrTruncated = false;
    int iTimeoutStopReason = XPROC_STOP_INTERRUPT;
    int iObservedStopReason = XPROC_STOP_NONE;
    size_t iRequestMaxBytes = 0u;
    size_t iTimeoutMs = 0u;
    size_t iCaptureLimit;
    size_t iEnvCount = 0u;
    size_t iRequestedEnvCount = 0u;
    size_t iStdinBytes = 0u;
    size_t iTerminalCols = 0u;
    size_t iTerminalRows = 0u;
    bool bHasTerminalCols = false;
    bool bHasTerminalRows = false;
    uint32 iProcessEventCount = 0u;
    int64 iBytesWritten = 0;
    int iWaitResult = XRT_WAIT_OK;
    int iExitCode = -1;
    const char *sFailureKind = NULL;
    const char *sFailureSummary = NULL;
    const char *sFailureMessage = NULL;
    const char *sTimeoutStopText = NULL;
    char *sEventsJson = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !pHost->bEnableProcessExec ) {
        return XWORK_ERROR_UNSUPPORTED;
    }

    iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    sCommand = xwork__local_host_request_get_text(tRequest, "command");
    if ( !sCommand || !sCommand[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "command is required";
        goto cleanup;
    }
    sRequestedCwd = xwork__local_host_request_get_text(tRequest, "cwd");
    if ( sRequestedCwd && !sRequestedCwd[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "cwd must not be empty";
        goto cleanup;
    }
    sInputText = xwork__local_host_request_get_text(tRequest, "stdin_text");
    if ( sInputText &&
         pHost->iMaxProcessInputBytes > 0u &&
         strlen(sInputText) > pHost->iMaxProcessInputBytes ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request (stdin_text exceeds limit)";
        sFailureMessage = "stdin_text exceeds max_process_input_bytes";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_bool(
        tRequest,
        "allow_nonzero_exit",
        &bHasAllowNonZeroExit,
        &bAllowNonZeroExit
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "allow_nonzero_exit must be boolean";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_bool(
        tRequest,
        "include_events",
        &bHasIncludeEvents,
        &bIncludeEvents
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "include_events must be boolean";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_bool(
        tRequest,
        "use_terminal",
        &bHasUseTerminal,
        &bUseTerminal
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "use_terminal must be boolean";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_bool(
        tRequest,
        "merge_stderr",
        &bHasMergeStderr,
        &bMergeStderr
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "merge_stderr must be boolean";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "terminal_cols",
        &bHasTerminalCols,
        &iTerminalCols
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "terminal_cols must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "terminal_rows",
        &bHasTerminalRows,
        &iTerminalRows
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "terminal_rows must be a positive integer";
        goto cleanup;
    }
    if ( !bUseTerminal && (bHasTerminalCols || bHasTerminalRows) ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "terminal_cols/terminal_rows require use_terminal:true";
        goto cleanup;
    }
    if ( bUseTerminal && bHasMergeStderr && !bMergeStderr ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "merge_stderr=false is unsupported with use_terminal:true";
        goto cleanup;
    }
    if ( bUseTerminal && !xrtProcessTerminalSupported() ) {
        iStatus = XWORK_ERROR_UNSUPPORTED;
        sFailureKind = "unsupported";
        sFailureSummary = "process.exec terminal mode unsupported";
        sFailureMessage = "terminal mode is not supported on this platform";
        goto cleanup;
    }
    if ( bUseTerminal ) {
        if ( !bHasTerminalCols ) {
            iTerminalCols = XWORK__LOCAL_HOST_DEFAULT_TERMINAL_COLS;
        }
        if ( !bHasTerminalRows ) {
            iTerminalRows = XWORK__LOCAL_HOST_DEFAULT_TERMINAL_ROWS;
        }
    } else {
        iTerminalCols = 0u;
        iTerminalRows = 0u;
    }
    if ( iTerminalCols > 0xffffffffu || iTerminalRows > 0xffffffffu ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "terminal_cols/terminal_rows exceed max supported value";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "timeout_ms",
        &bHasTimeoutMs,
        &iTimeoutMs
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "timeout_ms must be a positive integer";
        goto cleanup;
    }
    if ( bHasTimeoutMs && iTimeoutMs > 0xffffffffu ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "timeout_ms exceeds max supported value";
        goto cleanup;
    }
    sTimeoutStopText = xwork__local_host_request_get_text(tRequest, "timeout_stop");
    iStatus = xwork__local_host_parse_process_stop_reason(
        sTimeoutStopText,
        &iTimeoutStopReason
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage =
            "timeout_stop must be one of interrupt, terminate, kill, kill_tree";
        goto cleanup;
    }

    (void)xwork__local_host_request_get_size(tRequest, "max_output_bytes", &iRequestMaxBytes);
    iCaptureLimit = xwork__local_host_effective_limit(
        pHost->iMaxProcessOutputBytes,
        iRequestMaxBytes
    );
    iStatus = xwork__local_host_request_get_list(tRequest, "env", &tEnvList);
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.exec invalid request";
        sFailureMessage = "env must be a list of KEY=VALUE strings";
        goto cleanup;
    }
    iRequestedEnvCount = xwork__local_host_list_count(tEnvList);
    iStatus = xwork__local_host_build_process_env_entries(
        tEnvList,
        pHost->iMaxProcessEnvEntries,
        &psEnvEntries,
        &iEnvCount
    );
    if ( iStatus != XWORK_OK ) {
        if ( iStatus == XWORK_ERROR_INVALID_ARGUMENT ) {
            sFailureKind = "invalid_request";
            if ( pHost->iMaxProcessEnvEntries > 0u &&
                 iRequestedEnvCount > pHost->iMaxProcessEnvEntries ) {
                sFailureSummary = "process.exec invalid request (env limit exceeded)";
                sFailureMessage = "env exceeds max_process_env_entries";
            } else {
                sFailureSummary = "process.exec invalid request";
                sFailureMessage = "env entries are invalid";
            }
        }
        goto cleanup;
    }

    if ( sRequestedCwd ) {
        sResolvedCwd = xwork__local_host_resolve_path(pHost, sRequestedCwd);
        if ( !sResolvedCwd ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto cleanup;
        }
    } else if ( pHost->sDefaultWorkingDirectory && pHost->sDefaultWorkingDirectory[0] ) {
        sResolvedCwd = xwork__dup_cstr(pHost->sDefaultWorkingDirectory);
        if ( !sResolvedCwd ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto cleanup;
        }
    }

    xwork__local_host_process_capture_init(&tStdoutCapture, iCaptureLimit);
    xwork__local_host_process_capture_init(&tStderrCapture, iCaptureLimit);
    tCaptureSet.pStdoutCapture = &tStdoutCapture;
    tCaptureSet.pStderrCapture = &tStderrCapture;
    memset(&tProcessConfig, 0, sizeof(tProcessConfig));
    memset(&tProcessEvents, 0, sizeof(tProcessEvents));
    memset(&tExitInfo, 0, sizeof(tExitInfo));

    xrtProcessConfigInit(&tProcessConfig);
    tProcessEvents.OnStdout = xwork__local_host_process_stdout_cb;
    tProcessEvents.OnStderr = bMergeStderr ? NULL : xwork__local_host_process_stderr_cb;
    tProcessConfig.iTargetKind = XPROC_TARGET_SHELL;
    tProcessConfig.sCommand = (str)sCommand;
    tProcessConfig.sWorkDir =
        (str)((sResolvedCwd && sResolvedCwd[0]) ? sResolvedCwd : NULL);
    tProcessConfig.arrEnv = (str *)psEnvEntries;
    tProcessConfig.iEnvCount = (uint32)iEnvCount;
    tProcessConfig.bMergeStderr = bMergeStderr;
    tProcessConfig.bUseTerminal = bUseTerminal;
    tProcessConfig.bHideWindow = true;
    tProcessConfig.iTerminalCols = (uint32)iTerminalCols;
    tProcessConfig.iTerminalRows = (uint32)iTerminalRows;
    tProcessConfig.iMaxCaptureBytes = iCaptureLimit;
    tProcessConfig.Stdin.iMode = sInputText ? XPROC_STDIO_PIPE : XPROC_STDIO_NULL;
    tProcessConfig.Stdout.iMode = XPROC_STDIO_PIPE;
    tProcessConfig.Stderr.iMode = bMergeStderr ? XPROC_STDIO_NULL : XPROC_STDIO_PIPE;
    tProcessConfig.pEvents = &tProcessEvents;
    tProcessConfig.pUserData = &tCaptureSet;

    pProcess = xrtProcessSpawn(&tProcessConfig);
    if ( !pProcess ) {
        iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
        sFailureKind = "spawn_failed";
        sFailureSummary = "process.exec failed to start";
        sFailureMessage = "failed to start process";
        goto cleanup;
    }

    if ( sInputText ) {
        iStdinBytes = strlen(sInputText);
        iBytesWritten = xrtProcessWriteText(pProcess, (str)sInputText, iStdinBytes);
        if ( iBytesWritten < 0 || (size_t)iBytesWritten != iStdinBytes ) {
            iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
            sFailureKind = "stdin_failed";
            sFailureSummary = "process.exec failed to write stdin";
            sFailureMessage = "failed to write stdin_text";
            iObservedStopReason = xwork__local_host_process_stop_best_effort(
                pProcess,
                XPROC_STOP_INTERRUPT
            );
            goto cleanup;
        }
        if ( !xrtProcessCloseStdin(pProcess) ) {
            iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
            sFailureKind = "stdin_failed";
            sFailureSummary = "process.exec failed to write stdin";
            sFailureMessage = "failed to close stdin";
            iObservedStopReason = xwork__local_host_process_stop_best_effort(
                pProcess,
                XPROC_STOP_INTERRUPT
            );
            goto cleanup;
        }
    }

    if ( bHasTimeoutMs ) {
        iWaitResult = xrtProcessWaitTimeout(pProcess, (uint32)iTimeoutMs);
    } else {
        iWaitResult = xrtProcessWait(pProcess) ? XRT_WAIT_OK : XRT_WAIT_ERROR;
    }
    if ( iWaitResult == XRT_WAIT_TIMEOUT ) {
        iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
        bTimedOut = true;
        sFailureKind = "timeout";
        sFailureSummary = "process.exec timed out";
        sFailureMessage = "process exceeded timeout_ms";
        iObservedStopReason = xwork__local_host_process_stop_best_effort(
            pProcess,
            iTimeoutStopReason
        );
    } else if ( iWaitResult == XRT_WAIT_ERROR ) {
        iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
        sFailureKind = "wait_failed";
        sFailureSummary = "process.exec failed while waiting";
        sFailureMessage = "failed while waiting for process";
        iObservedStopReason = xwork__local_host_process_stop_best_effort(
            pProcess,
            XPROC_STOP_INTERRUPT
        );
    }

    if ( pProcess ) {
        (void)xrtProcessGetExitInfo(pProcess, &tExitInfo);
        iExitCode = tExitInfo.iExitCode;
        if ( tExitInfo.iStopReason != XPROC_STOP_NONE ) {
            iObservedStopReason = tExitInfo.iStopReason;
        }
        if ( bHasIncludeEvents && bIncludeEvents ) {
            pProcessEventsSnapshot = xrtProcessReadEventsSince(
                pProcess,
                0u,
                0u,
                &iProcessEventCount,
                NULL
            );
        }
        xrtProcessDestroy(pProcess);
        pProcess = NULL;
    }
    if ( tStdoutCapture.iStatus != XWORK_OK ) {
        iStatus = tStdoutCapture.iStatus;
        goto cleanup;
    }
    if ( tStderrCapture.iStatus != XWORK_OK ) {
        iStatus = tStderrCapture.iStatus;
        goto cleanup;
    }
    sStdout = xwork__local_host_process_capture_take_text(&tStdoutCapture);
    if ( !sStdout ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }
    sStderr = xwork__local_host_process_capture_take_text(&tStderrCapture);
    if ( !sStderr ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }
    bStdoutTruncated = tStdoutCapture.bTruncated;
    bStderrTruncated = tStderrCapture.bTruncated;
    bTruncated = bStdoutTruncated || bStderrTruncated;
    if ( bHasIncludeEvents && bIncludeEvents ) {
        iStatus = xwork__local_host_build_process_events_json(
            pProcessEventsSnapshot,
            iProcessEventCount,
            sStdout,
            sStderr,
            &sEventsJson
        );
        if ( iStatus != XWORK_OK ) {
            goto cleanup;
        }
    }

    if ( iStatus != XWORK_OK && sFailureKind ) {
        goto cleanup;
    }
    if ( iExitCode != 0 && !(bHasAllowNonZeroExit && bAllowNonZeroExit) ) {
        iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
        sFailureKind = "nonzero_exit";
        sDynamicFailureSummary = xwork__dup_printf(
            "process.exec failed (exit %d)",
            iExitCode
        );
        sDynamicFailureMessage = xwork__dup_printf(
            "process exited with code %d",
            iExitCode
        );
        if ( !sDynamicFailureSummary || !sDynamicFailureMessage ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto cleanup;
        }
        sFailureSummary = sDynamicFailureSummary;
        sFailureMessage = sDynamicFailureMessage;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_process_result(
        pHost,
        sCommand,
        sResolvedCwd,
        sStdout,
        sStderr,
        iExitCode,
        bTruncated,
        bStdoutTruncated,
        bStderrTruncated,
        bMergeStderr,
        bUseTerminal,
        iTerminalCols,
        iTerminalRows,
        iEnvCount,
        iStdinBytes,
        bHasTimeoutMs ? iTimeoutMs : 0u,
        bTimedOut,
        iTimeoutStopReason,
        iObservedStopReason,
        bHasAllowNonZeroExit && bAllowNonZeroExit,
        bHasIncludeEvents && bIncludeEvents,
        iProcessEventCount,
        sEventsJson,
        true,
        "process.exec ok",
        NULL,
        NULL,
        pResult
    );

cleanup:
    if ( pProcess ) {
        if ( xrtProcessIsRunning(pProcess) ) {
            iObservedStopReason = xwork__local_host_process_stop_best_effort(
                pProcess,
                iObservedStopReason != XPROC_STOP_NONE
                    ? iObservedStopReason
                    : XPROC_STOP_INTERRUPT
            );
        }
        (void)xrtProcessGetExitInfo(pProcess, &tExitInfo);
        if ( iExitCode < 0 ) {
            iExitCode = tExitInfo.iExitCode;
        }
        if ( tExitInfo.iStopReason != XPROC_STOP_NONE ) {
            iObservedStopReason = tExitInfo.iStopReason;
        }
        if ( bHasIncludeEvents && bIncludeEvents && !pProcessEventsSnapshot ) {
            pProcessEventsSnapshot = xrtProcessReadEventsSince(
                pProcess,
                0u,
                0u,
                &iProcessEventCount,
                NULL
            );
        }
        xrtProcessDestroy(pProcess);
        pProcess = NULL;
    }
    if ( !sStdout && tStdoutCapture.iStatus == XWORK_OK ) {
        sStdout = xwork__local_host_process_capture_take_text(&tStdoutCapture);
        if ( !sStdout && iStatus == XWORK_OK ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
        }
    }
    if ( !sStderr && tStderrCapture.iStatus == XWORK_OK ) {
        sStderr = xwork__local_host_process_capture_take_text(&tStderrCapture);
        if ( !sStderr && iStatus == XWORK_OK ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
        }
    }
    if ( (bHasIncludeEvents && bIncludeEvents) &&
         !sEventsJson &&
         sStdout &&
         sStderr ) {
        xwork_status iEventsStatus = xwork__local_host_build_process_events_json(
            pProcessEventsSnapshot,
            iProcessEventCount,
            sStdout,
            sStderr,
            &sEventsJson
        );
        if ( iEventsStatus != XWORK_OK && iStatus == XWORK_OK ) {
            iStatus = iEventsStatus;
        }
    }
    if ( iStatus != XWORK_OK && sFailureKind ) {
        (void)xwork__local_host_set_process_result(
            pHost,
            sCommand ? sCommand : "",
            sResolvedCwd ? sResolvedCwd : sRequestedCwd,
            sStdout ? sStdout : "",
            sStderr ? sStderr : "",
            iExitCode,
            bTruncated,
            bStdoutTruncated,
            bStderrTruncated,
            bMergeStderr,
            bUseTerminal,
            iTerminalCols,
            iTerminalRows,
            iEnvCount ? iEnvCount : iRequestedEnvCount,
            iStdinBytes,
            bHasTimeoutMs ? iTimeoutMs : 0u,
            bTimedOut,
            iTimeoutStopReason,
            iObservedStopReason,
            bHasAllowNonZeroExit && bAllowNonZeroExit,
            bHasIncludeEvents && bIncludeEvents,
            iProcessEventCount,
            sEventsJson,
            false,
            sFailureSummary,
            sFailureKind,
            sFailureMessage,
            pResult
        );
    }
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    xwork__local_host_process_capture_reset(&tStdoutCapture);
    xwork__local_host_process_capture_reset(&tStderrCapture);
    free(sResolvedCwd);
    xwork__local_host_free_process_env_entries(&psEnvEntries, iEnvCount);
    free(sStdout);
    free(sStderr);
    free(sEventsJson);
    free(sDynamicFailureSummary);
    free(sDynamicFailureMessage);
    xrtFree(pProcessEventsSnapshot);
    return iStatus;
}

static xwork_status xwork__local_host_terminal_session_snapshot(
    xwork__local_host_terminal_session *pSession,
    size_t iAfterSeq,
    size_t iMaxEvents,
    bool *pbTerminalOutputCaptured,
    bool *pbRunning,
    bool *pbEventStreamDone,
    xprocessexitinfo *pExitInfo,
    uint32 *piEventCount,
    uint64 *piNextAfterSeq,
    uint64 *piEventEndSeq,
    char **ppsEventsJson,
    char **ppsOutputText,
    size_t *piOutputBytes
)
{
    xprocesseventreadinfo tReadInfo;
    xprocessevent *pEvents = NULL;
    void *pStdoutData = NULL;
    void *pStderrData = NULL;
    size_t iStdoutSize = 0u;
    size_t iStderrSize = 0u;
    char *sStdout = NULL;
    char *sStderr = NULL;
    char *sEventsJson = NULL;
    char *sOutputText = NULL;
    xwork_status iStatus = XWORK_OK;

    if ( !pSession || !pSession->pProcess ||
         !pbTerminalOutputCaptured || !pbRunning || !pbEventStreamDone ||
         !pExitInfo || !piEventCount || !piNextAfterSeq || !piEventEndSeq ||
         !ppsEventsJson || !ppsOutputText || !piOutputBytes ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *pbTerminalOutputCaptured = false;
    *pbRunning = xrtProcessIsRunning(pSession->pProcess);
    *pbEventStreamDone = false;
    memset(pExitInfo, 0, sizeof(*pExitInfo));
    *piEventCount = 0u;
    *piNextAfterSeq = (uint64)iAfterSeq;
    *piEventEndSeq = (uint64)iAfterSeq;
    *ppsEventsJson = NULL;
    *ppsOutputText = NULL;
    *piOutputBytes = 0u;
    memset(&tReadInfo, 0, sizeof(tReadInfo));

    (void)xrtProcessGetExitInfo(pSession->pProcess, pExitInfo);
    pEvents = xrtProcessReadEventsSince(
        pSession->pProcess,
        (uint64)iAfterSeq,
        (iMaxEvents > 0xffffffffu) ? 0xffffffffu : (uint32)iMaxEvents,
        piEventCount,
        &tReadInfo
    );
    *piNextAfterSeq = tReadInfo.iNextSeq;
    *piEventEndSeq = tReadInfo.iEventEndSeq;
    *pbEventStreamDone = tReadInfo.bDone;

    pStdoutData = xrtProcessGetStdout(pSession->pProcess, &iStdoutSize);
    pStderrData = xrtProcessGetStderr(pSession->pProcess, &iStderrSize);
    sStdout = xwork__local_host_copy_xrt_text(pStdoutData, iStdoutSize);
    sStderr = xwork__local_host_copy_xrt_text(pStderrData, iStderrSize);
    if ( !sStdout || !sStderr ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_build_process_events_json(
        pEvents,
        *piEventCount,
        sStdout,
        sStderr,
        &sEventsJson
    );
    if ( iStatus != XWORK_OK ) {
        goto cleanup;
    }
    iStatus = xwork__local_host_build_process_events_output_text(
        pEvents,
        *piEventCount,
        sStdout,
        sStderr,
        &sOutputText,
        piOutputBytes
    );
    if ( iStatus != XWORK_OK ) {
        goto cleanup;
    }
    *pbTerminalOutputCaptured = (*piOutputBytes > 0u);

    *ppsEventsJson = sEventsJson;
    sEventsJson = NULL;
    *ppsOutputText = sOutputText;
    sOutputText = NULL;

cleanup:
    free(sStdout);
    free(sStderr);
    free(sEventsJson);
    free(sOutputText);
    xrtFree(pStdoutData);
    xrtFree(pStderrData);
    xrtFree(pEvents);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_start_terminal(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    xvalue tEnvList = NULL;
    const char *sCommand;
    const char *sSessionName = NULL;
    const char *sRequestedCwd = NULL;
    char *sResolvedCwd = NULL;
    char **psEnvEntries = NULL;
    xwork__local_host_terminal_session *pSession = NULL;
    xprocessconfig tProcessConfig;
    xprocessexitinfo tExitInfo;
    char *sEventsJson = NULL;
    char *sOutputText = NULL;
    bool bTerminalOutputCaptured = false;
    bool bRunning = false;
    bool bEventStreamDone = false;
    bool bHasTerminalCols = false;
    bool bHasTerminalRows = false;
    bool bHasMaxEvents = false;
    bool bInsertedSession = false;
    size_t iEnvCount = 0u;
    size_t iRequestedEnvCount = 0u;
    size_t iTerminalCols = 0u;
    size_t iTerminalRows = 0u;
    size_t iMaxEvents = 0u;
    size_t iOutputBytes = 0u;
    uint32 iEventCount = 0u;
    uint64 iNextAfterSeq = 0u;
    uint64 iEventEndSeq = 0u;
    const char *sFailureKind = NULL;
    const char *sFailureSummary = NULL;
    const char *sFailureMessage = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !pHost->bEnableProcessExec ) {
        return XWORK_ERROR_UNSUPPORTED;
    }
    if ( !xrtProcessTerminalSupported() ) {
        return XWORK_ERROR_UNSUPPORTED;
    }

    iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    sCommand = xwork__local_host_request_get_text(tRequest, "command");
    if ( !sCommand || !sCommand[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.start_terminal invalid request";
        sFailureMessage = "command is required";
        goto cleanup;
    }
    sSessionName = xwork__local_host_request_get_text(tRequest, "session_name");
    if ( sSessionName && !sSessionName[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.start_terminal invalid request";
        sFailureMessage = "session_name must not be empty";
        goto cleanup;
    }
    sRequestedCwd = xwork__local_host_request_get_text(tRequest, "cwd");
    if ( sRequestedCwd && !sRequestedCwd[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.start_terminal invalid request";
        sFailureMessage = "cwd must not be empty";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "terminal_cols",
        &bHasTerminalCols,
        &iTerminalCols
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.start_terminal invalid request";
        sFailureMessage = "terminal_cols must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "terminal_rows",
        &bHasTerminalRows,
        &iTerminalRows
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.start_terminal invalid request";
        sFailureMessage = "terminal_rows must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "max_events",
        &bHasMaxEvents,
        &iMaxEvents
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.start_terminal invalid request";
        sFailureMessage = "max_events must be a positive integer";
        goto cleanup;
    }
    if ( !bHasTerminalCols ) {
        iTerminalCols = XWORK__LOCAL_HOST_DEFAULT_TERMINAL_COLS;
    }
    if ( !bHasTerminalRows ) {
        iTerminalRows = XWORK__LOCAL_HOST_DEFAULT_TERMINAL_ROWS;
    }
    if ( iTerminalCols > 0xffffffffu || iTerminalRows > 0xffffffffu ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.start_terminal invalid request";
        sFailureMessage = "terminal size exceeds max supported value";
        goto cleanup;
    }

    iStatus = xwork__local_host_request_get_list(tRequest, "env", &tEnvList);
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.start_terminal invalid request";
        sFailureMessage = "env must be a list of KEY=VALUE strings";
        goto cleanup;
    }
    iRequestedEnvCount = xwork__local_host_list_count(tEnvList);
    iStatus = xwork__local_host_build_process_env_entries(
        tEnvList,
        pHost->iMaxProcessEnvEntries,
        &psEnvEntries,
        &iEnvCount
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        if ( pHost->iMaxProcessEnvEntries > 0u &&
             iRequestedEnvCount > pHost->iMaxProcessEnvEntries ) {
            sFailureSummary = "process.start_terminal invalid request (env limit exceeded)";
            sFailureMessage = "env exceeds max_process_env_entries";
        } else {
            sFailureSummary = "process.start_terminal invalid request";
            sFailureMessage = "env entries are invalid";
        }
        goto cleanup;
    }

    if ( sRequestedCwd ) {
        sResolvedCwd = xwork__local_host_resolve_path(pHost, sRequestedCwd);
        if ( !sResolvedCwd ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto cleanup;
        }
    } else if ( pHost->sDefaultWorkingDirectory && pHost->sDefaultWorkingDirectory[0] ) {
        sResolvedCwd = xwork__dup_cstr(pHost->sDefaultWorkingDirectory);
        if ( !sResolvedCwd ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto cleanup;
        }
    }

    pSession = (xwork__local_host_terminal_session *)calloc(1u, sizeof(*pSession));
    if ( !pSession ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }
    pSession->sSessionId = xwork__dup_printf(
        "terminal-session-%zu",
        pHost->iNextTerminalSessionId + 1u
    );
    pSession->sSessionName = xwork__dup_cstr(sSessionName ? sSessionName : "");
    pSession->sCommand = xwork__dup_cstr(sCommand);
    pSession->sResolvedCwd = xwork__dup_cstr(sResolvedCwd ? sResolvedCwd : "");
    pSession->iSessionIndex = pHost->iNextTerminalSessionId + 1u;
    pSession->iTerminalCols = iTerminalCols;
    pSession->iTerminalRows = iTerminalRows;
    if ( !pSession->sSessionId || !pSession->sSessionName ||
         !pSession->sCommand || !pSession->sResolvedCwd ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    memset(&tProcessConfig, 0, sizeof(tProcessConfig));
    memset(&tExitInfo, 0, sizeof(tExitInfo));
    xrtProcessConfigInit(&tProcessConfig);
    tProcessConfig.iTargetKind = XPROC_TARGET_SHELL;
    tProcessConfig.sCommand = (str)sCommand;
    tProcessConfig.sWorkDir =
        (str)((sResolvedCwd && sResolvedCwd[0]) ? sResolvedCwd : NULL);
    tProcessConfig.arrEnv = (str *)psEnvEntries;
    tProcessConfig.iEnvCount = (uint32)iEnvCount;
    tProcessConfig.bUseTerminal = true;
    tProcessConfig.bCreateProcessGroup = true;
    tProcessConfig.bHideWindow = true;
    tProcessConfig.iTerminalCols = (uint32)iTerminalCols;
    tProcessConfig.iTerminalRows = (uint32)iTerminalRows;
    tProcessConfig.iMaxCaptureBytes = pHost->iMaxProcessOutputBytes;
    tProcessConfig.Stdin.iMode = XPROC_STDIO_PIPE;
    tProcessConfig.Stdout.iMode = XPROC_STDIO_PIPE;
    tProcessConfig.Stderr.iMode = XPROC_STDIO_PIPE;

    pSession->pProcess = xrtProcessSpawn(&tProcessConfig);
    if ( !pSession->pProcess ) {
        iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
        sFailureKind = "spawn_failed";
        sFailureSummary = "process.start_terminal failed to start";
        sFailureMessage = "failed to start terminal session";
        goto cleanup;
    }

    pSession->pNext = (xwork__local_host_terminal_session *)pHost->pTerminalSessions;
    pHost->pTerminalSessions = pSession;
    ++pHost->iNextTerminalSessionId;
    bInsertedSession = true;

    iStatus = xwork__local_host_terminal_session_snapshot(
        pSession,
        0u,
        bHasMaxEvents ? iMaxEvents : 0u,
        &bTerminalOutputCaptured,
        &bRunning,
        &bEventStreamDone,
        &tExitInfo,
        &iEventCount,
        &iNextAfterSeq,
        &iEventEndSeq,
        &sEventsJson,
        &sOutputText,
        &iOutputBytes
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "snapshot_failed";
        sFailureSummary = "process.start_terminal failed";
        sFailureMessage = "failed to snapshot terminal session";
        goto cleanup;
    }

        iStatus = xwork__local_host_set_terminal_state_result(
            pHost,
            pSession->sSessionId,
            pSession->sSessionName,
            pSession->sCommand,
            pSession->sResolvedCwd,
            pSession->iSessionIndex,
            sOutputText,
            iOutputBytes,
            pSession->iTerminalCols,
            pSession->iTerminalRows,
            pSession->bStdinClosed,
            bTerminalOutputCaptured,
            bRunning,
            bEventStreamDone,
        false,
        iNextAfterSeq,
        iEventEndSeq,
        iEventCount,
        sEventsJson,
        NULL,
        &tExitInfo,
        true,
        "process.start_terminal ok",
        NULL,
        NULL,
        pResult
    );

cleanup:
    if ( iStatus != XWORK_OK && sFailureKind ) {
        (void)xwork__local_host_set_terminal_state_result(
            pHost,
            pSession ? pSession->sSessionId : "",
            pSession ? pSession->sSessionName : sSessionName,
            sCommand ? sCommand : "",
            sResolvedCwd ? sResolvedCwd : sRequestedCwd,
            pSession ? pSession->iSessionIndex : 0u,
            sOutputText,
            iOutputBytes,
            iTerminalCols,
            iTerminalRows,
            pSession ? pSession->bStdinClosed : false,
            bTerminalOutputCaptured,
            false,
            bEventStreamDone,
            false,
            iNextAfterSeq,
            iEventEndSeq,
            iEventCount,
            sEventsJson,
            NULL,
            &tExitInfo,
            false,
            sFailureSummary,
            sFailureKind,
            sFailureMessage,
            pResult
        );
    }
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sResolvedCwd);
    xwork__local_host_free_process_env_entries(&psEnvEntries, iEnvCount);
    free(sEventsJson);
    free(sOutputText);
    if ( pSession && !bInsertedSession ) {
        xwork__local_host_free_terminal_session(pSession);
    }
    if ( iStatus != XWORK_OK && bInsertedSession && pSession ) {
        xwork__local_host_remove_terminal_session(pHost, pSession);
    }
    return iStatus;
}

static xwork_status xwork__local_host_invoke_list_terminals(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    xwork__local_host_terminal_session *pSession;
    char *sSessionsJson = NULL;
    size_t iSessionCount = 0u;
    xwork_status iStatus = XWORK_OK;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( sRequestJson && sRequestJson[0] ) {
        iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
        if ( iStatus != XWORK_OK ) {
            return iStatus;
        }
    }

    sSessionsJson = xwork__dup_cstr("[");
    if ( !sSessionsJson ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    for ( pSession = (xwork__local_host_terminal_session *)pHost->pTerminalSessions;
          pSession;
          pSession = pSession->pNext ) {
        xprocessexitinfo tExitInfo;
        char *sEscapedSessionId = NULL;
        char *sEscapedSessionName = NULL;
        char *sEscapedCommand = NULL;
        char *sEscapedCwd = NULL;
        char *sEntryJson = NULL;
        char *sNextJson = NULL;
        bool bRunning = false;

        memset(&tExitInfo, 0, sizeof(tExitInfo));
        if ( pSession->pProcess ) {
            bRunning = xrtProcessIsRunning(pSession->pProcess);
            (void)xrtProcessGetExitInfo(pSession->pProcess, &tExitInfo);
        }

        iStatus = xwork__local_host_json_escape(
            pSession->sSessionId ? pSession->sSessionId : "",
            &sEscapedSessionId
        );
        if ( iStatus != XWORK_OK ) goto session_cleanup;
        iStatus = xwork__local_host_json_escape(
            pSession->sSessionName ? pSession->sSessionName : "",
            &sEscapedSessionName
        );
        if ( iStatus != XWORK_OK ) goto session_cleanup;
        iStatus = xwork__local_host_json_escape(
            pSession->sCommand ? pSession->sCommand : "",
            &sEscapedCommand
        );
        if ( iStatus != XWORK_OK ) goto session_cleanup;
        iStatus = xwork__local_host_json_escape(
            pSession->sResolvedCwd ? pSession->sResolvedCwd : "",
            &sEscapedCwd
        );
        if ( iStatus != XWORK_OK ) goto session_cleanup;

        sEntryJson = xwork__dup_printf(
            "%s{\"session_id\":\"%s\",\"session_name\":\"%s\",\"session_index\":%zu,"
            "\"command\":\"%s\",\"cwd\":\"%s\","
            "\"running\":%s,\"done\":%s,"
            "\"terminal_cols\":%zu,\"terminal_rows\":%zu,"
            "\"stdin_closed\":%s,"
            "\"exit_kind\":\"%s\",\"exit_code\":%d,\"signal\":%d,"
            "\"stage\":\"%s\",\"os_error\":%d,"
            "\"stop_reason\":\"%s\",\"timed_out\":%s,\"cancelled\":%s}",
            (iSessionCount > 0u) ? "," : "",
            sEscapedSessionId,
            sEscapedSessionName,
            pSession->iSessionIndex,
            sEscapedCommand,
            sEscapedCwd,
            bRunning ? "true" : "false",
            bRunning ? "false" : "true",
            pSession->iTerminalCols,
            pSession->iTerminalRows,
            pSession->bStdinClosed ? "true" : "false",
            xwork__local_host_process_exit_kind_name(tExitInfo.iKind),
            tExitInfo.iExitCode,
            tExitInfo.iSignal,
            xwork__local_host_process_exit_stage_name(tExitInfo.iStage),
            tExitInfo.iOsError,
            xwork__local_host_process_stop_reason_name(tExitInfo.iStopReason),
            tExitInfo.bTimedOut ? "true" : "false",
            tExitInfo.bCancelled ? "true" : "false"
        );
        if ( !sEntryJson ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto session_cleanup;
        }

        sNextJson = xwork__dup_printf("%s%s", sSessionsJson, sEntryJson);
        if ( !sNextJson ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto session_cleanup;
        }

        free(sSessionsJson);
        sSessionsJson = sNextJson;
        sNextJson = NULL;
        ++iSessionCount;

session_cleanup:
        free(sEscapedSessionId);
        free(sEscapedSessionName);
        free(sEscapedCommand);
        free(sEscapedCwd);
        free(sEntryJson);
        free(sNextJson);
        if ( iStatus != XWORK_OK ) {
            goto cleanup;
        }
    }

    {
        char *sClosedJson = xwork__dup_printf("%s]", sSessionsJson);
        if ( !sClosedJson ) {
            iStatus = XWORK_ERROR_NO_MEMORY;
            goto cleanup;
        }
        free(sSessionsJson);
        sSessionsJson = sClosedJson;
    }

    iStatus = xwork__local_host_set_terminal_list_result(
        pHost,
        iSessionCount,
        sSessionsJson,
        true,
        "process.list_terminals ok",
        NULL,
        NULL,
        pResult
    );

cleanup:
    if ( iStatus != XWORK_OK ) {
        (void)xwork__local_host_set_terminal_list_result(
            pHost,
            xwork__local_host_terminal_session_count(pHost),
            "[]",
            false,
            "process.list_terminals failed",
            "external_failure",
            "failed to build terminal session list",
            pResult
        );
    }
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sSessionsJson);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_terminal_read(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    const char *sSessionId;
    xwork__local_host_terminal_session *pSession = NULL;
    xprocessexitinfo tExitInfo;
    char *sEventsJson = NULL;
    char *sOutputText = NULL;
    bool bTerminalOutputCaptured = false;
    bool bRunning = false;
    bool bEventStreamDone = false;
    bool bHasAfterSeq = false;
    bool bHasMaxEvents = false;
    size_t iAfterSeq = 0u;
    size_t iMaxEvents = 0u;
    size_t iOutputBytes = 0u;
    uint32 iEventCount = 0u;
    uint64 iNextAfterSeq = 0u;
    uint64 iEventEndSeq = 0u;
    const char *sFailureKind = NULL;
    const char *sFailureSummary = NULL;
    const char *sFailureMessage = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    sSessionId = xwork__local_host_request_get_text(tRequest, "session_id");
    if ( !sSessionId || !sSessionId[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_read invalid request";
        sFailureMessage = "session_id is required";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "after_seq",
        &bHasAfterSeq,
        &iAfterSeq
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_read invalid request";
        sFailureMessage = "after_seq must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "max_events",
        &bHasMaxEvents,
        &iMaxEvents
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_read invalid request";
        sFailureMessage = "max_events must be a positive integer";
        goto cleanup;
    }

    pSession = xwork__local_host_find_terminal_session(pHost, sSessionId);
    if ( !pSession ) {
        iStatus = XWORK_ERROR_NOT_FOUND;
        sFailureKind = "not_found";
        sFailureSummary = "process.terminal_read failed";
        sFailureMessage = "session_id not found";
        goto cleanup;
    }

    memset(&tExitInfo, 0, sizeof(tExitInfo));
    iStatus = xwork__local_host_terminal_session_snapshot(
        pSession,
        bHasAfterSeq ? iAfterSeq : 0u,
        bHasMaxEvents ? iMaxEvents : 0u,
        &bTerminalOutputCaptured,
        &bRunning,
        &bEventStreamDone,
        &tExitInfo,
        &iEventCount,
        &iNextAfterSeq,
        &iEventEndSeq,
        &sEventsJson,
        &sOutputText,
        &iOutputBytes
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "snapshot_failed";
        sFailureSummary = "process.terminal_read failed";
        sFailureMessage = "failed to read terminal session";
        goto cleanup;
    }

    iStatus = xwork__local_host_set_terminal_state_result(
        pHost,
        pSession->sSessionId,
        pSession->sSessionName,
        pSession->sCommand,
        pSession->sResolvedCwd,
        pSession->iSessionIndex,
        sOutputText,
        iOutputBytes,
        pSession->iTerminalCols,
        pSession->iTerminalRows,
        pSession->bStdinClosed,
        bTerminalOutputCaptured,
        bRunning,
        bEventStreamDone,
        false,
        iNextAfterSeq,
        iEventEndSeq,
        iEventCount,
        sEventsJson,
        NULL,
        &tExitInfo,
        true,
        "process.terminal_read ok",
        NULL,
        NULL,
        pResult
    );

cleanup:
    if ( iStatus != XWORK_OK && sFailureKind ) {
        (void)xwork__local_host_set_terminal_state_result(
            pHost,
            sSessionId ? sSessionId : "",
            pSession ? pSession->sSessionName : "",
            pSession ? pSession->sCommand : "",
            pSession ? pSession->sResolvedCwd : NULL,
            pSession ? pSession->iSessionIndex : 0u,
            sOutputText,
            iOutputBytes,
            pSession ? pSession->iTerminalCols : 0u,
            pSession ? pSession->iTerminalRows : 0u,
            pSession ? pSession->bStdinClosed : false,
            bTerminalOutputCaptured,
            false,
            bEventStreamDone,
            false,
            iNextAfterSeq,
            iEventEndSeq,
            iEventCount,
            sEventsJson,
            NULL,
            &tExitInfo,
            false,
            sFailureSummary,
            sFailureKind,
            sFailureMessage,
            pResult
        );
    }
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sEventsJson);
    free(sOutputText);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_terminal_write(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    const char *sSessionId;
    const char *sInputText;
    xwork__local_host_terminal_session *pSession = NULL;
    xprocessexitinfo tExitInfo;
    char *sEventsJson = NULL;
    char *sOutputText = NULL;
    bool bTerminalOutputCaptured = false;
    bool bRunning = false;
    bool bEventStreamDone = false;
    bool bHasIncludeState = false;
    bool bIncludeState = false;
    bool bHasWriteEof = false;
    bool bWriteEof = false;
    bool bStdinClosed = false;
    bool bHasStateSnapshot = false;
    bool bHasAfterSeq = false;
    bool bHasMaxEvents = false;
    size_t iInputBytes = 0u;
    size_t iAfterSeq = 0u;
    size_t iMaxEvents = 0u;
    size_t iOutputBytes = 0u;
    uint32 iEventCount = 0u;
    uint64 iNextAfterSeq = 0u;
    uint64 iEventEndSeq = 0u;
    int64 iBytesWritten = 0;
    const char *sFailureKind = NULL;
    const char *sFailureSummary = NULL;
    const char *sFailureMessage = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    sSessionId = xwork__local_host_request_get_text(tRequest, "session_id");
    sInputText = xwork__local_host_request_get_text(tRequest, "input_text");
    if ( !sSessionId || !sSessionId[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_write invalid request";
        sFailureMessage = "session_id is required";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_bool(
        tRequest,
        "write_eof",
        &bHasWriteEof,
        &bWriteEof
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_write invalid request";
        sFailureMessage = "write_eof must be a boolean";
        goto cleanup;
    }
    if ( (!sInputText || !sInputText[0]) && !bWriteEof ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_write invalid request";
        sFailureMessage = "input_text or write_eof is required";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "after_seq",
        &bHasAfterSeq,
        &iAfterSeq
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_write invalid request";
        sFailureMessage = "after_seq must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "max_events",
        &bHasMaxEvents,
        &iMaxEvents
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_write invalid request";
        sFailureMessage = "max_events must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_bool(
        tRequest,
        "include_state",
        &bHasIncludeState,
        &bIncludeState
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_write invalid request";
        sFailureMessage = "include_state must be a boolean";
        goto cleanup;
    }
    bHasStateSnapshot = bIncludeState || bHasAfterSeq || bHasMaxEvents;
    iInputBytes = sInputText ? strlen(sInputText) : 0u;
    if ( sInputText &&
         pHost->iMaxProcessInputBytes > 0u &&
         iInputBytes > pHost->iMaxProcessInputBytes ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_write invalid request";
        sFailureMessage = "input_text exceeds max_process_input_bytes";
        goto cleanup;
    }

    pSession = xwork__local_host_find_terminal_session(pHost, sSessionId);
    if ( !pSession ) {
        iStatus = XWORK_ERROR_NOT_FOUND;
        sFailureKind = "not_found";
        sFailureSummary = "process.terminal_write failed";
        sFailureMessage = "session_id not found";
        goto cleanup;
    }
    if ( !pSession->pProcess || !xrtProcessIsRunning(pSession->pProcess) ) {
        iStatus = XWORK_ERROR_INVALID_STATE;
        sFailureKind = "invalid_state";
        sFailureSummary = "process.terminal_write failed";
        sFailureMessage = "terminal session is not running";
        goto cleanup;
    }

    if ( sInputText && iInputBytes > 0u ) {
        iBytesWritten = xrtProcessWriteText(
            pSession->pProcess,
            (str)sInputText,
            iInputBytes
        );
        if ( iBytesWritten < 0 || (size_t)iBytesWritten != iInputBytes ) {
            iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
            sFailureKind = "write_failed";
            sFailureSummary = "process.terminal_write failed";
            sFailureMessage = "failed to write terminal input";
            goto cleanup;
        }
    }
    if ( bWriteEof ) {
        if ( !xrtProcessCloseStdin(pSession->pProcess) ) {
            iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
            sFailureKind = "stdin_failed";
            sFailureSummary = "process.terminal_write failed";
            sFailureMessage = "failed to close terminal stdin";
            goto cleanup;
        }
        bStdinClosed = true;
        pSession->bStdinClosed = true;
    }

    if ( bHasStateSnapshot ) {
        memset(&tExitInfo, 0, sizeof(tExitInfo));
        iStatus = xwork__local_host_terminal_session_snapshot(
            pSession,
            bHasAfterSeq ? iAfterSeq : 0u,
            bHasMaxEvents ? iMaxEvents : 0u,
            &bTerminalOutputCaptured,
            &bRunning,
            &bEventStreamDone,
            &tExitInfo,
            &iEventCount,
            &iNextAfterSeq,
            &iEventEndSeq,
            &sEventsJson,
            &sOutputText,
            &iOutputBytes
        );
        if ( iStatus != XWORK_OK ) {
            sFailureKind = "snapshot_failed";
            sFailureSummary = "process.terminal_write failed";
            sFailureMessage = "failed to snapshot terminal session";
            goto cleanup;
        }
    }

    iStatus = xwork__local_host_set_terminal_write_result(
        pHost,
        pSession->sSessionId,
        pSession->sSessionName,
        pSession->iSessionIndex,
        iInputBytes,
        bWriteEof,
        bStdinClosed,
        bHasStateSnapshot,
        pSession->sCommand,
        pSession->sResolvedCwd,
        sOutputText,
        iOutputBytes,
        pSession->iTerminalCols,
        pSession->iTerminalRows,
        bTerminalOutputCaptured,
        bRunning,
        bEventStreamDone,
        iNextAfterSeq,
        iEventEndSeq,
        iEventCount,
        sEventsJson,
        true,
        "process.terminal_write ok",
        NULL,
        NULL,
        pResult
    );

cleanup:
    if ( iStatus != XWORK_OK && sFailureKind ) {
        (void)xwork__local_host_set_terminal_write_result(
            pHost,
            sSessionId ? sSessionId : "",
            pSession ? pSession->sSessionName : "",
            pSession ? pSession->iSessionIndex : 0u,
            (size_t)((iBytesWritten > 0) ? iBytesWritten : 0),
            bWriteEof,
            pSession ? pSession->bStdinClosed || bStdinClosed : bStdinClosed,
            bHasStateSnapshot,
            pSession ? pSession->sCommand : "",
            pSession ? pSession->sResolvedCwd : NULL,
            sOutputText,
            iOutputBytes,
            pSession ? pSession->iTerminalCols : 0u,
            pSession ? pSession->iTerminalRows : 0u,
            bTerminalOutputCaptured,
            false,
            bEventStreamDone,
            iNextAfterSeq,
            iEventEndSeq,
            iEventCount,
            sEventsJson,
            false,
            sFailureSummary,
            sFailureKind,
            sFailureMessage,
            pResult
        );
    }
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sEventsJson);
    free(sOutputText);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_terminal_resize(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    const char *sSessionId;
    xwork__local_host_terminal_session *pSession = NULL;
    xprocessexitinfo tExitInfo;
    char *sEventsJson = NULL;
    char *sOutputText = NULL;
    bool bTerminalOutputCaptured = false;
    bool bRunning = false;
    bool bEventStreamDone = false;
    bool bHasAfterSeq = false;
    bool bHasMaxEvents = false;
    bool bHasTerminalCols = false;
    bool bHasTerminalRows = false;
    size_t iAfterSeq = 0u;
    size_t iMaxEvents = 0u;
    size_t iTerminalCols = 0u;
    size_t iTerminalRows = 0u;
    size_t iOutputBytes = 0u;
    uint32 iEventCount = 0u;
    uint64 iNextAfterSeq = 0u;
    uint64 iEventEndSeq = 0u;
    bool bResizeApplied = false;
    char sResizeExtraJson[32] = "";
    const char *sFailureKind = NULL;
    const char *sFailureSummary = NULL;
    const char *sFailureMessage = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    sSessionId = xwork__local_host_request_get_text(tRequest, "session_id");
    if ( !sSessionId || !sSessionId[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_resize invalid request";
        sFailureMessage = "session_id is required";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "terminal_cols",
        &bHasTerminalCols,
        &iTerminalCols
    );
    if ( iStatus != XWORK_OK || !bHasTerminalCols ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_resize invalid request";
        sFailureMessage = "terminal_cols must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "terminal_rows",
        &bHasTerminalRows,
        &iTerminalRows
    );
    if ( iStatus != XWORK_OK || !bHasTerminalRows ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_resize invalid request";
        sFailureMessage = "terminal_rows must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "after_seq",
        &bHasAfterSeq,
        &iAfterSeq
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_resize invalid request";
        sFailureMessage = "after_seq must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "max_events",
        &bHasMaxEvents,
        &iMaxEvents
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_resize invalid request";
        sFailureMessage = "max_events must be a positive integer";
        goto cleanup;
    }
    if ( iTerminalCols > 0xffffffffu || iTerminalRows > 0xffffffffu ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_resize invalid request";
        sFailureMessage = "terminal size exceeds max supported value";
        goto cleanup;
    }

    pSession = xwork__local_host_find_terminal_session(pHost, sSessionId);
    if ( !pSession ) {
        iStatus = XWORK_ERROR_NOT_FOUND;
        sFailureKind = "not_found";
        sFailureSummary = "process.terminal_resize failed";
        sFailureMessage = "session_id not found";
        goto cleanup;
    }
    bResizeApplied = xrtProcessResizeTerminal(
        pSession->pProcess,
        (uint32)iTerminalCols,
        (uint32)iTerminalRows
    );
    pSession->iTerminalCols = iTerminalCols;
    pSession->iTerminalRows = iTerminalRows;
    snprintf(
        sResizeExtraJson,
        sizeof(sResizeExtraJson),
        "\"resize_applied\":%s,",
        bResizeApplied ? "true" : "false"
    );

    memset(&tExitInfo, 0, sizeof(tExitInfo));
    iStatus = xwork__local_host_terminal_session_snapshot(
        pSession,
        bHasAfterSeq ? iAfterSeq : 0u,
        bHasMaxEvents ? iMaxEvents : 0u,
        &bTerminalOutputCaptured,
        &bRunning,
        &bEventStreamDone,
        &tExitInfo,
        &iEventCount,
        &iNextAfterSeq,
        &iEventEndSeq,
        &sEventsJson,
        &sOutputText,
        &iOutputBytes
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "snapshot_failed";
        sFailureSummary = "process.terminal_resize failed";
        sFailureMessage = "failed to snapshot terminal session";
        goto cleanup;
    }

    iStatus = xwork__local_host_set_terminal_state_result(
        pHost,
        pSession->sSessionId,
        pSession->sSessionName,
        pSession->sCommand,
        pSession->sResolvedCwd,
        pSession->iSessionIndex,
        sOutputText,
        iOutputBytes,
        pSession->iTerminalCols,
        pSession->iTerminalRows,
        pSession->bStdinClosed,
        bTerminalOutputCaptured,
        bRunning,
        bEventStreamDone,
        false,
        iNextAfterSeq,
        iEventEndSeq,
        iEventCount,
        sEventsJson,
        sResizeExtraJson,
        &tExitInfo,
        true,
        "process.terminal_resize ok",
        NULL,
        NULL,
        pResult
    );

cleanup:
    if ( iStatus != XWORK_OK && sFailureKind ) {
        (void)xwork__local_host_set_terminal_state_result(
            pHost,
            sSessionId ? sSessionId : "",
            pSession ? pSession->sSessionName : "",
            pSession ? pSession->sCommand : "",
            pSession ? pSession->sResolvedCwd : NULL,
            pSession ? pSession->iSessionIndex : 0u,
            sOutputText,
            iOutputBytes,
            pSession ? pSession->iTerminalCols : iTerminalCols,
            pSession ? pSession->iTerminalRows : iTerminalRows,
            pSession ? pSession->bStdinClosed : false,
            bTerminalOutputCaptured,
            false,
            bEventStreamDone,
            false,
            iNextAfterSeq,
            iEventEndSeq,
            iEventCount,
            sEventsJson,
            sResizeExtraJson,
            &tExitInfo,
            false,
            sFailureSummary,
            sFailureKind,
            sFailureMessage,
            pResult
        );
    }
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sEventsJson);
    free(sOutputText);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_terminal_stop(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    const char *sSessionId;
    const char *sStopText = NULL;
    xwork__local_host_terminal_session *pSession = NULL;
    xprocessexitinfo tExitInfo;
    char *sEventsJson = NULL;
    char *sOutputText = NULL;
    bool bTerminalOutputCaptured = false;
    bool bRunning = false;
    bool bEventStreamDone = false;
    bool bHasAfterSeq = false;
    bool bHasMaxEvents = false;
    size_t iAfterSeq = 0u;
    size_t iMaxEvents = 0u;
    size_t iOutputBytes = 0u;
    uint32 iEventCount = 0u;
    uint64 iNextAfterSeq = 0u;
    uint64 iEventEndSeq = 0u;
    int iRequestedStopReason = XPROC_STOP_INTERRUPT;
    const char *sFailureKind = NULL;
    const char *sFailureSummary = NULL;
    const char *sFailureMessage = NULL;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    sSessionId = xwork__local_host_request_get_text(tRequest, "session_id");
    if ( !sSessionId || !sSessionId[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_stop invalid request";
        sFailureMessage = "session_id is required";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "after_seq",
        &bHasAfterSeq,
        &iAfterSeq
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_stop invalid request";
        sFailureMessage = "after_seq must be a positive integer";
        goto cleanup;
    }
    iStatus = xwork__local_host_request_get_positive_size_strict(
        tRequest,
        "max_events",
        &bHasMaxEvents,
        &iMaxEvents
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_stop invalid request";
        sFailureMessage = "max_events must be a positive integer";
        goto cleanup;
    }
    sStopText = xwork__local_host_request_get_text(tRequest, "stop");
    iStatus = xwork__local_host_parse_process_stop_reason(
        sStopText,
        &iRequestedStopReason
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "invalid_request";
        sFailureSummary = "process.terminal_stop invalid request";
        sFailureMessage = "stop must be one of interrupt, terminate, kill, kill_tree";
        goto cleanup;
    }

    pSession = xwork__local_host_find_terminal_session(pHost, sSessionId);
    if ( !pSession ) {
        iStatus = XWORK_ERROR_NOT_FOUND;
        sFailureKind = "not_found";
        sFailureSummary = "process.terminal_stop failed";
        sFailureMessage = "session_id not found";
        goto cleanup;
    }

    memset(&tExitInfo, 0, sizeof(tExitInfo));
    if ( pSession->pProcess && xrtProcessIsRunning(pSession->pProcess) ) {
        int iObservedStopReason = xwork__local_host_process_stop_best_effort(
            pSession->pProcess,
            iRequestedStopReason
        );
        (void)xrtProcessGetExitInfo(pSession->pProcess, &tExitInfo);
        if ( tExitInfo.iStopReason == XPROC_STOP_NONE ) {
            tExitInfo.iStopReason = iObservedStopReason;
        }
    }

    iStatus = xwork__local_host_terminal_session_snapshot(
        pSession,
        bHasAfterSeq ? iAfterSeq : 0u,
        bHasMaxEvents ? iMaxEvents : 0u,
        &bTerminalOutputCaptured,
        &bRunning,
        &bEventStreamDone,
        &tExitInfo,
        &iEventCount,
        &iNextAfterSeq,
        &iEventEndSeq,
        &sEventsJson,
        &sOutputText,
        &iOutputBytes
    );
    if ( iStatus != XWORK_OK ) {
        sFailureKind = "snapshot_failed";
        sFailureSummary = "process.terminal_stop failed";
        sFailureMessage = "failed to snapshot terminal session";
        goto cleanup;
    }

    iStatus = xwork__local_host_set_terminal_state_result(
        pHost,
        pSession->sSessionId,
        pSession->sSessionName,
        pSession->sCommand,
        pSession->sResolvedCwd,
        pSession->iSessionIndex,
        sOutputText,
        iOutputBytes,
        pSession->iTerminalCols,
        pSession->iTerminalRows,
        pSession->bStdinClosed,
        bTerminalOutputCaptured,
        bRunning,
        bEventStreamDone,
        true,
        iNextAfterSeq,
        iEventEndSeq,
        iEventCount,
        sEventsJson,
        NULL,
        &tExitInfo,
        true,
        "process.terminal_stop ok",
        NULL,
        NULL,
        pResult
    );
    if ( iStatus == XWORK_OK ) {
        xwork__local_host_remove_terminal_session(pHost, pSession);
        pSession = NULL;
    }

cleanup:
    if ( iStatus != XWORK_OK && sFailureKind ) {
        (void)xwork__local_host_set_terminal_state_result(
            pHost,
            sSessionId ? sSessionId : "",
            pSession ? pSession->sSessionName : "",
            pSession ? pSession->sCommand : "",
            pSession ? pSession->sResolvedCwd : NULL,
            pSession ? pSession->iSessionIndex : 0u,
            sOutputText,
            iOutputBytes,
            pSession ? pSession->iTerminalCols : 0u,
            pSession ? pSession->iTerminalRows : 0u,
            pSession ? pSession->bStdinClosed : false,
            bTerminalOutputCaptured,
            false,
            bEventStreamDone,
            false,
            iNextAfterSeq,
            iEventEndSeq,
            iEventCount,
            sEventsJson,
            NULL,
            &tExitInfo,
            false,
            sFailureSummary,
            sFailureKind,
            sFailureMessage,
            pResult
        );
    }
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sEventsJson);
    free(sOutputText);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_vcs(
    xwork_local_host *pHost,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    xvalue tRequest = NULL;
    const char *sPath = ".";
    char *sResolvedPath = NULL;
    char *sQuotedPath = NULL;
    char *sCommand = NULL;
    char *sOutput = NULL;
    char *sEscapedPath = NULL;
    char *sEscapedResolvedPath = NULL;
    char *sEscapedOutput = NULL;
    char *sOutputText = NULL;
    bool bTruncated = false;
    size_t iRequestMaxBytes = 0u;
    size_t iCaptureLimit;
    int iExitCode = -1;
    xwork_status iStatus;

    if ( !pHost || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( !pHost->bEnableVcsStatus ) {
        return XWORK_ERROR_UNSUPPORTED;
    }

    iStatus = xwork__local_host_parse_request_json(sRequestJson, &tRequest);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    if ( xwork__local_host_request_get_text(tRequest, "path") ) {
        sPath = xwork__local_host_request_get_text(tRequest, "path");
    }

    (void)xwork__local_host_request_get_size(tRequest, "max_output_bytes", &iRequestMaxBytes);
    iCaptureLimit = xwork__local_host_effective_limit(
        pHost->iMaxProcessOutputBytes,
        iRequestMaxBytes
    );

    sResolvedPath = xwork__local_host_resolve_path(pHost, sPath);
    if ( !sResolvedPath ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    sQuotedPath = xwork__local_host_quote_shell_arg(sResolvedPath);
    if ( !sQuotedPath ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        goto cleanup;
    }
    sCommand = xwork__dup_printf(
        "git -C %s status --short --branch 2>&1",
        sQuotedPath
    );
    if ( !sCommand ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_capture_command_output(
        sCommand,
        iCaptureLimit,
        &sOutput,
        &iExitCode,
        &bTruncated
    );
    if ( iStatus != XWORK_OK ) {
        goto cleanup;
    }
    if ( iExitCode != 0 ) {
        iStatus = XWORK_ERROR_EXTERNAL_FAILURE;
        goto cleanup;
    }

    iStatus = xwork__local_host_json_escape(sPath, &sEscapedPath);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(sResolvedPath, &sEscapedResolvedPath);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(sOutput, &sEscapedOutput);
    if ( iStatus != XWORK_OK ) goto cleanup;

    sOutputText = xwork__dup_printf(
        "{\"ok\":true,\"path\":\"%s\",\"resolved_path\":\"%s\",\"status\":\"%s\","
        "\"truncated\":%s}",
        sEscapedPath,
        sEscapedResolvedPath,
        sEscapedOutput,
        bTruncated ? "true" : "false"
    );
    if ( !sOutputText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sOutputText,
        "vcs.status ok",
        pResult
    );

cleanup:
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sResolvedPath);
    free(sQuotedPath);
    free(sCommand);
    free(sOutput);
    free(sEscapedPath);
    free(sEscapedResolvedPath);
    free(sEscapedOutput);
    free(sOutputText);
    return iStatus;
}

static xwork_status xwork__local_host_invoke_filesystem_cb(
    const char *sOperationId,
    const char *sRequestJson,
    xwork_tool_result *pResult,
    void *pUserData
)
{
    if ( !sOperationId ) {
        return XWORK_ERROR_UNSUPPORTED;
    }
    if ( strcmp(sOperationId, XWORK_HOST_FILESYSTEM_READ_TEXT) == 0 ) {
        return xwork__local_host_invoke_filesystem(
            (xwork_local_host *)pUserData,
            sRequestJson,
            pResult
        );
    }
    if ( strcmp(sOperationId, XWORK_HOST_FILESYSTEM_WRITE_TEXT) == 0 ) {
        return xwork__local_host_invoke_filesystem_write_text(
            (xwork_local_host *)pUserData,
            sRequestJson,
            pResult
        );
    }
    return XWORK_ERROR_UNSUPPORTED;
}

static xwork_status xwork__local_host_invoke_process_cb(
    const char *sOperationId,
    const char *sRequestJson,
    xwork_tool_result *pResult,
    void *pUserData
)
{
    xwork_local_host *pHost = (xwork_local_host *)pUserData;

    if ( !sOperationId ) {
        return XWORK_ERROR_UNSUPPORTED;
    }
    if ( strcmp(sOperationId, XWORK_HOST_PROCESS_EXEC) == 0 ) {
        return xwork__local_host_invoke_process(
            pHost,
            sRequestJson,
            pResult
        );
    }
    if ( strcmp(sOperationId, XWORK_HOST_PROCESS_START_TERMINAL) == 0 ) {
        return xwork__local_host_invoke_start_terminal(
            pHost,
            sRequestJson,
            pResult
        );
    }
    if ( strcmp(sOperationId, XWORK_HOST_PROCESS_LIST_TERMINALS) == 0 ) {
        return xwork__local_host_invoke_list_terminals(
            pHost,
            sRequestJson,
            pResult
        );
    }
    if ( strcmp(sOperationId, XWORK_HOST_PROCESS_TERMINAL_READ) == 0 ) {
        return xwork__local_host_invoke_terminal_read(
            pHost,
            sRequestJson,
            pResult
        );
    }
    if ( strcmp(sOperationId, XWORK_HOST_PROCESS_TERMINAL_WRITE) == 0 ) {
        return xwork__local_host_invoke_terminal_write(
            pHost,
            sRequestJson,
            pResult
        );
    }
    if ( strcmp(sOperationId, XWORK_HOST_PROCESS_TERMINAL_RESIZE) == 0 ) {
        return xwork__local_host_invoke_terminal_resize(
            pHost,
            sRequestJson,
            pResult
        );
    }
    if ( strcmp(sOperationId, XWORK_HOST_PROCESS_TERMINAL_STOP) == 0 ) {
        return xwork__local_host_invoke_terminal_stop(
            pHost,
            sRequestJson,
            pResult
        );
    }
    return XWORK_ERROR_UNSUPPORTED;
}

static xwork_status xwork__local_host_invoke_vcs_cb(
    const char *sOperationId,
    const char *sRequestJson,
    xwork_tool_result *pResult,
    void *pUserData
)
{
    if ( !sOperationId || strcmp(sOperationId, XWORK_HOST_VCS_STATUS) != 0 ) {
        return XWORK_ERROR_UNSUPPORTED;
    }
    return xwork__local_host_invoke_vcs(
        (xwork_local_host *)pUserData,
        sRequestJson,
        pResult
    );
}

void xwork_host_service_init(xwork_host_service *pService)
{
    if ( pService ) {
        memset(pService, 0, sizeof(*pService));
    }
}

void xwork_host_services_init(xwork_host_services *pServices)
{
    if ( pServices ) {
        memset(pServices, 0, sizeof(*pServices));
    }
}

void xwork_local_host_options_init(xwork_local_host_options *pOptions)
{
    if ( pOptions ) {
        memset(pOptions, 0, sizeof(*pOptions));
        pOptions->iMaxReadBytes = XWORK__LOCAL_HOST_DEFAULT_READ_BYTES;
        pOptions->iMaxProcessInputBytes = XWORK__LOCAL_HOST_DEFAULT_PROCESS_INPUT_BYTES;
        pOptions->iMaxProcessEnvEntries = XWORK__LOCAL_HOST_DEFAULT_PROCESS_ENV_ENTRIES;
        pOptions->iMaxProcessOutputBytes = XWORK__LOCAL_HOST_DEFAULT_PROCESS_OUTPUT_BYTES;
        pOptions->bEnableFilesystemReadText = true;
        pOptions->bEnableFilesystemWriteText = true;
        pOptions->bEnableProcessExec = true;
        pOptions->bEnableVcsStatus = true;
    }
}

void xwork_local_host_init(xwork_local_host *pHost)
{
    if ( pHost ) {
        memset(pHost, 0, sizeof(*pHost));
    }
}

void xwork_local_host_reset(xwork_local_host *pHost)
{
    if ( !pHost ) {
        return;
    }

    xwork__local_host_reset_terminal_sessions(pHost);
    xwork__free_cstr(&pHost->sDefaultWorkingDirectory);
    xwork__free_cstr(&pHost->sLastOutputText);
    xwork__free_cstr(&pHost->sLastVisibleSummary);
    xwork_local_host_init(pHost);
}

xwork_status xwork_local_host_configure_services(
    xwork_local_host *pHost,
    const xwork_local_host_options *pOptions,
    xwork_host_services *pServices
)
{
    if ( !pHost || !pOptions || !pServices ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    xwork_local_host_reset(pHost);
    if ( pOptions->sDefaultWorkingDirectory && pOptions->sDefaultWorkingDirectory[0] ) {
        pHost->sDefaultWorkingDirectory = xwork__dup_cstr(
            pOptions->sDefaultWorkingDirectory
        );
        if ( !pHost->sDefaultWorkingDirectory ) {
            return XWORK_ERROR_NO_MEMORY;
        }
    }

    pHost->iMaxReadBytes = pOptions->iMaxReadBytes;
    pHost->iMaxProcessInputBytes = pOptions->iMaxProcessInputBytes;
    pHost->iMaxProcessEnvEntries = pOptions->iMaxProcessEnvEntries;
    pHost->iMaxProcessOutputBytes = pOptions->iMaxProcessOutputBytes;
    pHost->bEnableFilesystemReadText = pOptions->bEnableFilesystemReadText;
    pHost->bEnableFilesystemWriteText = pOptions->bEnableFilesystemWriteText;
    pHost->bEnableProcessExec = pOptions->bEnableProcessExec;
    pHost->bEnableVcsStatus = pOptions->bEnableVcsStatus;

    xwork_host_service_init(&pServices->tFilesystem);
    xwork_host_service_init(&pServices->tProcess);
    xwork_host_service_init(&pServices->tVcs);

    if ( pHost->bEnableFilesystemReadText || pHost->bEnableFilesystemWriteText ) {
        pServices->tFilesystem.pfnInvoke = xwork__local_host_invoke_filesystem_cb;
        pServices->tFilesystem.pUserData = pHost;
    }
    if ( pHost->bEnableProcessExec ) {
        pServices->tProcess.pfnInvoke = xwork__local_host_invoke_process_cb;
        pServices->tProcess.pUserData = pHost;
    }
    if ( pHost->bEnableVcsStatus ) {
        pServices->tVcs.pfnInvoke = xwork__local_host_invoke_vcs_cb;
        pServices->tVcs.pUserData = pHost;
    }

    return XWORK_OK;
}

xwork_status xwork_runtime_invoke_host_service(
    const xwork_runtime *pRuntime,
    xwork_host_service_kind eKind,
    const char *sOperationId,
    const char *sRequestJson,
    xwork_tool_result *pResult
)
{
    const xwork_host_service *pService;

    if ( !pRuntime || !sOperationId || !sOperationId[0] || !pResult ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    pService = xwork__runtime_get_host_service_slot(pRuntime, eKind);
    if ( !pService || !pService->pfnInvoke ) {
        return XWORK_ERROR_UNSUPPORTED;
    }

    xwork_tool_result_init(pResult);
    return pService->pfnInvoke(sOperationId, sRequestJson, pResult, pService->pUserData);
}
