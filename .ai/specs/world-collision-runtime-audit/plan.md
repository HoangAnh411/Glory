# World Collision Runtime Audit and Fix

## Objective
Trace why the interactive game can leave the arena or cross visible world geometry, then fix the live paths and verify them through the actual game update callback rather than isolated solver tests.

## Tasks
1. Record executable-relative asset paths, current directory, loaded collider count, arena bounds, noclip state, and world-map prop bounds transformed by the renderer's exact model matrices.
2. Remove duplicate raw prop-collider injection and compare each rendered prop bound against the loaded collision data; report missing and >0.15 m mismatches.
3. Refactor the live fixed-update callback so scripted `--walk-test` drives the same input mapping/controller/combat/boss update path at 30, 60, and 144 render FPS, recording every fixed-step position and generating top-down traces.
4. Audit spawn, reset, respawn, network restore/reconciliation, knockback, dodge, boss charge, and server-authoritative movement. Ensure position imports resolve through the world solver and server instances load the same world collision data.
5. Correct the evidence-backed cause, add a 0.5 m arena safety clamp with a warning, and extend ground coverage beyond the arena boundary.
6. Capture eight arena-edge views with collider overlay; inspect images, run build/CTest/smoke and walk-test, update reports and roadmap only after all checks pass.

## Files
- `game/src/main.cpp`, `game/src/collision.cpp`, `game/src/character_controller.cpp`, `game/src/boss.cpp`, `game/src/prediction.cpp`
- `server/src/server_simulation.cpp`, `server/include/glory/server/server_simulation.hpp`
- `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp`, `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp`, `engine/src/ground_plane.cpp`
- `tests/collision_tests.cpp`, `tests/server_simulation_tests.cpp`
- `CMakeLists.txt`, `PLAN.md`, `.ai/specs/world-collision-runtime-audit/`
- runtime outputs under `captures/`

## Risks
- The live client can be offline or network-connected; both client prediction and authoritative server paths must enforce the same map collision.
- Rendered model bounds can include decorative geometry. Mismatch reporting must distinguish absent collision definitions from intentional non-blocking decoration.
- A circular safety limit may not match the physical wall; actual authored wall AABBs take priority where available.

## Acceptance Criteria
- Starting the executable from outside the repository loads and reports the expected absolute files, collider count, and arena data; missing/empty collision data fails loudly.
- Every rendered static prop is reported with mesh-derived world AABB versus collision AABB, including uncollided props and maximum error.
- `--walk-test` uses the live fixed-update callback and input mapping, detects any position outside the valid arena or inside a collider, and writes reproducible top-down PNG/JSON evidence for 30/60/144 FPS.
- Eight boundary captures show collider overlays aligned with visible edges; no visible world edge is traversable.
- Build, full CTest, smoke, and walk-test pass. Do not mark the roadmap item complete if any acceptance criterion fails.
