#include "../xwork_core/xwork_internal.h"

static const xwork_profile xwork__builtin_xcode_profile = {
    XWORK_PROFILE_XCODE,
    "xcode interactive",
    "Interactive coding profile for user-driven runs.",
    NULL,
    NULL,
    XWORK_AUTONOMY_SEMI_AUTO,
    { XWORK_RISK_LOW },
    { true, 0.75, 8u },
    8u,
    false,
    false
};

static const xwork_profile xwork__builtin_xclaw_profile = {
    XWORK_PROFILE_XCLAW,
    "xclaw autonomous",
    "Autonomous task profile for longer-running background work.",
    NULL,
    NULL,
    XWORK_AUTONOMY_AUTO,
    { XWORK_RISK_HIGH },
    { true, 0.90, 24u },
    32u,
    true,
    true
};

static const xwork_profile *xwork__find_builtin_profile(const char *sProfileId)
{
    if ( !sProfileId || !sProfileId[0] ) {
        return NULL;
    }
    if ( strcmp(sProfileId, XWORK_PROFILE_XCODE) == 0 ) {
        return &xwork__builtin_xcode_profile;
    }
    if ( strcmp(sProfileId, XWORK_PROFILE_XCLAW) == 0 ) {
        return &xwork__builtin_xclaw_profile;
    }
    return NULL;
}

void xwork_profile_init(xwork_profile *pProfile)
{
    if ( pProfile ) {
        memset(pProfile, 0, sizeof(*pProfile));
        pProfile->eAutonomy = XWORK_AUTONOMY_SEMI_AUTO;
        xwork_policy_options_init(&pProfile->tPolicy);
        xwork_session_policy_init(&pProfile->tSessionPolicy);
        pProfile->iDefaultMaxTurns = 4u;
        pProfile->bDefaultAutoApprove = true;
    }
}

xwork_status xwork_profile_get_builtin(const char *sProfileId, xwork_profile *pProfile)
{
    const xwork_profile *pBuiltin;

    if ( !pProfile ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    xwork_profile_init(pProfile);
    pBuiltin = xwork__find_builtin_profile(sProfileId);
    if ( !pBuiltin ) {
        return XWORK_ERROR_NOT_FOUND;
    }

    *pProfile = *pBuiltin;
    return XWORK_OK;
}

xwork_status xwork_profile_apply_runtime_options(
    const xwork_profile *pProfile,
    xwork_runtime_options *pOptions
)
{
    if ( !pProfile || !pOptions ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    pOptions->tPolicy = pProfile->tPolicy;
    return XWORK_OK;
}

xwork_status xwork_profile_apply_workspace_options(
    const xwork_profile *pProfile,
    xwork_workspace_options *pOptions
)
{
    if ( !pProfile || !pOptions ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    pOptions->bEnableMemory = pProfile->bEnableWorkspaceMemory;
    return XWORK_OK;
}

xwork_status xwork_profile_apply_run_options(
    const xwork_profile *pProfile,
    xwork_run_options *pOptions
)
{
    if ( !pProfile || !pOptions ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( (!pOptions->sLlmProfileId || !pOptions->sLlmProfileId[0]) &&
         pProfile->sDefaultLlmProfileId && pProfile->sDefaultLlmProfileId[0] ) {
        pOptions->sLlmProfileId = pProfile->sDefaultLlmProfileId;
    }
    if ( (!pOptions->sSessionProfileId || !pOptions->sSessionProfileId[0]) &&
         pProfile->sDefaultSessionProfileId && pProfile->sDefaultSessionProfileId[0] ) {
        pOptions->sSessionProfileId = pProfile->sDefaultSessionProfileId;
    }
    pOptions->eAutonomy = pProfile->eAutonomy;
    pOptions->tSessionPolicy = pProfile->tSessionPolicy;
    return XWORK_OK;
}

xwork_status xwork_profile_apply_orchestrator_options(
    const xwork_profile *pProfile,
    xwork_orchestrator_options *pOptions
)
{
    if ( !pProfile || !pOptions ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( pProfile->iDefaultMaxTurns > 0u ) {
        pOptions->iMaxTurns = pProfile->iDefaultMaxTurns;
    }
    pOptions->bAutoApprove = pProfile->bDefaultAutoApprove;
    return XWORK_OK;
}
