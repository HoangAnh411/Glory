# Review: World Collision Wall and Opening Corrections

## 1. Must Fix

- None found in the reviewed change set.

## 2. Recommended

- The cliff wall sections now match the full bounds of the generated rectangular rock mesh. If the rock art changes to irregular geometry, author compound parts or oriented colliders rather than shrinking the current collider away from visible solids.
- Keep the gate lantern collision footprints visible in the F3 collider view; they define part of the usable opening width.

## 3. Suggestions

- Add a dedicated runtime route through the narrowest intended walkable lane after the final world art/layout is fixed.
- Replace cliff AABBs with compound OBBs or authored mesh-aligned segments if the arena perimeter is redesigned.

## Review Result

- Instance override precedence and compound parts are handled in the game collision loader, separate from rendering and combat logic.
- The gate opening has player and boss traversal coverage; both gate supports and all six near-arena wall sections have blocker coverage. The wall tests expand the arena boundary so a safety clamp cannot make a missing wall collider appear to pass.
- The live walk-test exercises the same fixed update callback and loaded collision world as the client rather than only the unit-test solver.
- The circular bound now acts only as an outer recovery guard; tests verify roads remain traversable beyond the old radius 34 and beyond the gate arches.
- Build, CTest, client smoke, bootstrap smoke, and walk-test passed. No gameplay hitbox, combo, or damage code was changed.
