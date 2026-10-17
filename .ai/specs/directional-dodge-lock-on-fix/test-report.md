# Test Report

## Commands

- `cmake --build build --target glory_character_controller_tests`
- `build/glory_character_controller_tests.exe`
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`
- `build/glory_client.exe --smoke-test`

## Result

PASS.

## Cases

- Focused character controller test: PASS — lock-on + WASD uses the explicit camera-relative dodge vector.
- Focused character controller test: PASS — lock-on without movement input still dodges toward the target.
- Existing dodge speed, invulnerability, cooldown, movement, jump, and collision controller cases: PASS.
- Full CTest: PASS — 38/38 tests passed.
- Client smoke test: PASS — SDL3/OpenGL context, fixed ticks, HDR framebuffer and render passes completed.

## Scope Check

- No hitbox, combo, damage, or cooldown data changed.
- Dodge displacement still goes through `CollisionWorld::resolve`.
- Existing unrelated collision and capture changes remained outside this feature change.
