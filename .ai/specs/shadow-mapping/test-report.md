# Test Report: Shadow Mapping

## Commands Executed

- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build` — exit code 0.
- `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure` — exit code 0; 7/7 tests passed.
- `.\build\glory_client.exe --smoke-test` — exit code 0; output: `SDL3/OpenGL 4.5 context and 120 fixed ticks passed`.

## Result

**PASS**

## Cases

- Build of the client, engine, and all test targets — Passed.
- `glory_fixed_timestep` — Passed.
- `glory_input_mapping` — Passed.
- `glory_camera` — Passed.
- `glory_asset_loader` — Passed.
- `glory_directional_light` — Passed.
- `glory_skeletal_animation` — Passed.
- `glory_shadow` — Passed; CPU-only checks verified projected scene center, orthographic X/Y extents, downward light orientation, and rejection of invalid direction/extent.
- Shadow FBO and depth/main shader initialization — Passed through the client smoke run; FBO construction checks `GL_FRAMEBUFFER_COMPLETE` and shader construction validates compile/link status.
- 120-tick smoke test — Passed; application exited cleanly with the OpenGL 4.5 context active through renderer teardown.

CTest summary: **100% tests passed, 0 tests failed out of 7**.

## Tester Notes

- The first build attempt caught two compile-time issues in the new test call and const renderer binding helper. Both were corrected; the final build, all seven tests, and smoke test passed.
- The first configure triggered by the CMake change printed upstream SDL/GLM/FetchContent configuration/deprecation notices; the project configured and built successfully.
- Smoke exercises FBO creation, framebuffer completeness, shader initialization, rendering-loop teardown, and RAII destruction. It does not visually grade shadow softness or run a dedicated GPU leak detector/sanitizer.
