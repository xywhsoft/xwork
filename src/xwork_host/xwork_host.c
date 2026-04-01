#include "../xwork_core/xwork_internal.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define XWORK__HOST_POPEN _popen
#define XWORK__HOST_PCLOSE _pclose
#else
#include <sys/wait.h>
#define XWORK__HOST_POPEN popen
#define XWORK__HOST_PCLOSE pclose
#endif

#define XWORK__LOCAL_HOST_DEFAULT_READ_BYTES (64u * 1024u)
#define XWORK__LOCAL_HOST_DEFAULT_PROCESS_OUTPUT_BYTES (64u * 1024u)

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
    size_t iMaxBytes,
    char **ppsText,
    bool *pbTruncated
)
{
    FILE *pFile = NULL;
    char *sText = NULL;
    size_t iReadBytes;

    if ( !sPath || !sPath[0] || !ppsText || !pbTruncated ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppsText = NULL;
    *pbTruncated = false;

    pFile = fopen(sPath, "rb");
    if ( !pFile ) {
        return (errno == ENOENT) ? XWORK_ERROR_NOT_FOUND : XWORK_ERROR_EXTERNAL_FAILURE;
    }

    if ( fseek(pFile, 0, SEEK_END) != 0 ) {
        fclose(pFile);
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }
    iReadBytes = (size_t)ftell(pFile);
    if ( fseek(pFile, 0, SEEK_SET) != 0 ) {
        fclose(pFile);
        return XWORK_ERROR_EXTERNAL_FAILURE;
    }

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
    return XWORK_OK;
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

static xwork_status xwork__local_host_build_process_env_prefix(
    xvalue tEnvList,
    char **ppsPrefix,
    size_t *piEnvCount
)
{
    char *sPrefix = NULL;
    uint32 iCount = 0u;
    uint32 i;

    if ( !ppsPrefix ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    *ppsPrefix = NULL;
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
    for ( i = 0u; i < iCount; ++i ) {
        xvalue tItem = (xvoType(tEnvList) == XVO_DT_ARRAY)
            ? xvoArrayGetValue(tEnvList, i)
            : xvoListGetValue(tEnvList, (int64)i);
        const char *sEntry;
        const char *sEquals;
        size_t iNameLength;
        const char *sValue;
        char *sSegment = NULL;

        if ( !tItem || xvoType(tItem) != XVO_DT_TEXT ) {
            free(sPrefix);
            return XWORK_ERROR_INVALID_ARGUMENT;
        }

        sEntry = (const char *)xvoGetText(tItem);
        if ( !sEntry ) {
            free(sPrefix);
            return XWORK_ERROR_INVALID_ARGUMENT;
        }

        sEquals = strchr(sEntry, '=');
        if ( !sEquals || sEquals == sEntry ) {
            free(sPrefix);
            return XWORK_ERROR_INVALID_ARGUMENT;
        }

        iNameLength = (size_t)(sEquals - sEntry);
        if ( !xwork__local_host_is_valid_env_name(sEntry, iNameLength) ) {
            free(sPrefix);
            return XWORK_ERROR_INVALID_ARGUMENT;
        }

        sValue = sEquals + 1;

#ifdef _WIN32
        if ( strchr(sValue, '"') != NULL ) {
            free(sPrefix);
            return XWORK_ERROR_INVALID_ARGUMENT;
        }
        sSegment = xwork__dup_printf(
            "set \"%.*s=%s\"",
            (int)iNameLength,
            sEntry,
            sValue
        );
#else
        {
            char *sQuotedValue = xwork__local_host_quote_shell_arg(sValue);

            if ( !sQuotedValue ) {
                free(sPrefix);
                return XWORK_ERROR_INVALID_ARGUMENT;
            }
            sSegment = xwork__dup_printf(
                "export %.*s=%s",
                (int)iNameLength,
                sEntry,
                sQuotedValue
            );
            free(sQuotedValue);
        }
#endif
        if ( !sSegment ) {
            free(sPrefix);
            return XWORK_ERROR_NO_MEMORY;
        }

        if ( !sPrefix ) {
            sPrefix = sSegment;
        } else {
            char *sNextPrefix = xwork__dup_printf("%s && %s", sPrefix, sSegment);

            free(sPrefix);
            free(sSegment);
            if ( !sNextPrefix ) {
                return XWORK_ERROR_NO_MEMORY;
            }
            sPrefix = sNextPrefix;
        }
    }

    *ppsPrefix = sPrefix;
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
    char *sEscapedPath = NULL;
    char *sEscapedResolvedPath = NULL;
    char *sEscapedText = NULL;
    char *sOutputText = NULL;
    bool bTruncated = false;
    size_t iRequestMaxBytes = 0u;
    size_t iReadLimit;
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
        goto cleanup;
    }

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
        iReadLimit,
        &sText,
        &bTruncated
    );
    if ( iStatus != XWORK_OK ) {
        goto cleanup;
    }

    iStatus = xwork__local_host_json_escape(sPath, &sEscapedPath);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(sResolvedPath, &sEscapedResolvedPath);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(sText, &sEscapedText);
    if ( iStatus != XWORK_OK ) goto cleanup;

    sOutputText = xwork__dup_printf(
        "{\"ok\":true,\"path\":\"%s\",\"resolved_path\":\"%s\",\"text\":\"%s\","
        "\"truncated\":%s}",
        sEscapedPath,
        sEscapedResolvedPath,
        sEscapedText,
        bTruncated ? "true" : "false"
    );
    if ( !sOutputText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sOutputText,
        "filesystem.read_text ok",
        pResult
    );

cleanup:
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sResolvedPath);
    free(sText);
    free(sEscapedPath);
    free(sEscapedResolvedPath);
    free(sEscapedText);
    free(sOutputText);
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
    char *sEscapedPath = NULL;
    char *sEscapedResolvedPath = NULL;
    char *sEscapedMode = NULL;
    char *sOutputText = NULL;
    size_t iBytesWritten = 0u;
    bool bAppend = false;
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
        goto cleanup;
    }
    if ( sMode && sMode[0] ) {
        if ( strcmp(sMode, "append") == 0 ) {
            bAppend = true;
        } else if ( strcmp(sMode, "overwrite") != 0 ) {
            iStatus = XWORK_ERROR_INVALID_ARGUMENT;
            goto cleanup;
        }
    }

    sResolvedPath = xwork__local_host_resolve_path(pHost, sPath);
    if ( !sResolvedPath ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_write_text_file(
        sResolvedPath,
        sText,
        bAppend,
        &iBytesWritten
    );
    if ( iStatus != XWORK_OK ) {
        goto cleanup;
    }

    iStatus = xwork__local_host_json_escape(sPath, &sEscapedPath);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(sResolvedPath, &sEscapedResolvedPath);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        bAppend ? "append" : "overwrite",
        &sEscapedMode
    );
    if ( iStatus != XWORK_OK ) goto cleanup;

    sOutputText = xwork__dup_printf(
        "{\"ok\":true,\"path\":\"%s\",\"resolved_path\":\"%s\",\"mode\":\"%s\",\"bytes_written\":%llu}",
        sEscapedPath,
        sEscapedResolvedPath,
        sEscapedMode,
        (unsigned long long)iBytesWritten
    );
    if ( !sOutputText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sOutputText,
        "filesystem.write_text ok",
        pResult
    );

cleanup:
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sResolvedPath);
    free(sEscapedPath);
    free(sEscapedResolvedPath);
    free(sEscapedMode);
    free(sOutputText);
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
    char *sResolvedCwd = NULL;
    char *sQuotedCwd = NULL;
    char *sEnvPrefix = NULL;
    char *sShellCommand = NULL;
    char *sOutput = NULL;
    char *sEscapedCommand = NULL;
    char *sEscapedOutput = NULL;
    char *sEscapedCwd = NULL;
    char *sOutputText = NULL;
    bool bTruncated = false;
    size_t iRequestMaxBytes = 0u;
    size_t iCaptureLimit;
    size_t iEnvCount = 0u;
    int iExitCode = -1;
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
        goto cleanup;
    }
    sRequestedCwd = xwork__local_host_request_get_text(tRequest, "cwd");
    if ( sRequestedCwd && !sRequestedCwd[0] ) {
        iStatus = XWORK_ERROR_INVALID_ARGUMENT;
        goto cleanup;
    }

    (void)xwork__local_host_request_get_size(tRequest, "max_output_bytes", &iRequestMaxBytes);
    iCaptureLimit = xwork__local_host_effective_limit(
        pHost->iMaxProcessOutputBytes,
        iRequestMaxBytes
    );
    iStatus = xwork__local_host_request_get_list(tRequest, "env", &tEnvList);
    if ( iStatus != XWORK_OK ) {
        goto cleanup;
    }
    iStatus = xwork__local_host_build_process_env_prefix(
        tEnvList,
        &sEnvPrefix,
        &iEnvCount
    );
    if ( iStatus != XWORK_OK ) {
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

    if ( sResolvedCwd && sResolvedCwd[0] ) {
        sQuotedCwd = xwork__local_host_quote_shell_arg(sResolvedCwd);
        if ( !sQuotedCwd ) {
            iStatus = XWORK_ERROR_INVALID_ARGUMENT;
            goto cleanup;
        }
        if ( sEnvPrefix ) {
#ifdef _WIN32
            sShellCommand = xwork__dup_printf(
                "cd /d %s && %s && %s 2>&1",
                sQuotedCwd,
                sEnvPrefix,
                sCommand
            );
#else
            sShellCommand = xwork__dup_printf(
                "cd %s && %s && %s 2>&1",
                sQuotedCwd,
                sEnvPrefix,
                sCommand
            );
#endif
        } else {
#ifdef _WIN32
            sShellCommand = xwork__dup_printf(
                "cd /d %s && %s 2>&1",
                sQuotedCwd,
                sCommand
            );
#else
            sShellCommand = xwork__dup_printf(
                "cd %s && %s 2>&1",
                sQuotedCwd,
                sCommand
            );
#endif
        }
    } else if ( sEnvPrefix ) {
        sShellCommand = xwork__dup_printf("%s && %s 2>&1", sEnvPrefix, sCommand);
    } else {
        sShellCommand = xwork__dup_printf("%s 2>&1", sCommand);
    }
    if ( !sShellCommand ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_capture_command_output(
        sShellCommand,
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

    iStatus = xwork__local_host_json_escape(sCommand, &sEscapedCommand);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(sOutput, &sEscapedOutput);
    if ( iStatus != XWORK_OK ) goto cleanup;
    iStatus = xwork__local_host_json_escape(
        sResolvedCwd ? sResolvedCwd : "",
        &sEscapedCwd
    );
    if ( iStatus != XWORK_OK ) goto cleanup;

    sOutputText = xwork__dup_printf(
        "{\"ok\":true,\"command\":\"%s\",\"cwd\":\"%s\",\"stdout\":\"%s\","
        "\"exit_code\":%d,\"truncated\":%s,\"env_count\":%zu}",
        sEscapedCommand,
        sEscapedCwd,
        sEscapedOutput,
        iExitCode,
        bTruncated ? "true" : "false",
        iEnvCount
    );
    if ( !sOutputText ) {
        iStatus = XWORK_ERROR_NO_MEMORY;
        goto cleanup;
    }

    iStatus = xwork__local_host_set_result(
        pHost,
        sOutputText,
        "process.exec ok",
        pResult
    );

cleanup:
    if ( tRequest ) {
        xvoUnref(tRequest);
    }
    free(sResolvedCwd);
    free(sQuotedCwd);
    free(sEnvPrefix);
    free(sShellCommand);
    free(sOutput);
    free(sEscapedCommand);
    free(sEscapedOutput);
    free(sEscapedCwd);
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
    if ( !sOperationId || strcmp(sOperationId, XWORK_HOST_PROCESS_EXEC) != 0 ) {
        return XWORK_ERROR_UNSUPPORTED;
    }
    return xwork__local_host_invoke_process(
        (xwork_local_host *)pUserData,
        sRequestJson,
        pResult
    );
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
