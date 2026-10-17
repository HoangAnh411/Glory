# Test Report

## Commands

- `cmake --build build --target glory_targeting_tests glory_client`
- `build/glory_targeting_tests.exe`
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`
- `build/glory_client.exe --smoke-test`
- `build/glory_client.exe --combat-test`

## Result

PASS.

## Cases

- Targeting test: PASS — a newly selected target behind a blocking AABB is rejected.
- Targeting regression: PASS — an already active lock remains valid when the same collider blocks line of sight.
- Targeting regression: PASS — active lock releases when the target dies.
- Targeting regression: PASS — active lock releases when the target exceeds 30 m.
- Full CTest: PASS — 38/38 tests passed.
- Client smoke test: PASS — SDL3/OpenGL context, HDR framebuffer, render passes and fixed ticks completed.
- Runtime `--combat-test`: PASS — `lock_frame`, `lock_cancel`, and `lock_hold` phases passed; final output was `--combat-test PASS`.

## Scope Check

- Line of sight remains required for initial target selection.
- Active lock maintenance checks only target liveness and horizontal range, matching the lock-on rule.
- No hitbox, combo, damage, or cooldown data changed.
- Existing unrelated collision and capture changes remained outside this feature change.
