#include "../xwork_core/xwork_internal.h"

static xwork_risk_level xwork__policy_risk_level(xwork_tool_side_effect eSideEffect)
{
    switch ( eSideEffect ) {
        case XWORK_SIDE_EFFECT_READ_ONLY:
            return XWORK_RISK_LOW;
        case XWORK_SIDE_EFFECT_WORKSPACE_WRITE:
            return XWORK_RISK_MEDIUM;
        case XWORK_SIDE_EFFECT_PROCESS_EXEC:
        case XWORK_SIDE_EFFECT_NETWORK_ACCESS:
            return XWORK_RISK_HIGH;
        case XWORK_SIDE_EFFECT_EXTERNAL_MUTATION:
            return XWORK_RISK_CRITICAL;
        default:
            return XWORK_RISK_MEDIUM;
    }
}

static const char *xwork__policy_scope(xwork_tool_side_effect eSideEffect)
{
    switch ( eSideEffect ) {
        case XWORK_SIDE_EFFECT_READ_ONLY:
            return "read_only";
        case XWORK_SIDE_EFFECT_WORKSPACE_WRITE:
            return "workspace_write";
        case XWORK_SIDE_EFFECT_PROCESS_EXEC:
            return "process_exec";
        case XWORK_SIDE_EFFECT_NETWORK_ACCESS:
            return "network_access";
        case XWORK_SIDE_EFFECT_EXTERNAL_MUTATION:
            return "external_mutation";
        default:
            return "unknown_scope";
    }
}

void xwork_policy_options_init(xwork_policy_options *pOptions)
{
    if ( pOptions ) {
        memset(pOptions, 0, sizeof(*pOptions));
        pOptions->eAutoApproveRiskLimit = XWORK_RISK_CRITICAL;
    }
}

void xwork_approval_eval_input_init(xwork_approval_eval_input *pInput)
{
    if ( pInput ) {
        memset(pInput, 0, sizeof(*pInput));
        pInput->eAutonomy = XWORK_AUTONOMY_SEMI_AUTO;
        pInput->eApprovalMode = XWORK_APPROVAL_DEFAULT;
        pInput->eSideEffect = XWORK_SIDE_EFFECT_READ_ONLY;
    }
}

void xwork_approval_decision_init(xwork_approval_decision *pDecision)
{
    if ( pDecision ) {
        memset(pDecision, 0, sizeof(*pDecision));
        pDecision->eRiskLevel = XWORK_RISK_LOW;
        pDecision->sScope = "read_only";
    }
}

xwork_status xwork_policy_evaluate_approval(
    const xwork_policy_options *pPolicy,
    const xwork_approval_eval_input *pInput,
    xwork_approval_decision *pDecision
)
{
    xwork_policy_options tDefaultPolicy;

    if ( !pInput || !pDecision ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( !pPolicy ) {
        xwork_policy_options_init(&tDefaultPolicy);
        pPolicy = &tDefaultPolicy;
    }

    xwork_approval_decision_init(pDecision);
    pDecision->eRiskLevel = xwork__policy_risk_level(pInput->eSideEffect);
    pDecision->sScope = xwork__policy_scope(pInput->eSideEffect);

    switch ( pInput->eApprovalMode ) {
        case XWORK_APPROVAL_NEVER:
            pDecision->bRequiresApproval = false;
            pDecision->sReason = "Approval is disabled for this tool.";
            break;
        case XWORK_APPROVAL_ALWAYS:
            pDecision->bRequiresApproval = true;
            pDecision->sReason = "Tool approval mode is set to always.";
            break;
        case XWORK_APPROVAL_ON_DEMAND:
            pDecision->bRequiresApproval = true;
            pDecision->sReason = "Tool approval mode is set to on_demand.";
            break;
        case XWORK_APPROVAL_DEFAULT:
        default:
            if ( pInput->eSideEffect == XWORK_SIDE_EFFECT_READ_ONLY ) {
                pDecision->bRequiresApproval = false;
                pDecision->sReason = "Read-only tools do not require approval.";
            } else if ( pInput->eAutonomy == XWORK_AUTONOMY_AUTO ) {
                pDecision->bRequiresApproval = false;
                pDecision->sReason = "Auto autonomy allows this tool without approval.";
            } else {
                pDecision->bRequiresApproval = true;
                pDecision->sReason = "Current autonomy requires approval for side-effecting tools.";
            }
            break;
    }

    pDecision->bAutoApproved =
        pDecision->bRequiresApproval &&
        pInput->bAutoApproveRequested &&
        pDecision->eRiskLevel <= pPolicy->eAutoApproveRiskLimit;

    return XWORK_OK;
}
