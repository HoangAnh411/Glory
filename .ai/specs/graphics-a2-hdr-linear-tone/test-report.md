# A2 Test Report

## Commands
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`
- `build/glory_client.exe --smoke-test-1080p`
- `git diff --check`

## Result: PASS
- Build completed successfully.
- All 37 CTest tests passed.
- 1080p smoke run created and checked the RGBA16F/depth framebuffer, resized it twice, restored the drawable size, and completed 120 fixed ticks.
- Timer query support was available. GPU timings are recorded in `docs/perf.md`.
- OpenGL renderer: NVIDIA GeForce RTX 3050 Ti Laptop GPU, driver 617.42, OpenGL 4.5.0.
- Confirmed SDL drawable size: 1920x1080. Latest 240-frame averages: shadow 0.106 ms, scene 0.315 ms, sky 0.010 ms, particle/overlay 0.045 ms, tonemap 0.071 ms, total GPU frame 0.582 ms.
- `git diff --check` completed without whitespace errors (Git reports only LF-to-CRLF notices).

## Shader and resize checks
- Application smoke launch compiled all runtime shaders and reported framebuffer completeness.
- HDR resize count was 3 on the smoke path: initial allocation, half-size, and restored drawable size.
