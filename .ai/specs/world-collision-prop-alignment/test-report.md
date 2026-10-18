# Test Report

## Baseline before data fix

After adding the regression assertions but before reapplying the collider data fix, `cmake --build build; .\\build\\glory_collision_tests.exe` exited with code 1 and reported 5 failures:

- `lamp in the south spawn road blocks movement`
- `south spawn lamp blocks a capsule approaching from the north`
- `south spawn lamp blocks a capsule approaching from the south`
- `gate support collider reaches the visible gate height` for both gates

This proves the new checks detect the old `blocksMovement=false` and `6.0 m` gate support data.

## Final verification

- `cmake --build build`: PASS, exit code 0.
- `build/glory_collision_tests.exe`: PASS, `All collision tests passed`.
- `ctest --test-dir build --output-on-failure`: PASS, 38/38 tests.
- `build/glory_client.exe --smoke-test`: PASS, SDL/OpenGL context, HDR resize checks, 120 fixed ticks.
- `build/glory_client.exe --walk-test`: PASS at 30, 60, and 144 FPS. North gate reached about `44.8 m`; south route reached about `-43.6 m` by briefly offsetting around the blocking lamp; spawn side walls stopped at `x ~= +/-7.85`.
- `git diff --check`: PASS.

## Runtime evidence

The collision diagnostic reports:

- `lamp_wooden_lantern#10` render and collider bounds match within `2.4e-7 m`.
- `village_gate_tower#35` and `#51` render and collider bounds match at `maxError=0 m`.

New artifacts were written under:

- `captures/world-collision-prop-alignment/runtime/captures/center_boss_colliders.png`
- `captures/world-collision-prop-alignment/runtime/captures/pillar_colliders.png`
- `captures/world-collision-prop-alignment/walk/captures/walk-test-30.png`
- `captures/world-collision-prop-alignment/walk/captures/walk-test-60.png`
- `captures/world-collision-prop-alignment/walk/captures/walk-test-144.png`

The walk-test top-down image was inspected manually. The south route visibly moves sideways around the lamp before continuing to the gate.

## Known validation limitation

`--capture-colliders` writes the first runtime images and verifies the collider bounds, then exits during the existing wall camera scenario with `Capture root-joint marker is behind the camera`. This is a camera capture failure outside the collider data change; the standard smoke test and walk-test pass.
