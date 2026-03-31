#include "../xwork_core/xwork_internal.h"

static void xwork__artifact_snapshot_free_cstr(const char **psText)
{
    if ( psText && *psText ) {
        free((void *)*psText);
        *psText = NULL;
    }
}

static xwork_status xwork__artifact_snapshot_replace_cstr(
    const char **psTarget,
    const char *sText
)
{
    char *sCopy = NULL;

    if ( !psTarget ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    if ( sText ) {
        sCopy = xwork__dup_cstr(sText);
        if ( !sCopy ) {
            return XWORK_ERROR_NO_MEMORY;
        }
    }

    xwork__artifact_snapshot_free_cstr(psTarget);
    *psTarget = sCopy;
    return XWORK_OK;
}

static xwork_status xwork__artifact_copy(
    xwork_artifact *pTarget,
    const xwork_artifact *pSource
)
{
    xwork_status iStatus;

    if ( !pTarget || !pSource ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    xwork_artifact_reset(pTarget);
    iStatus = xwork__artifact_snapshot_replace_cstr(&pTarget->sArtifactId, pSource->sArtifactId);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    iStatus = xwork__artifact_snapshot_replace_cstr(&pTarget->sRunId, pSource->sRunId);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    iStatus = xwork__artifact_snapshot_replace_cstr(&pTarget->sName, pSource->sName);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    iStatus = xwork__artifact_snapshot_replace_cstr(&pTarget->sMimeType, pSource->sMimeType);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    iStatus = xwork__artifact_snapshot_replace_cstr(&pTarget->sStorageRef, pSource->sStorageRef);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }
    iStatus = xwork__artifact_snapshot_replace_cstr(&pTarget->sSummary, pSource->sSummary);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    pTarget->eKind = pSource->eKind;
    pTarget->iSequence = pSource->iSequence;
    return XWORK_OK;
}

static void xwork__artifact_record_reset(xwork_artifact_record *pRecord)
{
    if ( !pRecord ) {
        return;
    }

    xwork__free_cstr(&pRecord->sArtifactId);
    xwork__free_cstr(&pRecord->sName);
    xwork__free_cstr(&pRecord->sMimeType);
    xwork__free_cstr(&pRecord->sStorageRef);
    xwork__free_cstr(&pRecord->sSummary);
    memset(&pRecord->tArtifact, 0, sizeof(pRecord->tArtifact));
}

static xwork_status xwork__ensure_artifact_capacity(xwork_run *pRun)
{
    xwork_artifact_record *pNewItems;
    size_t iNewCapacity;

    if ( !pRun ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( pRun->iArtifactCount < pRun->iArtifactCapacity ) {
        return XWORK_OK;
    }

    iNewCapacity = pRun->iArtifactCapacity ? pRun->iArtifactCapacity * 2u : 4u;
    pNewItems = (xwork_artifact_record *)realloc(
        pRun->pArtifactLog,
        iNewCapacity * sizeof(*pNewItems)
    );
    if ( !pNewItems ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    memset(
        pNewItems + pRun->iArtifactCapacity,
        0,
        (iNewCapacity - pRun->iArtifactCapacity) * sizeof(*pNewItems)
    );
    pRun->pArtifactLog = pNewItems;
    pRun->iArtifactCapacity = iNewCapacity;
    return XWORK_OK;
}

static xwork_status xwork__append_artifact_record_owned(
    xwork_run *pRun,
    char *sArtifactId,
    xwork_artifact_kind eKind,
    char *sName,
    char *sMimeType,
    char *sStorageRef,
    char *sSummary,
    size_t iSequence,
    xwork_artifact *pArtifact
)
{
    xwork_artifact_record *pRecord;
    xwork_status iStatus;

    if ( !pRun || !sArtifactId || !sArtifactId[0] ) {
        free(sArtifactId);
        free(sName);
        free(sMimeType);
        free(sStorageRef);
        free(sSummary);
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iStatus = xwork__ensure_artifact_capacity(pRun);
    if ( iStatus != XWORK_OK ) {
        free(sArtifactId);
        free(sName);
        free(sMimeType);
        free(sStorageRef);
        free(sSummary);
        return iStatus;
    }

    pRecord = &pRun->pArtifactLog[pRun->iArtifactCount];
    memset(pRecord, 0, sizeof(*pRecord));

    pRecord->sArtifactId = sArtifactId;
    pRecord->sName = sName;
    pRecord->sMimeType = sMimeType;
    pRecord->sStorageRef = sStorageRef;
    pRecord->sSummary = sSummary;
    pRecord->tArtifact.sArtifactId = pRecord->sArtifactId;
    pRecord->tArtifact.sRunId = pRun->sRunId;
    pRecord->tArtifact.eKind = eKind;
    pRecord->tArtifact.sName = pRecord->sName;
    pRecord->tArtifact.sMimeType = pRecord->sMimeType;
    pRecord->tArtifact.sStorageRef = pRecord->sStorageRef;
    pRecord->tArtifact.sSummary = pRecord->sSummary;
    pRecord->tArtifact.iSequence = iSequence;

    ++pRun->iArtifactCount;
    if ( iSequence > pRun->iNextArtifactSequence ) {
        pRun->iNextArtifactSequence = iSequence;
    }

    if ( pArtifact ) {
        *pArtifact = pRecord->tArtifact;
    }

    return XWORK_OK;
}

void xwork_artifact_options_init(xwork_artifact_options *pOptions)
{
    if ( pOptions ) {
        memset(pOptions, 0, sizeof(*pOptions));
        pOptions->eKind = XWORK_ARTIFACT_OUTPUT;
    }
}

void xwork_artifact_init(xwork_artifact *pArtifact)
{
    if ( pArtifact ) {
        memset(pArtifact, 0, sizeof(*pArtifact));
        pArtifact->eKind = XWORK_ARTIFACT_OUTPUT;
    }
}

void xwork_artifact_reset(xwork_artifact *pArtifact)
{
    if ( !pArtifact ) {
        return;
    }

    xwork__free_cstr((char **)&pArtifact->sArtifactId);
    xwork__free_cstr((char **)&pArtifact->sRunId);
    xwork__free_cstr((char **)&pArtifact->sName);
    xwork__free_cstr((char **)&pArtifact->sMimeType);
    xwork__free_cstr((char **)&pArtifact->sStorageRef);
    xwork__free_cstr((char **)&pArtifact->sSummary);
    xwork_artifact_init(pArtifact);
}

void xwork__run_reset_artifacts(xwork_run *pRun)
{
    size_t i;

    if ( !pRun ) {
        return;
    }

    for ( i = 0u; i < pRun->iArtifactCount; ++i ) {
        xwork__artifact_record_reset(&pRun->pArtifactLog[i]);
    }
    free(pRun->pArtifactLog);
    pRun->pArtifactLog = NULL;
    pRun->iArtifactCount = 0u;
    pRun->iArtifactCapacity = 0u;
    pRun->iNextArtifactSequence = 0u;
}

xwork_status xwork__run_append_artifact_record(
    xwork_run *pRun,
    const xwork_artifact_options *pOptions,
    xwork_artifact *pArtifact
)
{
    char *sArtifactId = NULL;
    char *sName = NULL;
    char *sMimeType = NULL;
    char *sStorageRef = NULL;
    char *sSummary = NULL;
    size_t i;
    size_t iSequence;

    if ( !pRun || !pOptions ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    iSequence = pRun->iNextArtifactSequence + 1u;

    if ( pOptions->sArtifactId && pOptions->sArtifactId[0] ) {
        sArtifactId = xwork__dup_cstr(pOptions->sArtifactId);
    } else {
        sArtifactId = xwork__dup_scoped_id(pRun->sRunId, "artifact", iSequence);
    }
    if ( !sArtifactId ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    for ( i = 0u; i < pRun->iArtifactCount; ++i ) {
        if ( pRun->pArtifactLog[i].tArtifact.sArtifactId &&
             strcmp(pRun->pArtifactLog[i].tArtifact.sArtifactId, sArtifactId) == 0 ) {
            free(sArtifactId);
            return XWORK_ERROR_ALREADY_EXISTS;
        }
    }

    if ( pOptions->sName ) {
        sName = xwork__dup_cstr(pOptions->sName);
        if ( !sName ) {
            free(sArtifactId);
            return XWORK_ERROR_NO_MEMORY;
        }
    }
    if ( pOptions->sMimeType ) {
        sMimeType = xwork__dup_cstr(pOptions->sMimeType);
        if ( !sMimeType ) {
            free(sName);
            free(sArtifactId);
            return XWORK_ERROR_NO_MEMORY;
        }
    }
    if ( pOptions->sStorageRef ) {
        sStorageRef = xwork__dup_cstr(pOptions->sStorageRef);
        if ( !sStorageRef ) {
            free(sMimeType);
            free(sName);
            free(sArtifactId);
            return XWORK_ERROR_NO_MEMORY;
        }
    }
    if ( pOptions->sSummary ) {
        sSummary = xwork__dup_cstr(pOptions->sSummary);
        if ( !sSummary ) {
            free(sStorageRef);
            free(sMimeType);
            free(sName);
            free(sArtifactId);
            return XWORK_ERROR_NO_MEMORY;
        }
    }

    return xwork__append_artifact_record_owned(
        pRun,
        sArtifactId,
        pOptions->eKind,
        sName,
        sMimeType,
        sStorageRef,
        sSummary,
        iSequence,
        pArtifact
    );
}

void xwork__run_snapshot_reset_artifacts(xwork_run_snapshot *pSnapshot)
{
    size_t i;
    xwork_artifact *pArtifacts;

    if ( !pSnapshot || !pSnapshot->pArtifacts ) {
        return;
    }

    pArtifacts = (xwork_artifact *)pSnapshot->pArtifacts;
    for ( i = 0u; i < pSnapshot->iArtifactCount; ++i ) {
        xwork__artifact_snapshot_free_cstr(&pArtifacts[i].sArtifactId);
        if ( pArtifacts[i].sRunId != pSnapshot->sRunId ) {
            xwork__artifact_snapshot_free_cstr(&pArtifacts[i].sRunId);
        }
        xwork__artifact_snapshot_free_cstr(&pArtifacts[i].sName);
        xwork__artifact_snapshot_free_cstr(&pArtifacts[i].sMimeType);
        xwork__artifact_snapshot_free_cstr(&pArtifacts[i].sStorageRef);
        xwork__artifact_snapshot_free_cstr(&pArtifacts[i].sSummary);
    }
    free(pArtifacts);
    pSnapshot->pArtifacts = NULL;
    pSnapshot->iArtifactCount = 0u;
}

xwork_status xwork__run_snapshot_copy_artifacts(
    xwork_run_snapshot *pSnapshot,
    const xwork_run *pRun
)
{
    xwork_artifact *pArtifacts;
    size_t i;

    if ( !pSnapshot || !pRun ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    xwork__run_snapshot_reset_artifacts(pSnapshot);

    if ( pRun->iArtifactCount == 0u ) {
        return XWORK_OK;
    }

    pArtifacts = (xwork_artifact *)calloc(pRun->iArtifactCount, sizeof(*pArtifacts));
    if ( !pArtifacts ) {
        return XWORK_ERROR_NO_MEMORY;
    }

    for ( i = 0u; i < pRun->iArtifactCount; ++i ) {
        xwork_status iStatus;
        const xwork_artifact *pSource = &pRun->pArtifactLog[i].tArtifact;

        xwork_artifact_init(&pArtifacts[i]);
        iStatus = xwork__artifact_snapshot_replace_cstr(&pArtifacts[i].sArtifactId, pSource->sArtifactId);
        if ( iStatus != XWORK_OK ) {
            pSnapshot->pArtifacts = pArtifacts;
            pSnapshot->iArtifactCount = i + 1u;
            xwork__run_snapshot_reset_artifacts(pSnapshot);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(&pArtifacts[i].sName, pSource->sName);
        if ( iStatus != XWORK_OK ) {
            pSnapshot->pArtifacts = pArtifacts;
            pSnapshot->iArtifactCount = i + 1u;
            xwork__run_snapshot_reset_artifacts(pSnapshot);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(&pArtifacts[i].sMimeType, pSource->sMimeType);
        if ( iStatus != XWORK_OK ) {
            pSnapshot->pArtifacts = pArtifacts;
            pSnapshot->iArtifactCount = i + 1u;
            xwork__run_snapshot_reset_artifacts(pSnapshot);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(&pArtifacts[i].sStorageRef, pSource->sStorageRef);
        if ( iStatus != XWORK_OK ) {
            pSnapshot->pArtifacts = pArtifacts;
            pSnapshot->iArtifactCount = i + 1u;
            xwork__run_snapshot_reset_artifacts(pSnapshot);
            return iStatus;
        }
        iStatus = xwork__artifact_snapshot_replace_cstr(&pArtifacts[i].sSummary, pSource->sSummary);
        if ( iStatus != XWORK_OK ) {
            pSnapshot->pArtifacts = pArtifacts;
            pSnapshot->iArtifactCount = i + 1u;
            xwork__run_snapshot_reset_artifacts(pSnapshot);
            return iStatus;
        }

        pArtifacts[i].sRunId = pSnapshot->sRunId;
        pArtifacts[i].eKind = pSource->eKind;
        pArtifacts[i].iSequence = pSource->iSequence;
    }

    pSnapshot->pArtifacts = pArtifacts;
    pSnapshot->iArtifactCount = pRun->iArtifactCount;
    return XWORK_OK;
}

xwork_status xwork__run_apply_snapshot_artifacts(
    xwork_run *pRun,
    const xwork_run_snapshot *pSnapshot
)
{
    size_t i;

    if ( !pRun || !pSnapshot ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    xwork__run_reset_artifacts(pRun);

    for ( i = 0u; i < pSnapshot->iArtifactCount; ++i ) {
        const xwork_artifact *pSource = &pSnapshot->pArtifacts[i];
        xwork_status iStatus;
        char *sArtifactId = xwork__dup_cstr(pSource->sArtifactId);
        char *sName = xwork__dup_cstr(pSource->sName);
        char *sMimeType = xwork__dup_cstr(pSource->sMimeType);
        char *sStorageRef = xwork__dup_cstr(pSource->sStorageRef);
        char *sSummary = xwork__dup_cstr(pSource->sSummary);

        if ( (pSource->sArtifactId && !sArtifactId) ||
             (pSource->sName && !sName) ||
             (pSource->sMimeType && !sMimeType) ||
             (pSource->sStorageRef && !sStorageRef) ||
             (pSource->sSummary && !sSummary) ) {
            free(sSummary);
            free(sStorageRef);
            free(sMimeType);
            free(sName);
            free(sArtifactId);
            xwork__run_reset_artifacts(pRun);
            return XWORK_ERROR_NO_MEMORY;
        }

        iStatus = xwork__append_artifact_record_owned(
            pRun,
            sArtifactId,
            pSource->eKind,
            sName,
            sMimeType,
            sStorageRef,
            sSummary,
            pSource->iSequence,
            NULL
        );
        if ( iStatus != XWORK_OK ) {
            xwork__run_reset_artifacts(pRun);
            return iStatus;
        }
    }

    if ( pSnapshot->iNextArtifactSequence > pRun->iNextArtifactSequence ) {
        pRun->iNextArtifactSequence = pSnapshot->iNextArtifactSequence;
    }

    return XWORK_OK;
}

char *xwork__run_build_artifact_refs(const xwork_run *pRun)
{
    char *sRefs;
    size_t i;
    size_t iLen = 0u;

    if ( !pRun || pRun->iArtifactCount == 0u ) {
        return NULL;
    }

    for ( i = 0u; i < pRun->iArtifactCount; ++i ) {
        const char *sArtifactId = pRun->pArtifactLog[i].tArtifact.sArtifactId;

        if ( !sArtifactId || !sArtifactId[0] ) {
            continue;
        }
        iLen += strlen(sArtifactId);
        if ( i > 0u ) {
            ++iLen;
        }
    }

    if ( iLen == 0u ) {
        return NULL;
    }

    sRefs = (char *)calloc(iLen + 1u, sizeof(char));
    if ( !sRefs ) {
        return NULL;
    }

    for ( i = 0u; i < pRun->iArtifactCount; ++i ) {
        const char *sArtifactId = pRun->pArtifactLog[i].tArtifact.sArtifactId;

        if ( !sArtifactId || !sArtifactId[0] ) {
            continue;
        }
        if ( sRefs[0] ) {
            strcat(sRefs, ",");
        }
        strcat(sRefs, sArtifactId);
    }

    return sRefs;
}

xwork_status xwork_run_emit_artifact(
    xwork_run *pRun,
    const xwork_artifact_options *pOptions,
    xwork_artifact *pArtifact
)
{
    xwork_artifact tArtifact;
    xwork_status iStatus;

    if ( !pRun || !pOptions ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }

    xwork_artifact_init(&tArtifact);
    iStatus = xwork__run_append_artifact_record(pRun, pOptions, &tArtifact);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    iStatus = xwork__runtime_store_artifact(pRun->pRuntime, &tArtifact);
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    iStatus = xwork__run_record_event(
        pRun,
        XWORK_EVENT_ARTIFACT_EMITTED,
        pRun->sLastToolId,
        pRun->sLastApprovalRequestId,
        pRun->sLastCheckpointId,
        tArtifact.sSummary ? tArtifact.sSummary : "Artifact emitted."
    );
    if ( iStatus != XWORK_OK ) {
        return iStatus;
    }

    if ( pArtifact ) {
        iStatus = xwork__artifact_copy(pArtifact, &tArtifact);
        if ( iStatus != XWORK_OK ) {
            return iStatus;
        }
    }
    return XWORK_OK;
}

size_t xwork_run_get_artifact_count(const xwork_run *pRun)
{
    return pRun ? pRun->iArtifactCount : 0u;
}

xwork_status xwork_run_get_artifact(
    const xwork_run *pRun,
    size_t iIndex,
    xwork_artifact *pArtifact
)
{
    if ( !pRun || !pArtifact ) {
        return XWORK_ERROR_INVALID_ARGUMENT;
    }
    if ( iIndex >= pRun->iArtifactCount ) {
        return XWORK_ERROR_NOT_FOUND;
    }

    return xwork__artifact_copy(pArtifact, &pRun->pArtifactLog[iIndex].tArtifact);
}
