# Test Report: World Collision Regression Fix

## Commands and Results

- `cmake --build build`: PASS; rebuilt `glory_collision_tests.exe` and `glory_client.exe`.
- `ctest --test-dir build --output-on-failure`: PASS, 37/37 tests.
- `build/glory_client.exe --walk-test`: PASS at 30, 60, and 144 render FPS using the live fixed-update callback.
- `build/glory_client.exe --smoke-test`: PASS; OpenGL 4.5 context and 120 fixed ticks completed.
- `build/glory_bootstrap.exe`: PASS (`Glory project scaffold ready`).
- `git diff --check`: PASS.

## Reproduction and Fix Verification

- Before the first correction, the southward live route from spawn stopped at Z=-7.75 against `lamp_wooden_lantern#10`. The side routes reached deeper cliff colliders at X=+/-13.4595, but the player could enter the visible hills in front of those cliffs because hill instances `#63/#64/#68/#69` were non-blocking.
- Kept `lamp_wooden_lantern#10` non-blocking and authored movement collision for the four near-spawn hill instances. Other distant hill instances and other lamps retain their previous settings.
- Added CPU checks that travel from spawn through both gate openings without contact, and that travel toward both side walls is stopped at the near-spawn hill colliders.
- Extended live routes to start at spawn and travel through both gates and toward both side walls. At 30, 60, and 144 FPS, routes reached north Z=+44.9 to +45.0 m and south Z=-44.9 to -45.0 m. Side routes stopped at X=-7.85 m on `terrain_hill_green#64` and X=+7.85 m on `terrain_hill_green#69`.
- Final live captures and JSON traces are in `.ai/specs/world-collision-regression/runtime-captures/captures/`.

## Notes

- The first wall test checked only the deeper cliff collider and missed entry into the visually nearer hill meshes. The updated test now stops at the near-spawn hills on both sides.
- An initial baseline `--walk-test` was launched with the repository root as its working directory before the output path was checked. That command writes `captures/walk-test-*.png/json`; the root `captures/` directory already contained untracked files, so those specific files may have been regenerated. Their prior contents were not recorded and cannot be verified from Git. The final verification wrote to the feature spec directory.
- No `PLAN.md` checkbox was changed because the collision roadmap items were already complete and this work is a regression fix.
