# Profile Selection

> Status: English draft, pending review.

Profiles provide reusable defaults for product families. They are integration defaults, not hard security boundaries.

## Built-in Profiles

| Constant | Profile | Typical Product | Default Direction |
| --- | --- | --- |
| `XWORK_PROFILE_XCODE` | `xcode` | AI IDE | More conservative approvals and IDE-facing artifacts. |
| `XWORK_PROFILE_XCLAW` | `xclaw` | Autonomous CLI agent | Broader local tool capability and autonomous flow. |

## Minimal Use

```c
xwork_profile tProfile;
xwork_runtime_options tRuntime;
xwork_run_options tRun;

xwork_profile_init(&tProfile);
xwork_runtime_options_init(&tRuntime);
xwork_run_options_init(&tRun);

if (xwork_profile_get_builtin(XWORK_PROFILE_XCLAW, &tProfile) == XWORK_OK) {
    xwork_profile_apply_runtime_options(&tProfile, &tRuntime);
    xwork_profile_apply_run_options(&tProfile, &tRun);
}
```

## Next

- [Profiles API](../api/api-profiles.en.md)
- [Policy / Approval API](../api/api-policy-approval.en.md)
