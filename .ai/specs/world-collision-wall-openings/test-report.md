# Test Report: World Collision Wall and Opening Corrections

## Commands

- `cmake --build build`
- `ctest --test-dir build --output-on-failure`
- `build/glory_client.exe --smoke-test`
- `build/glory_bootstrap.exe`
- From `build/wall-opening-validation`: `../glory_client.exe --walk-test`

## Results

- Build: PASS.
- CTest: PASS, 37/37 tests.
- Client smoke: PASS; OpenGL 4.5 context, 120 fixed ticks, HDR framebuffer resize checks completed. On the RTX 3050 Ti Laptop GPU at 1280x720, the recorded average GPU frame was 0.845 ms.
- Bootstrap smoke: PASS (`Glory project scaffold ready`).
- Live walk-test: PASS at 30, 60, and 144 render FPS. Each run performed 19,200 fixed updates across 16 directions and checked player/boss positions against the loaded collision world.
- At 30 FPS the scripted routes traveled through and beyond the gates to north `z=38.5002` and south `z=-38.5002`; the player-facing side of the west cliff stopped movement at `x=-13.4595`.
- The executable was launched with working directory `build/wall-opening-validation`; it loaded map and collider data from the absolute repository asset path, reported 82 colliders, and used a 90 m safety radius.

## Coverage

- CPU tests load authored map and collider JSON, traverse both gate openings and the open north/south roads past the old radius-34 clamp, verify a boss capsule fits through the north gate, block traversal across supports on both gates, and independently test all six cliff sections with the arena recovery radius expanded so it cannot mask a missing wall collider.
- A synthetic radius-34 test verifies movement is allowed through a gap inside the 0.5 m recovery margin and recovery only runs after that margin is exceeded.
- A temporary JSON fixture verifies an instance compound override replaces the map's generated `multi_collision` shape rather than adding both shapes together.
- Existing tests continue to check distant cliffs and decorative terrain are non-blocking.
- Top-down route/collider image: `build/wall-opening-validation/captures/walk-test-30.png`.
- Gate close-ups inspected: `build/wall-opening-validation/captures/north-gate-topdown.png` and `build/wall-opening-validation/captures/south-gate-topdown.png`.
- Machine-readable route data: `build/wall-opening-validation/captures/walk-test-30.json`, `walk-test-60.json`, and `walk-test-144.json`.

## Notes

- The usable clear width is smaller than the authored gate arch because lantern colliders sit beside the opening. The central route is clear for both the player and the larger boss capsule; movement around the lantern posts remains blocked as expected.
- Cliff sections use authored AABBs derived from the mesh's local bounds and instance transforms. They remain approximations of irregular rock geometry.
