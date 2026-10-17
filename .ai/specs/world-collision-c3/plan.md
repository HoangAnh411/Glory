# C3: Shared Boss and Camera Colliders

## Objective
Use the same loaded world collider data for boss movement, lightweight player/boss separation, and third-person camera obstruction while ignoring decorative small props for the camera.

## Tasks
1. Inject the loaded `CollisionWorld` into `Boss`; depenetrate its spawn and retain shared collision for chase/charge.
2. Resolve small horizontal player/boss capsule separation through each actor's world collision solver, without changing combat hitbox dimensions or combo logic.
3. Pass the same world-space AABBs and arena bounds to the camera obstruction query; ignore bounds whose largest dimension is under 1.2 m.
4. Add CPU tests for boss movement blocked by a world wall, actor separation, and camera large/small AABB obstruction behavior.
5. Build, CTest, smoke, inspect current collider capture, update plan/spec reports, and commit C3 separately.

## Files
- `game/include/glory/game/collision.hpp`, `game/src/collision.cpp`
- `game/include/glory/game/character_controller.hpp`, `game/src/character_controller.cpp`
- `game/include/glory/game/boss.hpp`, `game/src/boss.cpp`, `game/src/main.cpp`
- `engine/include/glory/engine/application.hpp`, `engine/include/glory/engine/model_renderer.hpp`
- `engine/src/application.cpp`, `engine/src/model_renderer.cpp`
- `tests/collision_tests.cpp`, `tests/boss_tests.cpp`
- `PLAN.md`, `.ai/specs/world-collision-c3/`

## Risks
- Boss combat position mirrors the combat dummy; apply separation to the actor position and then refresh the dummy base without changing its hitbox.
- Camera collision uses world AABB approximations and may be wider for rotated props; skip small decorative bounds below 1.2 m.
- Avoid changing camera orbit controls or combat damage/hitbox data.

## Acceptance
- Boss movement cannot pass through a wall and boss spawn is depenetrated.
- Player and boss resolve overlapping capsules without changing combat definitions.
- Camera obstruction is sourced from the same loaded AABBs and arena boundary, excluding small props.
- Build, CTest, and smoke pass.
