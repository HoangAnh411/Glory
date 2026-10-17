# C1: World Collider Data and Debug Visualization

## Objective
Load static world colliders and arena bounds into the renderer independent game collision world. Expose collider wireframes, contact coloring, and noclip through F3 without coupling the collision solver to OpenGL.

## Tasks
1. Extend collision-world data loading for static prop AABBs, yaw/scale transforms, and arena boundary metadata. Prefer authored bounds from world-map entries and allow overrides in `assets/data/world_colliders.json`.
2. Keep collider storage and queries in `game`; do not add graphics dependencies to collision or controller modules.
3. Provide engine debug snapshots as plain world-space boxes/boundary data, project them in the existing debug panel only when enabled, and mark colliders touching the player.
4. Add an F3 noclip switch routed to the game controller; it must bypass movement collision only and leave combat hitboxes untouched.
5. Add CPU tests for collider loading/transforms/boundary and a capture smoke for visible debug colliders.

## Files
- `game/include/glory/game/collision.hpp`, `game/src/collision.cpp`
- `game/src/main.cpp`, `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp`
- `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp`
- `assets/data/world_colliders.json`
- `tests/collision_tests.cpp`, CMake test registration if required
- `PLAN.md`, `.ai/specs/world-collision-c1/`

## Risks
- Rotated authored AABBs become world AABBs and may conservatively widen at non-90-degree yaw; C3 can use the same bounds for camera until OBB support is warranted.
- The visible arena radius is currently 34 m; encode it once in collision data so renderer and gameplay can converge on the same value.
- Noclip is a debug control and must not modify server/combat state.

## Acceptance
- World collision data loads in CPU-only game code and contains arena perimeter plus authored prop colliders.
- F3 wireframe/contact visualization and noclip toggle work in runtime.
- CTest, build, smoke, and collider capture pass; no combat/hitbox behavior changes.
