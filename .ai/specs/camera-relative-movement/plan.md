# Camera-Relative Movement Plan

## 1. Objective

Implement camera-relative character locomotion while keeping hitboxes, combo logic,
damage, and the existing collision solver unchanged. Movement must use the actual
fixed update path, support smooth acceleration and facing rotation, and expose a
runtime `--move-test` that records measurable trajectories and top-down images.

## 2. Tasks

1. Add a runtime movement test mode and command-line flag. Exercise the real
   `fixedUpdate` callback with a fixed timestep while simulating 30, 60, and
   144 FPS render cadence.
2. Validate W/A/S/D and W+A at camera yaw 0, 90, 180, 270, and an arbitrary yaw;
   measure velocity from consecutive positions, check S turns toward the camera,
   cap facing rotation at 720 degrees/second, verify wall sliding and equal
   distance across render rates, and write JSON plus top-down PNG traces.
3. Update `CharacterController` to accelerate/decelerate over about 0.1 seconds,
   use a 720 degrees/second facing limit, preserve normalized camera-relative
   directions, and make stationary dodge move backward from the facing direction.
4. Extend controller tests for acceleration, deceleration, turn-rate cap, dodge
   fallback, and solver-preserved wall sliding without changing capsule dimensions.
5. Build, run the complete CTest suite, run `--move-test` and smoke test, inspect
   generated trajectory images, then write tester and reviewer reports.

## 3. Files to touch

- `game/src/main.cpp`: parse `--move-test`, run the fixed-update movement
  scenarios, emit validation JSON and top-down PNG traces.
- `game/include/glory/game/character_controller.hpp`: add movement tuning fields
  needed by the controller configuration.
- `game/src/character_controller.cpp`: implement smooth horizontal velocity,
  720 degrees/second facing, and backward stationary dodge direction.
- `tests/character_controller_tests.cpp`: cover the new movement invariants.
- `.ai/specs/camera-relative-movement/test-report.md`: record real command output.
- `.ai/specs/camera-relative-movement/review.md`: record the independent review.

`PLAN.md` will remain unchanged until this feature has a proven runtime pass and
the repository workflow artifacts are complete.

## 4. Risks and mitigations

- Smoothing can make short input taps travel different distances. Use a bounded
  acceleration and deceleration with the same fixed timestep, and test sustained
  input plus release phases.
- Collision response can remove the inward component of velocity. Measure the
  post-solver velocity and explicitly test a diagonal approach to an AABB.
- Render cadence must not change fixed simulation distance. Drive the same fixed
  update callback from simulated 30/60/144 FPS accumulators.
- Existing collision work is uncommitted. Keep it unstaged and avoid modifying
  its files except for the existing `main.cpp` hunks needed to add the new flag
  and test branch.

## 5. Acceptance criteria

- W/A/S/D are camera-yaw-relative and W+A has the same speed as a cardinal move.
- S rotates the character toward its run direction and moves toward the camera.
- Horizontal speed reaches and leaves the target smoothly over approximately 0.1 s;
  idle input does not rotate the character.
- Facing rotation never exceeds 720 degrees/second.
- Jump, dodge, knockback, and normal movement continue through the existing solver;
  no direct position write is added to gameplay movement.
- Stationary dodge moves backward relative to the current facing direction.
- Actual velocity measured from positions matches the scripted direction after the
  acceleration settling window, and diagonal wall input slides along the wall.
- The same sustained scenario produces equal distance at simulated 30, 60, and
  144 FPS within the test tolerance.
- `--move-test` exits 0 after writing JSON and top-down PNG traces; the pre-fix
  baseline of the same harness records FAIL.
- Build, CTest, and `--smoke-test` pass. No hitbox, combo, or damage behavior is
  changed, and `PLAN.md` is not ticked without the runtime evidence.
