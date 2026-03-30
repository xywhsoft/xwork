#ifndef XWORK_INTERNAL_H
#define XWORK_INTERNAL_H

#include "../../xwork.h"

#include <stdlib.h>
#include <string.h>

typedef struct xwork_tool_record xwork_tool_record;

struct xwork_runtime {
    xllm_runtime *pLlmRuntime;
    void *pUserData;
    xwork_workspace *pWorkspaces;
    xwork_tool_record *pTools;
    xwork_run *pRuns;
};

struct xwork_workspace {
    xwork_runtime *pRuntime;
    char *sWorkspaceId;
    char *sRootPath;
    bool bEnableMemory;
    xllm_memory *pMemory;
    xwork_workspace *pNext;
};

struct xwork_tool_record {
    xwork_tool_def tDef;
    char *sToolId;
    char *sDisplayName;
    char *sDescription;
    xwork_tool_record *pNext;
};

struct xwork_run {
    xwork_runtime *pRuntime;
    char *sRunId;
    char *sParentRunId;
    char *sInstruction;
    char *sLlmProfileId;
    char *sSessionProfileId;
    char **psWorkspaceIds;
    size_t iWorkspaceCount;
    xwork_autonomy_mode eAutonomy;
    xwork_run_state eState;
    xwork_run *pNext;
};

char *xwork__dup_cstr(const char *sText);
void xwork__free_cstr(char **psText);
void xwork__free_str_array(char ***ppsItems, size_t *piItemCount);
bool xwork__runtime_has_workspace(const xwork_runtime *pRuntime, const char *sWorkspaceId);
bool xwork__run_state_is_terminal(xwork_run_state eState);

#endif
