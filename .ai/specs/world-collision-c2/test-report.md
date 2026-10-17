# C2 Test Report

## Commands
- `cmake --build build`
- `build/glory_collision_tests.exe`
- `ctest --test-dir build --output-on-failure`
- `build/glory_client.exe --smoke-test-1080p`

## Result: PASS
- Build succeeded.
- Focused collision test passed, including repeated wall approaches at normal (5.5 m/s) and dodge (12 m/s) speeds at 30, 60, and 144 ticks/s.
- The 90-degree corner test remained outside both AABBs after 1000 ticks. Diagonal slide regression remains covered by the wall-slide test.
- A deterministic 10,000-step randomized test stayed inside the arena's capsule-inset radius (33.6 m) and above y=0.
- Low 0.2 m step remained walkable; 0.6 m authored step was blocked with a 0.25 m step offset.
- CTest passed 37/37. 1080p smoke completed 120 fixed ticks, recreated the framebuffer three times, and measured 1.403 ms total GPU time on the RTX 3050 Ti smoke scene.

## Scope
- Player movement paths already converge on `CollisionWorld::resolve`; this stage removes the 256-segment tunneling cap and enforces the arena boundary. Boss and camera integration remain C3.
