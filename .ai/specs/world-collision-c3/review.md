# C3 Review

## 1. Must Fix
- None found in the reviewed C3 changes.

## 2. Recommended
- Authored camera collision uses world-space AABBs. Rotated props may have a broader box than an oriented box, though small props are excluded by the requested size threshold.
- The player capsule dimensions used for separation follow the current controller defaults; future configurable capsule data should feed this call directly.

## 3. Suggestions
- If encounter content adds moving props or multiple non-boss enemies, provide their runtime colliders through the same game collision representation and actor separation path.

## Scope review
- Boss world movement/spawn depenetration uses `CollisionWorld`.
- Player/boss separation changes actor positions only; attack hitbox and combo definitions are untouched.
- Camera collision now uses the loaded world AABBs and configured arena center/radius, ignoring bounds below 1.2 m.
- Build, all 37 CTest cases, and 1080p smoke passed.
