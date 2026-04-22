# Docs Review Checklist

> Status: English draft, pending review.

Use this checklist when reviewing xwork documentation changes.

## Links and Structure

- Root README links to the docs center, API, guides, and examples.
- Relative links under `docs/` are valid.
- Formal docs may link to `dev/docs/` as internal references, but user-facing facts must be understandable from formal docs alone.
- New docs are added to the relevant index.
- English `.en.md` files keep the same structure as the Chinese source where practical.

## API Consistency

- Type names, function names, enum names, error codes, and fields match current `xwork.h`.
- API pages describe ownership, lifetime, reset/destroy rules, and thread boundaries.
- Persistence docs match `XWORK_PERSISTENCE_FORMAT_VERSION`.
- Remote-worker docs match `XWORK_REMOTE_PROTOCOL_VERSION_CURRENT`.
- Schema names and built-in tool names match constants in `xwork.h`.

## Examples and Snippets

- Complete runnable code lives in `examples/`.
- Build commands are locally or CI validated.
- Pseudocode uses `text`, not `c`.
- New examples update `examples/README.md` and the relevant guide or case page.

## Local Check

```powershell
powershell -ExecutionPolicy Bypass -File tools\check_docs.ps1
```
