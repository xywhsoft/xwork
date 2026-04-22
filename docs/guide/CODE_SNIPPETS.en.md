# Code Snippet Maintenance

> Status: English draft, pending review.

This page defines how guide snippets are maintained so docs do not drift from `xwork.h` and `examples/`.

## Snippet Types

| Type | Marker | Rule |
| --- | --- | --- |
| Complete program | Link to `examples/*.c` and provide a build command | Must be validated by examples smoke or CI. |
| C snippet | Fenced `c` block | Must use real types, functions, and fields from `xwork.h`. |
| Call sequence | Fenced `text` block | Expresses lifecycle order only; not claimed as compilable C. |
| JSON contract | Fenced `json` block | Must match host tool contracts or implementation behavior. |

## Source of Truth

- [`xwork.h`](../../xwork.h)
- [`examples/README.md`](../../examples/README.md)
- [Host Tools API](../api/api-host-tools.en.md)
- [Docs review checklist](../DOCS_REVIEW_CHECKLIST.en.md)

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools\check_docs.ps1
```
