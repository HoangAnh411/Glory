# Attack Direction and Target Lock-On Plan

## 1. Objective

Add deterministic attack-facing selection and target lock-on while preserving the existing combat data, hitbox geometry, combo progression, damage calculation, and cooldown timing. Keep targeting decisions in gameplay code so the render loop only presents the resulting state.

## 2. Tasks

1. Add a small gameplay targeting model for Manual, Assist, and Auto attack aim modes.
2. Select attack yaw at the exact skill/primary-attack request boundary and orient the character before the existing combat update call.
3. Expose aim mode and selected target state to the HUD and add the F3 controls.
4. Add `--combat-test` using the production fixed-update callback and deterministic scenarios. First run it against the existing behavior and record the expected failure.
5. Add target-lock selection, visibility/range validation, cancellation, middle-mouse binding, and horizontal flick target cycling.
6. Add lock-specific camera framing while retaining the existing spring arm and collision constraint.
7. Make attack and dodge use the selected lock target direction without bypassing the character collision solver.
8. Build, run the unit suite, smoke test, combat test, and inspect the generated combat captures.
9. Write tester and reviewer reports, then create two focused commits: attack direction and lock-on.

## 3. Files to touch

- `game/include/glory/game/character_controller.hpp`
- `game/src/character_controller.cpp`
- `game/include/glory/game/action_map.hpp`
- `game/src/action_map.cpp`
- `game/include/glory/game/targeting.hpp`
- `game/src/targeting.cpp`
- `game/src/main.cpp`
- `engine/include/glory/engine/application.hpp`
- `engine/src/application.cpp`
- `engine/include/glory/engine/third_person_camera.hpp`
- `engine/src/debug_panel.cpp`
- `tests/targeting_tests.cpp`
- `CMakeLists.txt` or the project test target only if required by the existing build layout
- `.ai/specs/attack-direction-lock-on/test-report.md`
- `.ai/specs/attack-direction-lock-on/review.md`

Generated runtime evidence belongs under `captures/combat-test/`.

## 4. Risks and mitigations

- Existing `CombatSystem::update` owns hitbox placement; only its facing-yaw input will change, and skill definitions remain untouched.
- The current application has a boss-only lock toggle; target selection will be validated before camera state is updated and will clear on death or range/visibility failure.
- Camera lock can conflict with the existing mouse orbit and spring arm. Lock rotation will be smooth and will use the same arm constraint path, with no lateral camera slide.
- The repository already contains unrelated dirty collision and capture files. They must remain untouched and unstaged.
- OpenGL pixel evidence may be unavailable on headless machines; the test must report that condition rather than claiming a pass without readback.

## 5. Acceptance criteria

- Manual attacks face camera yaw; Assist uses a live target only inside a 35 degree cone and attack range; Auto uses the nearest live target in attack range.
- Attack orientation completes within 0.1 seconds and never changes hitbox dimensions, combo, damage, or cooldown values.
- Tab and middle mouse toggle lock-on. Selection requires visibility and 25 m range; lock cancels beyond 30 m or on death.
- Locked camera frames player and target, retains spring-arm collision behavior, and never places the camera inside a collider.
- Locked attacks and dodges use the target direction, while normal movement-facing remains movement-driven.
- `--combat-test` exercises real fixed updates, reports baseline failure before implementation, then passes all attack and lock assertions after implementation.
- `cmake --build build`, `ctest --test-dir build --output-on-failure`, `--smoke-test`, and `--combat-test` pass after the two commits.
- Six or more combat capture PNGs are generated and visually inspected.
- `PLAN.md` remains unchanged unless a matching roadmap item is proven and explicitly applicable.
