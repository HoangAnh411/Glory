# World Collision Decoration Filtering

## 1. Objective

Stop decorative scenery from behaving like solid walls while preserving collision for structures and props that should physically block actors. Keep movement collision in the renderer-independent game module and leave combat hitboxes, combos, and damage untouched.

## 2. Tasks

1. Trace world-map prop loading, movement resolution, camera obstruction, and collider debug drawing to identify all consumers of static AABBs.
2. Add explicit collision participation metadata to static AABBs, with existing manually authored obstacles remaining blocking by default for compatibility.
3. Mark foliage and mesh-derived terrain scenery as non-blocking where their broad bounds do not describe a physical obstacle. Give trees a narrow trunk collider where the data supports a credible trunk footprint; keep houses, walls/fences, towers, wells, carts, benches, lamps, and other solid props blocking.
4. Make movement resolution, depenetration, camera obstruction, and contact coloring honor the blocking flag. Keep non-blocking bounds available to the debug overlay, with distinct coloring/legend so they can be distinguished from physical colliders.
5. Add CPU regression tests for passing through bushes and broad terrain bounds while remaining blocked by a tree trunk and a solid wall/structure. Run the live `--walk-test`, full CTest, client smoke, and bootstrap smoke.
6. Update the roadmap and this spec's test/review reports only after all requested checks pass. Do not claim the whole world collision system complete if any decorative blocker remains unverified.

## 3. Files to Touch

- `game/include/glory/game/collision.hpp`
- `game/src/collision.cpp`
- `assets/data/world_map.json` and/or `assets/data/world_colliders.json`
- `engine/src/application.cpp` (debug collider snapshots/rendering only, if needed)
- `tests/collision_tests.cpp`
- `tests/server_simulation_tests.cpp` (keep the server integration assertion aimed at a real authored blocker after decorative terrain stops colliding)
- `PLAN.md`
- `.ai/specs/world-collision-decoration-filter/plan.md`
- `.ai/specs/world-collision-decoration-filter/test-report.md`
- `.ai/specs/world-collision-decoration-filter/review.md`

## 4. Risks & Mitigations

- Some scenery is visually decorative but should still block movement (for example, a tree trunk or fence). Classify by world-map prop type and use narrow authored bounds instead of globally disabling all prop collision.
- Existing code may use the obstacle list for both gameplay and camera obstruction. Check each consumer to avoid non-blocking foliage continuing to stop the camera or being incorrectly tinted as a physical contact.
- Existing tests and sample worlds construct AABBs directly. Preserve blocking as the default so unrelated collision behavior stays stable.
- Terrain meshes currently use broad mesh-derived AABBs. Remove those from actor blocking only after confirming they are decorative backdrop geometry; do not alter arena boundary enforcement.

## 5. Acceptance Criteria

- A capsule can walk through configured bushes and non-walkable decorative hill/cliff bounds without collision contact or camera obstruction.
- A capsule remains blocked by walls/fences/buildings and by a tree trunk collider that matches the trunk footprint rather than the canopy.
- Debug view clearly distinguishes blocking colliders from non-blocking debug bounds.
- Existing direct AABB collision behavior remains blocking by default; CPU regression tests cover both blocking and non-blocking cases.
- Build, full CTest, `glory_client.exe --walk-test`, `glory_client.exe --smoke-test`, and `glory_bootstrap.exe` pass.
- No gameplay hitbox, combo, or damage logic is changed.
