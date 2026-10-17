# World Collision Wall and Opening Corrections

## 1. Objective

Correct two opposite collision errors in the live world: visible perimeter walls must stop actors, while visible openings such as the village gate must remain passable. Replace broad type-wide assumptions with per-instance compound collision data, preserving actor/gameplay hitboxes and combat behavior.

## 2. Evidence and Current Causes

- `assets/data/world_colliders.json` lists `terrain_cliff_rock` as globally non-blocking. This includes the six near-arena wall instances `terrain_cliff_rock#60` through `#62` and `#65` through `#67`.
- `village_gate_tower#35` and `village_gate_tower#51` each load one AABB from local X `-8.5` to `8.5`, Y `0` to `8.5`, and Z `-1.8` to `1.8`. This box fills the visible gate opening between the supports.
- The renderer and collision loader use the same prop transform convention. The existing type override mechanism only supports one box and cannot represent gate supports or per-instance wall sections.

## 3. Tasks

1. Extend authored world collision data to support instance-specific collision overrides and multiple local AABBs per prop. Preserve the current blocking default and existing type-level decoration filtering.
2. Replace the gate tower's full bounds with compound side-support bounds that leave a capsule-sized opening through each gate arch. Ensure both north/south gate instances behave consistently despite yaw.
3. Restore collision for the six near-arena cliff-wall instances using authored, narrow sections aligned with the visible blocking wall surfaces. Keep the four distant backdrop cliffs non-blocking and keep decorative hills/bushes non-blocking.
4. Update collision debug rendering and startup audit to label instance overrides and show compound parts distinctly; verify camera obstruction uses the physical parts only.
5. Add CPU tests for walking through both gate openings, being blocked by both gate supports and near-arena wall sections, and passing through distant scenery. Add a live `--walk-test` scenario that traverses the gate and runs into a near-arena wall.
6. Run build, full CTest, client smoke, bootstrap smoke, and live walk-test. Capture the gate and wall collision wireframes in a separate validation output directory and inspect them. Update roadmap/spec reports only after checks pass.

## 4. Files to Touch

- `game/include/glory/game/collision.hpp`
- `game/src/collision.cpp`
- `assets/data/world_colliders.json` and/or `assets/data/world_map.json`
- `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp`, `engine/src/debug_panel.cpp`
- `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp`
- `game/src/main.cpp`
- `tests/collision_tests.cpp`, `tests/server_simulation_tests.cpp`
- `PLAN.md`
- `.ai/specs/world-collision-wall-openings/`

## 5. Risks & Mitigations

- The cliff meshes have large irregular footprints. Derive their physical strips from the visible boundary placement and compare wireframes in captures; do not restore their broad mesh AABBs as blockers.
- Gate tower orientation can mirror/rotate the opening. Test the same traversal from both sides and at both instances.
- The arena circle already serves as a safety boundary. Preserve it while adding wall collision so the wall and safety clamp do not create a wider false barrier.
- Compound AABBs are still approximations. Keep parts narrow and expose them in debug view; avoid claiming mesh-accurate collision.

## 6. Acceptance Criteria

- Player and boss cannot pass through the six near-arena physical cliff-wall sections.
- The center opening through both gate structures is traversable by the standard capsule; gate supports remain solid.
- Decorative bushes, hills, and distant cliffs stay non-blocking.
- CPU collision tests and live walk-test cover both the passable gap and solid wall cases using the actual loaded world collision data.
- Build, all CTest tests, client smoke, bootstrap smoke, and walk-test pass. Gate and wall debug captures are visually checked.
- No attack hitbox, combo, damage, or other combat behavior changes.

## 7. Follow-up Findings and Corrections

- The first cliff overrides covered only local X `8..12` (or `-12..-8`) while the rendered `terrain_cliff_rock` mesh spans local X `-12..12`. Actors could cross most of the visible rock. Use the mesh's full local bounds for each of the six near-world instances.
- `CollisionWorld::resolve` applied the radius-34 circle as an interior wall on every step, despite only logging recovery after radius `34.5`. This closed routes in the north/south zones. Make the circle recovery-only and set its radius to 90 m so authored colliders govern the populated map zones.
- For map variants that supply `multi_collision`, an explicit instance/type override shape replaces that map shape; it must not be appended afterward and silently narrow the authored opening.
- Acceptance now includes live travel past both gate structures beyond the former radius-34 boundary, per-instance wall blocking tests with the safety clamp disabled, and a safety-margin regression test.
