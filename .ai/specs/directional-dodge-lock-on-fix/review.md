# Review

## 1. Must Fix

None.

## 2. Should Fix

None for this focused bug fix.

## 3. Suggestions

- A future runtime `--combat-test` scenario could include explicit WASD dodge input while lock-on is active, so this behavior is also covered through the client loop.

## Verification

- `game/src/character_controller.cpp` now checks normalized movement input before lock-on target direction.
- `tests/character_controller_tests.cpp` covers both explicit movement priority and target-only fallback.
- The change does not bypass the collision solver or modify combat data.
