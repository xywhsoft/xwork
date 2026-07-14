typedef struct xwork_walk_entry {
    const char* sAbsolutePath;
    const char* sRelativePath;
    const char* sName;
    bool bDirectory;
    uint64_t uSize;
    uint32_t uDepth;
} xwork_walk_entry;

typedef bool (*xwork_walk_fn)(void* pUserData, const xwork_walk_entry* pEntry);

static bool xwork__buf_append_escaped_bytes(xwork_buf* pBuf, const unsigned char* pData, size_t iSize)
{
    size_t i;
    for ( i = 0u; i < iSize; ++i ) {
        unsigned char c = pData[i];
        if ( c == '\n' || c == '\r' || c == '\t' || (c >= 0x20u && c <= 0x7Eu) ) {
            if ( !xwork__buf_append_char(pBuf, (char)c) ) return false;
        } else if ( !xwork__buf_appendf(pBuf, "\\x%02X", (unsigned int)c) ) {
            return false;
        }
    }
    return true;
}

/* Process pipes are byte streams. Tool results, however, are JSON text and
 * therefore must be valid UTF-8 before they reach the model provider. */
static bool xwork__buf_append_process_text(xwork_buf* pBuf, const void* pData, size_t iSize)
{
    const unsigned char* pBytes = (const unsigned char*)pData;
    if ( iSize == 0u ) return true;
    if ( !pData ) return false;
    if ( memchr(pData, 0, iSize) == NULL && xrtIsUTF8((str)pData, iSize) ) {
        return xwork__buf_append(pBuf, pData, iSize);
    }
#if defined(_WIN32)
    if ( memchr(pData, 0, iSize) == NULL ) {
        char* sConverted = (char*)xrtConvCharset((ptr)pData, iSize, (int)GetOEMCP(), XRT_CP_UTF8, NULL);
        if ( sConverted && sConverted[0] && xrtIsUTF8((str)sConverted, strlen(sConverted)) ) {
            bool bOk = xwork__buf_append_cstr(pBuf, sConverted);
            xrtFree(sConverted);
            return bOk;
        }
        if ( sConverted ) xrtFree(sConverted);
    }
#endif
    return xwork__buf_append_escaped_bytes(pBuf, pBytes, iSize);
}

typedef struct xwork_walk_context {
    xwork_agent* pAgent;
    xwork_walk_fn OnEntry;
    void* pUserData;
    bool bRecursive;
    bool bStopped;
    uint32_t uMaxDepth;
} xwork_walk_context;

static bool xwork__is_skipped_directory(const char* sName)
{
    return strcmp(sName, ".") == 0 || strcmp(sName, "..") == 0 ||
           strcmp(sName, ".git") == 0 || strcmp(sName, ".xcode") == 0;
}

static bool xwork__wildcard_match(const char* sPattern, const char* sText)
{
    const char* sStar = NULL;
    const char* sRetry = NULL;
    if ( !sPattern || !sPattern[0] || strcmp(sPattern, "*") == 0 ) return true;
    while ( *sText ) {
        if ( *sPattern == '?' || *sPattern == *sText ) { ++sPattern; ++sText; continue; }
        if ( *sPattern == '*' ) { sStar = sPattern++; sRetry = sText; continue; }
        if ( sStar ) { sPattern = sStar + 1; sText = ++sRetry; continue; }
        return false;
    }
    while ( *sPattern == '*' ) ++sPattern;
    return *sPattern == '\0';
}

static bool xwork__looks_binary(const unsigned char* pData, size_t iSize)
{
    size_t i;
    size_t iCheck = iSize < 8192u ? iSize : 8192u;
    for ( i = 0u; i < iCheck; ++i ) if ( pData[i] == 0u ) return true;
    return false;
}

static bool xwork__walk_directory(xwork_walk_context* pContext, const char* sDirectory, uint32_t uDepth)
{
#if defined(_WIN32)
    WIN32_FIND_DATAW tData;
    HANDLE hFind;
    char* sPattern = (char*)xrtPathJoin(2u, sDirectory, "*");
    u16str sPatternWide;
    if ( !sPattern ) return false;
    sPatternWide = xrtUTF8to16((u8str)sPattern, 0u, NULL);
    xrtFree(sPattern);
    if ( !sPatternWide ) return false;
    hFind = FindFirstFileW((LPCWSTR)sPatternWide, &tData);
    xrtFree(sPatternWide);
    if ( hFind == INVALID_HANDLE_VALUE ) return false;
    do {
        char* sPath;
        char* sRelative;
        char* sName;
        xwork_walk_entry tEntry;
        bool bDir = (tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        sName = (char*)xrtUTF16to8((u16str)tData.cFileName, 0u, NULL);
        if ( !sName ) { FindClose(hFind); return false; }
        if ( xwork__is_skipped_directory(sName) ) { xrtFree(sName); continue; }
        sPath = (char*)xrtPathJoin(2u, sDirectory, sName);
        if ( !sPath ) { xrtFree(sName); FindClose(hFind); return false; }
        sRelative = xwork__relative_path(pContext->pAgent, sPath);
        if ( !sRelative ) { xrtFree(sName); xrtFree(sPath); FindClose(hFind); return false; }
        memset(&tEntry, 0, sizeof(tEntry));
        tEntry.sAbsolutePath = sPath;
        tEntry.sRelativePath = sRelative;
        tEntry.sName = sName;
        tEntry.bDirectory = bDir;
        tEntry.uSize = ((uint64_t)tData.nFileSizeHigh << 32u) | tData.nFileSizeLow;
        tEntry.uDepth = uDepth;
        if ( !pContext->OnEntry(pContext->pUserData, &tEntry) ) pContext->bStopped = true;
        if ( !pContext->bStopped && bDir && pContext->bRecursive && uDepth < pContext->uMaxDepth ) {
            (void)xwork__walk_directory(pContext, sPath, uDepth + 1u);
        }
        free(sRelative);
        xrtFree(sPath);
        xrtFree(sName);
        if ( pContext->bStopped ) break;
    } while ( FindNextFileW(hFind, &tData) );
    FindClose(hFind);
#else
    DIR* pDir = opendir(sDirectory);
    struct dirent* pItem;
    if ( !pDir ) return false;
    while ( !pContext->bStopped && (pItem = readdir(pDir)) != NULL ) {
        char* sPath;
        char* sRelative;
        struct stat tStat;
        xwork_walk_entry tEntry;
        bool bDir;
        if ( xwork__is_skipped_directory(pItem->d_name) ) continue;
        sPath = (char*)xrtPathJoin(2u, sDirectory, pItem->d_name);
        if ( !sPath ) { closedir(pDir); return false; }
        if ( stat(sPath, &tStat) != 0 ) { xrtFree(sPath); continue; }
        bDir = S_ISDIR(tStat.st_mode);
        sRelative = xwork__relative_path(pContext->pAgent, sPath);
        if ( !sRelative ) { xrtFree(sPath); closedir(pDir); return false; }
        memset(&tEntry, 0, sizeof(tEntry));
        tEntry.sAbsolutePath = sPath;
        tEntry.sRelativePath = sRelative;
        tEntry.sName = pItem->d_name;
        tEntry.bDirectory = bDir;
        tEntry.uSize = bDir ? 0u : (uint64_t)tStat.st_size;
        tEntry.uDepth = uDepth;
        if ( !pContext->OnEntry(pContext->pUserData, &tEntry) ) pContext->bStopped = true;
        if ( !pContext->bStopped && bDir && pContext->bRecursive && uDepth < pContext->uMaxDepth ) {
            (void)xwork__walk_directory(pContext, sPath, uDepth + 1u);
        }
        free(sRelative);
        xrtFree(sPath);
    }
    closedir(pDir);
#endif
    return true;
}

static bool xwork__walk(
    xwork_agent* pAgent,
    const char* sRoot,
    bool bRecursive,
    uint32_t uMaxDepth,
    xwork_walk_fn OnEntry,
    void* pUserData
)
{
    xwork_walk_context tContext;
    memset(&tContext, 0, sizeof(tContext));
    tContext.pAgent = pAgent;
    tContext.OnEntry = OnEntry;
    tContext.pUserData = pUserData;
    tContext.bRecursive = bRecursive;
    tContext.uMaxDepth = uMaxDepth;
    return xwork__walk_directory(&tContext, sRoot, 1u);
}

static xwork_result xwork__tool_fail(xwork_tool_output* pOutput, const char* sMessage)
{
    if ( !xworkToolOutputSet(pOutput, false, sMessage) ) return XWORK_RESULT_ERROR;
    return XWORK_RESULT_OK;
}

static xwork_result xwork__tool_read_file(
    void* pUserData,
    const xwork_tool_context* pContext,
    const char* sArgumentsJson,
    xwork_tool_output* pOutput,
    xwork_error* pError
)
{
    xvalue tArgs = NULL;
    const char* sPath;
    char* sResolved = NULL;
    unsigned char* pData = NULL;
    size_t iSize = 0u;
    uint64_t uStartLine;
    uint64_t uMaxLines;
    bool bValid;
    uint64_t uLine = 1u;
    uint64_t uEmitted = 0u;
    size_t i = 0u;
    xwork_buf tOutput = {0};
    xwork_result eResult = XWORK_RESULT_ERROR;
    xwork_agent* pAgent = (xwork_agent*)pUserData;
    (void)pContext;
    tArgs = xwork__json_parse_object(sArgumentsJson);
    if ( !tArgs ) return xwork__tool_fail(pOutput, "invalid arguments: expected a JSON object");
    sPath = xwork__json_text(tArgs, "path");
    uStartLine = xwork__json_u64(tArgs, "start_line", 1u, &bValid);
    if ( !bValid || uStartLine == 0u ) { eResult = xwork__tool_fail(pOutput, "invalid start_line"); goto cleanup; }
    uMaxLines = xwork__json_u64(tArgs, "max_lines", 400u, &bValid);
    if ( !bValid || uMaxLines == 0u || uMaxLines > 10000u ) { eResult = xwork__tool_fail(pOutput, "max_lines must be between 1 and 10000"); goto cleanup; }
    if ( !sPath || !sPath[0] ) { eResult = xwork__tool_fail(pOutput, "path is required"); goto cleanup; }
    sResolved = xwork__resolve_path(pAgent, sPath, pError);
    if ( !sResolved ) { eResult = xwork__tool_fail(pOutput, pError && pError->sMessage[0] ? pError->sMessage : "path denied"); goto cleanup; }
    if ( !xrtFileExists((str)sResolved) ) { eResult = xwork__tool_fail(pOutput, "file does not exist"); goto cleanup; }
    if ( xrtFileGetSize((str)sResolved) > 64u * 1024u * 1024u ) { eResult = xwork__tool_fail(pOutput, "file is larger than the 64 MiB read limit"); goto cleanup; }
    pData = (unsigned char*)xrtFileGetAll((str)sResolved, &iSize);
    if ( !pData && iSize ) { eResult = xwork__tool_fail(pOutput, "failed to read file"); goto cleanup; }
    if ( xwork__looks_binary(pData, iSize) ) { eResult = xwork__tool_fail(pOutput, "file appears to be binary"); goto cleanup; }
    if ( !xwork__buf_appendf(&tOutput, "file: %s (%zu bytes)\n", sPath, iSize) ) goto oom;
    while ( i < iSize && uEmitted < uMaxLines ) {
        size_t iStart = i;
        size_t iLen;
        while ( i < iSize && pData[i] != '\n' ) ++i;
        iLen = i - iStart;
        if ( iLen && pData[iStart + iLen - 1u] == '\r' ) --iLen;
        if ( uLine >= uStartLine ) {
            if ( !xwork__buf_appendf(&tOutput, "%6llu | ", (unsigned long long)uLine) ||
                 !xwork__buf_append(&tOutput, pData + iStart, iLen) ||
                 !xwork__buf_append_char(&tOutput, '\n') ) goto oom;
            ++uEmitted;
        }
        if ( i < iSize ) ++i;
        ++uLine;
    }
    if ( uEmitted == uMaxLines && i < iSize && !xwork__buf_appendf(&tOutput, "[more lines available; continue at start_line=%llu]\n", (unsigned long long)uLine) ) goto oom;
    if ( uStartLine >= uLine && i >= iSize && !xwork__buf_append_cstr(&tOutput, "[start_line is beyond end of file]\n") ) goto oom;
    if ( !xworkToolOutputSet(pOutput, true, tOutput.pData ? tOutput.pData : "") ) goto oom;
    eResult = XWORK_RESULT_OK;
    goto cleanup;
oom:
    xwork__set_error(pError, XWORK_ERROR_OUT_OF_MEMORY, "failed to build read_file output");
cleanup:
    if ( tArgs ) xvoUnref(tArgs);
    free(sResolved);
    if ( pData && iSize ) xrtFree(pData);
    xwork__buf_unit(&tOutput);
    return eResult;
}

typedef struct xwork_list_context {
    xwork_buf tOutput;
    const char* sPattern;
    uint64_t uLimit;
    uint64_t uCount;
} xwork_list_context;

static bool xwork__list_entry(void* pUserData, const xwork_walk_entry* pEntry)
{
    xwork_list_context* pContext = (xwork_list_context*)pUserData;
    if ( pContext->uCount >= pContext->uLimit ) return false;
    if ( pContext->sPattern && pContext->sPattern[0] &&
         !xwork__wildcard_match(pContext->sPattern, pEntry->sName) &&
         !xwork__wildcard_match(pContext->sPattern, pEntry->sRelativePath) ) return true;
    if ( pEntry->bDirectory ) {
        if ( !xwork__buf_appendf(&pContext->tOutput, "%s/\n", pEntry->sRelativePath) ) return false;
    } else if ( !xwork__buf_appendf(
            &pContext->tOutput,
            "%s\t%llu bytes\n",
            pEntry->sRelativePath,
            (unsigned long long)pEntry->uSize
         ) ) return false;
    ++pContext->uCount;
    return pContext->uCount < pContext->uLimit;
}

static xwork_result xwork__tool_list_files(
    void* pUserData,
    const xwork_tool_context* pContext,
    const char* sArgumentsJson,
    xwork_tool_output* pOutput,
    xwork_error* pError
)
{
    xwork_agent* pAgent = (xwork_agent*)pUserData;
    xvalue tArgs = xwork__json_parse_object(sArgumentsJson);
    const char* sPath;
    const char* sPattern;
    char* sResolved = NULL;
    bool bValid;
    bool bRecursive;
    uint64_t uLimit;
    uint64_t uDepth;
    xwork_list_context tList;
    xwork_result eResult = XWORK_RESULT_ERROR;
    (void)pContext;
    memset(&tList, 0, sizeof(tList));
    if ( !tArgs ) return xwork__tool_fail(pOutput, "invalid arguments: expected a JSON object");
    sPath = xwork__json_text(tArgs, "path");
    if ( !sPath || !sPath[0] ) sPath = ".";
    sPattern = xwork__json_text(tArgs, "pattern");
    bRecursive = xwork__json_bool(tArgs, "recursive", false, &bValid);
    if ( !bValid ) { eResult = xwork__tool_fail(pOutput, "recursive must be boolean"); goto cleanup; }
    uLimit = xwork__json_u64(tArgs, "max_results", 500u, &bValid);
    if ( !bValid || uLimit == 0u || uLimit > 10000u ) { eResult = xwork__tool_fail(pOutput, "max_results must be between 1 and 10000"); goto cleanup; }
    uDepth = xwork__json_u64(tArgs, "max_depth", 12u, &bValid);
    if ( !bValid || uDepth == 0u || uDepth > 64u ) { eResult = xwork__tool_fail(pOutput, "max_depth must be between 1 and 64"); goto cleanup; }
    sResolved = xwork__resolve_path(pAgent, sPath, pError);
    if ( !sResolved ) { eResult = xwork__tool_fail(pOutput, pError && pError->sMessage[0] ? pError->sMessage : "path denied"); goto cleanup; }
    if ( !xrtDirExists((str)sResolved) ) { eResult = xwork__tool_fail(pOutput, "directory does not exist"); goto cleanup; }
    tList.sPattern = sPattern;
    tList.uLimit = uLimit;
    if ( !xwork__buf_appendf(&tList.tOutput, "listing: %s (recursive=%s)\n", sPath, bRecursive ? "true" : "false") ) goto oom;
    if ( !xwork__walk(pAgent, sResolved, bRecursive, (uint32_t)uDepth, xwork__list_entry, &tList) && !tList.tOutput.pData ) {
        eResult = xwork__tool_fail(pOutput, "failed to scan directory"); goto cleanup;
    }
    if ( tList.uCount >= tList.uLimit && !xwork__buf_append_cstr(&tList.tOutput, "[result limit reached; narrow the path or pattern]\n") ) goto oom;
    if ( tList.uCount == 0u && !xwork__buf_append_cstr(&tList.tOutput, "[no matching entries]\n") ) goto oom;
    if ( !xworkToolOutputSet(pOutput, true, tList.tOutput.pData) ) goto oom;
    eResult = XWORK_RESULT_OK;
    goto cleanup;
oom:
    xwork__set_error(pError, XWORK_ERROR_OUT_OF_MEMORY, "failed to build list_files output");
cleanup:
    if ( tArgs ) xvoUnref(tArgs);
    free(sResolved);
    xwork__buf_unit(&tList.tOutput);
    return eResult;
}

typedef struct xwork_search_context {
    xwork_buf tOutput;
    const char* sQuery;
    const char* sPattern;
    uint64_t uLimit;
    uint64_t uMatches;
    uint64_t uFiles;
} xwork_search_context;

static bool xwork__search_entry(void* pUserData, const xwork_walk_entry* pEntry)
{
    xwork_search_context* pContext = (xwork_search_context*)pUserData;
    unsigned char* pData;
    size_t iSize = 0u;
    size_t i = 0u;
    uint64_t uLine = 1u;
    if ( pEntry->bDirectory ) return true;
    if ( pContext->uMatches >= pContext->uLimit ) return false;
    if ( pContext->sPattern && pContext->sPattern[0] &&
         !xwork__wildcard_match(pContext->sPattern, pEntry->sName) &&
         !xwork__wildcard_match(pContext->sPattern, pEntry->sRelativePath) ) return true;
    if ( pEntry->uSize > 4u * 1024u * 1024u ) return true;
    pData = (unsigned char*)xrtFileGetAll((str)pEntry->sAbsolutePath, &iSize);
    if ( !pData || xwork__looks_binary(pData, iSize) ) { if ( pData ) xrtFree(pData); return true; }
    ++pContext->uFiles;
    while ( i < iSize && pContext->uMatches < pContext->uLimit ) {
        size_t iStart = i;
        size_t iLen;
        char* sLine;
        while ( i < iSize && pData[i] != '\n' ) ++i;
        iLen = i - iStart;
        if ( iLen && pData[iStart + iLen - 1u] == '\r' ) --iLen;
        sLine = (char*)malloc(iLen + 1u);
        if ( !sLine ) { if ( iSize ) xrtFree(pData); return false; }
        memcpy(sLine, pData + iStart, iLen);
        sLine[iLen] = '\0';
        if ( strstr(sLine, pContext->sQuery) ) {
            size_t iShown = iLen < 1200u ? iLen : 1200u;
            if ( !xwork__buf_appendf(&pContext->tOutput, "%s:%llu: ", pEntry->sRelativePath, (unsigned long long)uLine) ||
                 !xwork__buf_append(&pContext->tOutput, sLine, iShown) ||
                 !xwork__buf_append_char(&pContext->tOutput, '\n') ) {
                free(sLine); if ( iSize ) xrtFree(pData); return false;
            }
            ++pContext->uMatches;
        }
        free(sLine);
        if ( i < iSize ) ++i;
        ++uLine;
    }
    if ( iSize ) xrtFree(pData);
    return pContext->uMatches < pContext->uLimit;
}

static xwork_result xwork__tool_search_text(
    void* pUserData,
    const xwork_tool_context* pContext,
    const char* sArgumentsJson,
    xwork_tool_output* pOutput,
    xwork_error* pError
)
{
    xwork_agent* pAgent = (xwork_agent*)pUserData;
    xvalue tArgs = xwork__json_parse_object(sArgumentsJson);
    const char* sPath;
    const char* sQuery;
    char* sResolved = NULL;
    bool bValid;
    uint64_t uLimit;
    uint64_t uDepth;
    xwork_search_context tSearch;
    xwork_result eResult = XWORK_RESULT_ERROR;
    (void)pContext;
    memset(&tSearch, 0, sizeof(tSearch));
    if ( !tArgs ) return xwork__tool_fail(pOutput, "invalid arguments: expected a JSON object");
    sPath = xwork__json_text(tArgs, "path");
    if ( !sPath || !sPath[0] ) sPath = ".";
    sQuery = xwork__json_text(tArgs, "query");
    if ( !sQuery || !sQuery[0] ) { eResult = xwork__tool_fail(pOutput, "query is required"); goto cleanup; }
    uLimit = xwork__json_u64(tArgs, "max_results", 200u, &bValid);
    if ( !bValid || uLimit == 0u || uLimit > 5000u ) { eResult = xwork__tool_fail(pOutput, "max_results must be between 1 and 5000"); goto cleanup; }
    uDepth = xwork__json_u64(tArgs, "max_depth", 24u, &bValid);
    if ( !bValid || uDepth == 0u || uDepth > 64u ) { eResult = xwork__tool_fail(pOutput, "max_depth must be between 1 and 64"); goto cleanup; }
    sResolved = xwork__resolve_path(pAgent, sPath, pError);
    if ( !sResolved ) { eResult = xwork__tool_fail(pOutput, pError && pError->sMessage[0] ? pError->sMessage : "path denied"); goto cleanup; }
    if ( !xrtDirExists((str)sResolved) ) { eResult = xwork__tool_fail(pOutput, "search path must be a directory"); goto cleanup; }
    tSearch.sQuery = sQuery;
    tSearch.sPattern = xwork__json_text(tArgs, "pattern");
    tSearch.uLimit = uLimit;
    if ( !xwork__buf_appendf(&tSearch.tOutput, "search: %s in %s\n", sQuery, sPath) ) goto oom;
    (void)xwork__walk(pAgent, sResolved, true, (uint32_t)uDepth, xwork__search_entry, &tSearch);
    if ( tSearch.uMatches == 0u && !xwork__buf_appendf(&tSearch.tOutput, "[no matches in %llu text files]\n", (unsigned long long)tSearch.uFiles) ) goto oom;
    if ( tSearch.uMatches >= tSearch.uLimit && !xwork__buf_append_cstr(&tSearch.tOutput, "[result limit reached; narrow the query, path, or pattern]\n") ) goto oom;
    if ( !xworkToolOutputSet(pOutput, true, tSearch.tOutput.pData) ) goto oom;
    eResult = XWORK_RESULT_OK;
    goto cleanup;
oom:
    xwork__set_error(pError, XWORK_ERROR_OUT_OF_MEMORY, "failed to build search_text output");
cleanup:
    if ( tArgs ) xvoUnref(tArgs);
    free(sResolved);
    xwork__buf_unit(&tSearch.tOutput);
    return eResult;
}

static bool xwork__write_bytes(const char* sPath, const char* sContent, bool bAppend)
{
    size_t iLen = strlen(sContent);
    if ( iLen == 0u ) {
        FILE* pFile = fopen(sPath, bAppend ? "ab" : "wb");
        if ( !pFile ) return false;
        fclose(pFile);
        return true;
    }
    return bAppend
        ? xrtFileAppend((str)sPath, (str)sContent, iLen, XRT_CP_UTF8) == (int)iLen
        : xrtFilePutAll((str)sPath, (ptr)sContent, iLen) == (int)iLen;
}

static xwork_result xwork__tool_write_file(
    void* pUserData,
    const xwork_tool_context* pContext,
    const char* sArgumentsJson,
    xwork_tool_output* pOutput,
    xwork_error* pError
)
{
    xwork_agent* pAgent = (xwork_agent*)pUserData;
    xvalue tArgs = xwork__json_parse_object(sArgumentsJson);
    const char* sPath;
    const char* sContent;
    const char* sMode;
    char* sResolved = NULL;
    bool bValid;
    bool bCreateDirs;
    bool bAppend = false;
    bool bCreate = false;
    xwork_buf tOutput = {0};
    xwork_result eResult = XWORK_RESULT_ERROR;
    (void)pContext;
    if ( !tArgs ) return xwork__tool_fail(pOutput, "invalid arguments: expected a JSON object");
    sPath = xwork__json_text(tArgs, "path");
    sContent = xwork__json_text(tArgs, "content");
    sMode = xwork__json_text(tArgs, "mode");
    if ( !sMode || !sMode[0] ) sMode = "overwrite";
    if ( !sPath || !sPath[0] || !sContent ) { eResult = xwork__tool_fail(pOutput, "path and content are required"); goto cleanup; }
    if ( strcmp(sMode, "append") == 0 ) bAppend = true;
    else if ( strcmp(sMode, "create") == 0 ) bCreate = true;
    else if ( strcmp(sMode, "overwrite") != 0 ) { eResult = xwork__tool_fail(pOutput, "mode must be overwrite, append, or create"); goto cleanup; }
    bCreateDirs = xwork__json_bool(tArgs, "create_dirs", true, &bValid);
    if ( !bValid ) { eResult = xwork__tool_fail(pOutput, "create_dirs must be boolean"); goto cleanup; }
    sResolved = xwork__resolve_path(pAgent, sPath, pError);
    if ( !sResolved ) { eResult = xwork__tool_fail(pOutput, pError && pError->sMessage[0] ? pError->sMessage : "path denied"); goto cleanup; }
    if ( bCreate && xrtPathExists((str)sResolved) ) { eResult = xwork__tool_fail(pOutput, "create conflict: target already exists"); goto cleanup; }
    if ( bCreateDirs && !xwork__ensure_parent(sResolved) ) { eResult = xwork__tool_fail(pOutput, "failed to create parent directories"); goto cleanup; }
    if ( !xwork__write_bytes(sResolved, sContent, bAppend) ) { eResult = xwork__tool_fail(pOutput, "failed to write file"); goto cleanup; }
    if ( !xwork__buf_appendf(&tOutput, "wrote %zu bytes to %s (mode=%s)", strlen(sContent), sPath, sMode) ||
         !xworkToolOutputSet(pOutput, true, tOutput.pData) ) goto oom;
    eResult = XWORK_RESULT_OK;
    goto cleanup;
oom:
    xwork__set_error(pError, XWORK_ERROR_OUT_OF_MEMORY, "failed to build write_file output");
cleanup:
    if ( tArgs ) xvoUnref(tArgs);
    free(sResolved);
    xwork__buf_unit(&tOutput);
    return eResult;
}

static xwork_result xwork__tool_replace_text(
    void* pUserData,
    const xwork_tool_context* pContext,
    const char* sArgumentsJson,
    xwork_tool_output* pOutput,
    xwork_error* pError
)
{
    xwork_agent* pAgent = (xwork_agent*)pUserData;
    xvalue tArgs = xwork__json_parse_object(sArgumentsJson);
    const char* sPath;
    const char* sOld;
    const char* sNew;
    char* sResolved = NULL;
    char* sCurrent = NULL;
    size_t iCurrent = 0u;
    bool bValid;
    bool bAll;
    const char* pScan;
    const char* pMatch;
    size_t iOld;
    size_t iNew;
    uint64_t uMatches = 0u;
    xwork_buf tNext = {0};
    xwork_buf tOutput = {0};
    xwork_result eResult = XWORK_RESULT_ERROR;
    (void)pContext;
    if ( !tArgs ) return xwork__tool_fail(pOutput, "invalid arguments: expected a JSON object");
    sPath = xwork__json_text(tArgs, "path");
    sOld = xwork__json_text(tArgs, "old_text");
    sNew = xwork__json_text(tArgs, "new_text");
    if ( !sPath || !sPath[0] || !sOld || !sOld[0] || !sNew ) { eResult = xwork__tool_fail(pOutput, "path, non-empty old_text, and new_text are required"); goto cleanup; }
    bAll = xwork__json_bool(tArgs, "replace_all", false, &bValid);
    if ( !bValid ) { eResult = xwork__tool_fail(pOutput, "replace_all must be boolean"); goto cleanup; }
    sResolved = xwork__resolve_path(pAgent, sPath, pError);
    if ( !sResolved ) { eResult = xwork__tool_fail(pOutput, pError && pError->sMessage[0] ? pError->sMessage : "path denied"); goto cleanup; }
    if ( !xrtFileExists((str)sResolved) ) { eResult = xwork__tool_fail(pOutput, "file does not exist"); goto cleanup; }
    sCurrent = (char*)xrtFileReadAll((str)sResolved, XRT_CP_UTF8, &iCurrent);
    if ( !sCurrent && iCurrent ) { eResult = xwork__tool_fail(pOutput, "failed to read file"); goto cleanup; }
    iOld = strlen(sOld);
    iNew = strlen(sNew);
    pScan = sCurrent ? sCurrent : "";
    while ( (pMatch = strstr(pScan, sOld)) != NULL ) {
        if ( !xwork__buf_append(&tNext, pScan, (size_t)(pMatch - pScan)) ||
             !xwork__buf_append(&tNext, sNew, iNew) ) goto oom;
        ++uMatches;
        pScan = pMatch + iOld;
        if ( !bAll ) break;
    }
    if ( uMatches == 0u ) { eResult = xwork__tool_fail(pOutput, "replace conflict: old_text was not found"); goto cleanup; }
    if ( !bAll && strstr(pScan, sOld) != NULL ) { eResult = xwork__tool_fail(pOutput, "replace conflict: old_text occurs more than once; add more context or set replace_all=true"); goto cleanup; }
    if ( !xwork__buf_append_cstr(&tNext, pScan) ) goto oom;
    if ( !xwork__write_bytes(sResolved, tNext.pData ? tNext.pData : "", false) ) { eResult = xwork__tool_fail(pOutput, "failed to write replaced file"); goto cleanup; }
    if ( !xwork__buf_appendf(&tOutput, "replaced %llu occurrence%s in %s (%zu -> %zu bytes)",
            (unsigned long long)uMatches, uMatches == 1u ? "" : "s", sPath, iCurrent, tNext.iLen) ||
         !xworkToolOutputSet(pOutput, true, tOutput.pData) ) goto oom;
    eResult = XWORK_RESULT_OK;
    goto cleanup;
oom:
    xwork__set_error(pError, XWORK_ERROR_OUT_OF_MEMORY, "failed to apply text replacement");
cleanup:
    if ( tArgs ) xvoUnref(tArgs);
    free(sResolved);
    if ( sCurrent && iCurrent ) xrtFree(sCurrent);
    xwork__buf_unit(&tNext);
    xwork__buf_unit(&tOutput);
    return eResult;
}

static xwork_result xwork__tool_exec_command(
    void* pUserData,
    const xwork_tool_context* pContext,
    const char* sArgumentsJson,
    xwork_tool_output* pOutput,
    xwork_error* pError
)
{
    xwork_agent* pAgent = (xwork_agent*)pUserData;
    xvalue tArgs = xwork__json_parse_object(sArgumentsJson);
    const char* sCommand;
    const char* sCwd;
    char* sResolvedCwd = NULL;
    bool bValid;
    bool bMerge;
    uint64_t uTimeout;
    xprocessconfig tConfig;
    xprocessresult tProcess;
    xwork_buf tOutput = {0};
    xwork_result eResult = XWORK_RESULT_ERROR;
    (void)pContext;
    memset(&tProcess, 0, sizeof(tProcess));
    if ( !tArgs ) return xwork__tool_fail(pOutput, "invalid arguments: expected a JSON object");
    sCommand = xwork__json_text(tArgs, "command");
    sCwd = xwork__json_text(tArgs, "cwd");
    if ( !sCwd || !sCwd[0] ) sCwd = ".";
    if ( !sCommand || !sCommand[0] ) { eResult = xwork__tool_fail(pOutput, "command is required"); goto cleanup; }
    uTimeout = xwork__json_u64(tArgs, "timeout_ms", pAgent->uCommandTimeoutMs, &bValid);
    if ( !bValid || uTimeout == 0u || uTimeout > 3600000u ) { eResult = xwork__tool_fail(pOutput, "timeout_ms must be between 1 and 3600000"); goto cleanup; }
    bMerge = xwork__json_bool(tArgs, "merge_stderr", true, &bValid);
    if ( !bValid ) { eResult = xwork__tool_fail(pOutput, "merge_stderr must be boolean"); goto cleanup; }
    sResolvedCwd = xwork__resolve_path(pAgent, sCwd, pError);
    if ( !sResolvedCwd ) { eResult = xwork__tool_fail(pOutput, pError && pError->sMessage[0] ? pError->sMessage : "cwd denied"); goto cleanup; }
    if ( !xrtDirExists((str)sResolvedCwd) ) { eResult = xwork__tool_fail(pOutput, "cwd does not exist"); goto cleanup; }
    xrtProcessConfigInit(&tConfig);
    tConfig.iTargetKind = XPROC_TARGET_SHELL;
    tConfig.sCommand = (str)sCommand;
    tConfig.sWorkDir = (str)sResolvedCwd;
    tConfig.bInheritEnv = true;
    tConfig.bMergeStderr = bMerge;
    tConfig.bCreateProcessGroup = true;
    tConfig.bHideWindow = true;
    tConfig.iMaxCaptureBytes = pAgent->iMaxCapturedCommandBytes;
    tConfig.Stdout.iMode = XPROC_STDIO_PIPE;
    tConfig.Stdout.bCapture = true;
    tConfig.Stderr.iMode = bMerge ? XPROC_STDIO_INHERIT : XPROC_STDIO_PIPE;
    tConfig.Stderr.bCapture = !bMerge;
    tConfig.Stdin.iMode = XPROC_STDIO_NULL;
    if ( !xrtExecCapture(&tConfig, &tProcess, (uint32_t)uTimeout) ) {
        eResult = xwork__tool_fail(pOutput, "failed to start or capture command"); goto cleanup;
    }
    if ( !xwork__buf_appendf(&tOutput, "$ %s\nexit_code: %d\nduration_ms: %llu%s\n",
            sCommand,
            tProcess.iExitCode,
            (unsigned long long)tProcess.iDurationMs,
            tProcess.ExitInfo.bTimedOut ? " (timed out)" : "") ) goto oom;
    if ( tProcess.iStdoutSize ) {
        if ( !xwork__buf_append_cstr(&tOutput, "--- stdout ---\n") ||
             !xwork__buf_append_process_text(&tOutput, tProcess.pStdout, tProcess.iStdoutSize) ||
             !xwork__buf_append_char(&tOutput, '\n') ) goto oom;
    }
    if ( tProcess.iStderrSize ) {
        if ( !xwork__buf_append_cstr(&tOutput, "--- stderr ---\n") ||
             !xwork__buf_append_process_text(&tOutput, tProcess.pStderr, tProcess.iStderrSize) ||
             !xwork__buf_append_char(&tOutput, '\n') ) goto oom;
    }
    if ( tProcess.bStdoutTruncated || tProcess.bStderrTruncated ) {
        if ( !xwork__buf_append_cstr(&tOutput, "[process capture was truncated by the configured capture limit]\n") ) goto oom;
    }
    if ( !xworkToolOutputSet(pOutput, xrtProcessResultSuccess(&tProcess), tOutput.pData ? tOutput.pData : "") ) goto oom;
    eResult = XWORK_RESULT_OK;
    goto cleanup;
oom:
    xwork__set_error(pError, XWORK_ERROR_OUT_OF_MEMORY, "failed to build exec_command output");
cleanup:
    if ( tArgs ) xvoUnref(tArgs);
    free(sResolvedCwd);
    xrtProcessResultUnit(&tProcess);
    xwork__buf_unit(&tOutput);
    return eResult;
}

bool xworkAgentRegisterBuiltinTools(xwork_agent* pAgent, xwork_error* pError)
{
    static const xwork_tool_definition arrTools[] = {
        {
            "read_file",
            "Read a UTF-8 text file from the workspace with stable line numbers. Use start_line to continue large files.",
            "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"start_line\":{\"type\":\"integer\",\"minimum\":1},\"max_lines\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":10000}},\"required\":[\"path\"],\"additionalProperties\":false}",
            true, XWORK_TOOL_EFFECT_READ_ONLY, xwork__tool_read_file, NULL
        },
        {
            "list_files",
            "List files and directories within the workspace. Recursive scans skip .git and .xcode.",
            "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"pattern\":{\"type\":\"string\"},\"recursive\":{\"type\":\"boolean\"},\"max_results\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":10000},\"max_depth\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":64}},\"additionalProperties\":false}",
            true, XWORK_TOOL_EFFECT_READ_ONLY, xwork__tool_list_files, NULL
        },
        {
            "search_text",
            "Search literal text in workspace files. Use pattern such as *.c to narrow files; binary and files over 4 MiB are skipped.",
            "{\"type\":\"object\",\"properties\":{\"query\":{\"type\":\"string\"},\"path\":{\"type\":\"string\"},\"pattern\":{\"type\":\"string\"},\"max_results\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":5000},\"max_depth\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":64}},\"required\":[\"query\"],\"additionalProperties\":false}",
            true, XWORK_TOOL_EFFECT_READ_ONLY, xwork__tool_search_text, NULL
        },
        {
            "write_file",
            "Create, overwrite, or append a UTF-8 file inside the workspace. Prefer replace_text for small edits to existing files.",
            "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"content\":{\"type\":\"string\"},\"mode\":{\"type\":\"string\",\"enum\":[\"overwrite\",\"create\",\"append\"]},\"create_dirs\":{\"type\":\"boolean\"}},\"required\":[\"path\",\"content\"],\"additionalProperties\":false}",
            true, XWORK_TOOL_EFFECT_WORKSPACE_WRITE, xwork__tool_write_file, NULL
        },
        {
            "replace_text",
            "Replace an exact text block in one workspace file. By default the old text must occur exactly once; include surrounding context for safe edits.",
            "{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"old_text\":{\"type\":\"string\"},\"new_text\":{\"type\":\"string\"},\"replace_all\":{\"type\":\"boolean\"}},\"required\":[\"path\",\"old_text\",\"new_text\"],\"additionalProperties\":false}",
            true, XWORK_TOOL_EFFECT_WORKSPACE_WRITE, xwork__tool_replace_text, NULL
        },
        {
            "exec_command",
            "Run a non-interactive command in a workspace directory and capture stdout, stderr, exit code, duration, and timeout state.",
            "{\"type\":\"object\",\"properties\":{\"command\":{\"type\":\"string\"},\"cwd\":{\"type\":\"string\"},\"timeout_ms\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":3600000},\"merge_stderr\":{\"type\":\"boolean\"}},\"required\":[\"command\"],\"additionalProperties\":false}",
            true, XWORK_TOOL_EFFECT_PROCESS, xwork__tool_exec_command, NULL
        }
    };
    size_t i;
    xwork_tool_definition tTool;
    if ( !pAgent ) {
        xwork__set_error(pError, XWORK_ERROR_INVALID_ARGUMENT, "agent is null");
        return false;
    }
    for ( i = 0u; i < sizeof(arrTools) / sizeof(arrTools[0]); ++i ) {
        tTool = arrTools[i];
        tTool.pUserData = pAgent;
        if ( !xworkAgentRegisterTool(pAgent, &tTool, pError) ) return false;
    }
    return true;
}
