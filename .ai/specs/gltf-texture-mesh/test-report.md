# Test Report: Textured glTF Mesh

## Commands

- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build` — exit code 0.
- `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure` — exit code 0.
- `.\build\glory_client.exe --smoke-test` — exit code 0.

## Result

**PASS**

## Cases

- C++20 engine/client/test targets compiled, including the pinned stb and cgltf implementations — Passed.
- `glory_fixed_timestep` — Passed.
- `glory_input_mapping` — Passed.
- `glory_camera` — Passed.
- `glory_asset_loader` — Passed: decoded the 8x8 sample PNG, parsed 24 glTF vertices and 36 indices and texture URI, rejected invalid image bytes and a missing glTF file.
- Smoke test — Passed: SDL3 created an OpenGL 4.5 Core context, loaded and rendered the sample textured model, completed 120 fixed ticks, and exited with code 0.

CTest summary: **100% tests passed, 0 tests failed out of 4**.
