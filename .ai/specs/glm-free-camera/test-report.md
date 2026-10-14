# Test Report: GLM and Free-Fly Camera

## Commands Executed

- `& 'C:\Program Files\CMake\bin\cmake.exe' -S . -B build -G Ninja`
- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build`
- `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure`
- `.\build\glory_client.exe --smoke-test`
- `git diff --check`

## Overall Result

**PASS** — GLM 1.0.1 configured through FetchContent; final build succeeded; all 3 CTest tests passed; SDL/OpenGL smoke test created the 4.5 Core renderer, ran 120 fixed ticks, and exited successfully.

## Test Cases

- Camera view matrix and camera translation: Passed.
- Perspective projection and aspect-ratio update: Passed.
- Mouse yaw and pitch clamping at both limits: Passed.
- Fixed-delta forward movement, normalized diagonal movement, Space ascent, and Control descent: Passed.
- Invalid aspect ratio and near/far plane configuration rejection: Passed.
- Invalid negative timestep does not move camera: Passed.
- Existing fixed timestep suite: Passed.
- Existing input mapping/rebinding suite, including new Control scancode mappings compiled into engine: Passed.
- CTest total: 3/3 passed, 0 failed.
- SDL3/OpenGL 4.5 Core smoke test: Passed (`SDL3/OpenGL 4.5 context and 120 fixed ticks passed`).

## Build/Test Details

- CMake fetched GLM tag `1.0.1` and reported `GLM: Version 1.0.1`. It emitted upstream GLM CMake deprecation notices and optional PkgConfig/LibUSB notices; configuration still completed successfully.
- Initial build exposed missing SDL header typedefs for core `glEnable`; the engine now declares the correct OpenGL calling-convention signature. The final requested build completed with exit code 0.
- CTest exit code: 0; `glory_fixed_timestep`, `glory_input_mapping`, and `glory_camera` all passed.
- Smoke test exit code: 0; 120 fixed updates completed and the process exited normally.
- `git diff --check` is expected to report only the repository's existing LF-to-CRLF notices, with no whitespace errors.
