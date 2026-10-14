# Test Report: Input Capture and Remapping

## Commands Executed

- `cmake -S . -B build -G Ninja` — PATH lookup failed because `cmake` was not available in this PowerShell session. Retried with the absolute path recorded in `AGENTS.md`:
  - `& 'C:\Program Files\CMake\bin\cmake.exe' -S . -B build -G Ninja`
- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build` — first attempt exposed a compile error in the newly added input helper; after correction, the build completed successfully.
- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; & 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure`
- `.\build\glory_client.exe --smoke-test`
- `git diff --check`

## Overall Result

**PASS** — final build succeeded, both CTest tests passed, the SDL3/OpenGL smoke test completed 120 fixed ticks, and `git diff --check` reported no whitespace errors.

## Test Cases

- Default W and left-mouse bindings: Passed.
- Action pressed/held/released transitions across successive frames: Passed.
- Rebinding to Up and verifying the old control no longer activates the action: Passed.
- Rejecting unknown controls and invalid action identifiers without changing the existing binding: Passed.
- Allowing one physical control to intentionally activate multiple actions: Passed.
- Existing fixed-timestep regression suite: Passed.
- SDL3/OpenGL 4.5 application smoke test, 120 fixed ticks: Passed (`SDL3/OpenGL 4.5 context and 120 fixed ticks passed`).

## Build/Test Details

- CMake configure completed with Ninja and the configured LLVM-MinGW toolchain. CMake printed optional dependency notices for PkgConfig and LibUSB; neither prevented configuration.
- Final build completed successfully; Ninja reported no pending work on the subsequent test run.
- CTest: 2/2 passed (`glory_fixed_timestep`, `glory_input_mapping`); 0 failed.
- Initial build failure: `engine/src/input_system.cpp` had an unqualified `InputFrame` parameter in the release helper. It was corrected to `input::InputFrame`; the next build passed.
- No failures remained after correction.
