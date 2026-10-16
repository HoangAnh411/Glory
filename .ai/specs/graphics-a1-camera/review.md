# A1 Camera Review

## Must Fix
- None found in the reviewed A1 diff.

## Should Fix
- Camera obstruction bounds are conservative spheres around static props because the current asset pipeline does not expose camera collision meshes. Replace these bounds with authored collision volumes when those become available.

## Suggestions
- Capture a gameplay comparison with the player and boss both visible, and verify shoulder placement near representative arena props on the target display aspect ratios.

## Review Result
- Scope matches the plan: camera-only behavior, F3 comparison, legacy F1 free-camera control, and no changes to gameplay collision, hitboxes, or combo logic.
