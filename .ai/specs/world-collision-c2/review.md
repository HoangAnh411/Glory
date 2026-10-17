# C2 Review

## Must Fix
- None found for the C2 acceptance criteria.

## Should Fix
- AABB contacts are resolved with radius-bounded substeps and positional separation. Very large externally supplied displacements can require many iterations; this intentionally avoids a fixed cap that could permit tunneling.

## Suggestions
- Add a performance benchmark for extreme knockback distances if gameplay later allows large teleports to enter the movement solver.
- Add rotated OBB collision support only if the authored world AABBs prove too conservative in playtesting.

## Result
- Arena enforcement and static obstacle collision execute in pure game code, with no renderer dependency.
- Run, jump, dodge, and knockback still use the same controller/combat state transitions; hitbox and combo code was not changed.
- Build, focused collision test, CTest 37/37, and 1080p smoke passed.
