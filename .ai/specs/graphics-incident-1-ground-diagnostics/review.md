# Review: ground diagnostics

## Must fix
- None found in code review. The normal render path keeps debug view 0 (Lit).

## Recommended
- Capture albedo, normal, ambient, direct, shadow, fog, depth, and invalid-value views in the interactive client to establish which buffer first becomes black.

## Suggestions
- Add automated pixel readback assertions when a deterministic headless GL image test exists.

## Result
- Build, CTest, and 1080p smoke pass. Visual root cause is not claimed without an image from the diagnostic views.
