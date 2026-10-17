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
- Client smoke: PASS; OpenGL 4.5 context, 120 fixed ticks, HDR framebuffer resize checks completed. On the RTX 3050 Ti Laptop GPU at 1280x720, the recorded average GPU frame was 0.925 ms.
- Bootstrap smoke: PASS (`Glory project scaffold ready`).
- Live walk-test: PASS at 30, 60, and 144 render FPS. Each run performed 19,200 fixed updates across 16 directions and checked player/boss positions against the loaded collision world.
- At 30 FPS the scripted routes ended at north gate `z=31.7502`, south gate `z=-31.7502`, and the near-arena cliff wall `x=-29.9134`.
- The executable was launched with working directory `build/wall-opening-validation`; it loaded map and collider data from the absolute repository asset path and reported 82 colliders.

## Coverage

- CPU tests load authored map and collider JSON, walk through both gate openings, verify a boss capsule fits through the north gate, block traversal across supports on both gates, and verify a near-arena cliff section stops movement.
- Existing tests continue to check distant cliffs and decorative terrain are non-blocking.
- Top-down route/collider image: `build/wall-opening-validation/captures/walk-test-30.png`.
- Gate close-ups inspected: `build/wall-opening-validation/captures/north-gate-topdown.png` and `build/wall-opening-validation/captures/south-gate-topdown.png`.
- Machine-readable route data: `build/wall-opening-validation/captures/walk-test-30.json`, `walk-test-60.json`, and `walk-test-144.json`.

## Notes

- The usable clear width is smaller than the authored gate arch because lantern colliders sit beside the opening. The central route is clear for both the player and the larger boss capsule; movement around the lantern posts remains blocked as expected.
- Cliff sections use authored AABBs derived from the mesh's local bounds and instance transforms. They remain approximations of irregular rock geometry.
