#include "../xwork_core/xwork_internal.h"

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
