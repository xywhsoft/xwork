# Provider Smoke

> Status: English draft, pending review.

Provider smoke tests verify that xwork can execute an orchestrator turn with either offline stub behavior or a real configured model provider.

## Offline Smoke

Offline smoke should be deterministic and safe for CI. It must not require secrets or real network access.

## Real Provider Smoke

Real provider smoke should be opt-in through environment variables or CI variables/secrets. It may fail because of provider/network issues and should not block ordinary CI unless explicitly enabled.

## Next

- [xllm Integration API](../api/api-xllm-integration.en.md)
- [Orchestrator API](../api/api-orchestrator.en.md)
