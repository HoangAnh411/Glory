# C1 Test Report

## Commands
- `cmake -S . -B build -G Ninja`
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`
- `build/glory_client.exe --smoke-test-1080p`
- `build/glory_client.exe --capture-colliders`

## Result: PASS
- Build succeeded. One pre-existing ignored `[[nodiscard]]` warning remains in the respawn trigger update path.
- CTest passed 37/37, including `glory_collision` data loading assertions.
- The runtime collision loader produced 65 world-space prop AABBs from authored `collision` and `multi_collision` bounds in `world_map.json`; arena metadata loads with center `(0,0)` and radius `34 m`.
- 1080p smoke passed, recreated HDR FBO three times, and completed 120 fixed ticks. RTX 3050 Ti total GPU frame measured 1.498 ms on this light smoke scene.
- `--capture-colliders` produced six 1080p PNG captures with the collider overlay. `captures/center_boss_colliders.png` was opened for visual review: prop boxes are green, arena boundary is yellow.

## Scope
- C1 adds static collider data, debug wireframes/contact tint, and controller-only noclip. Continuous movement sweeps, arena blocking, boss collision, and camera integration are deferred to C2/C3.
