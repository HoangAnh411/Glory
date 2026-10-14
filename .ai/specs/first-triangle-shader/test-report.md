# Test Report: First Shader Triangle

## Commands Executed

- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build`
- `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure`
- `.\build\glory_client.exe --smoke-test`
- `git diff --check`
- `rg -n "\bgl[A-Z][A-Za-z0-9_]*\s*\(" engine game` (no direct OpenGL function invocations found)

## Overall Result

**PASS** — final build succeeded, existing CTest tests passed (2/2), and the application smoke test created the OpenGL 4.5 Core renderer, rendered during 120 fixed ticks, and exited successfully. Whitespace check passed; it emitted only Git's existing LF-to-CRLF notices.

## Test Cases

- All engine sources, including the SDL procedure loader, shader compile/link, triangle VAO/VBO, and renderer: Passed (CMake/Ninja build exit code 0).
- Existing fixed-timestep test: Passed.
- Existing input mapping/rebinding test: Passed.
- Combined CTest suite: 2/2 passed, 0 failed.
- OpenGL 4.5 Core smoke path, including procedure lookup, GLSL compilation/link, vertex upload, draw loop, resource teardown, and 120 fixed ticks: Passed (`SDL3/OpenGL 4.5 context and 120 fixed ticks passed`).
- Architecture scan for direct `gl*()` invocations in `engine/` and `game/`: Passed; rendering calls are made through loaded function pointers owned by the engine.

## Build/Test Details

- The first build attempt found missing function-pointer typedefs for core GL calls (`glDrawArrays`, `glViewport`, `glClearColor`, and `glClear`) and the SDL video declaration for `SDL_GL_GetProcAddress`. The engine now declares the four core signatures with the OpenGL calling convention and includes `SDL_video.h` in the loader implementation; the next and final build passed.
- Final build command exit code: 0.
- CTest command exit code: 0; 2 tests passed.
- Smoke test command exit code: 0; the process reported that the OpenGL 4.5 context and all 120 fixed ticks passed.
- `git diff --check` exit code: 0. Git printed line-ending conversion notices for existing modified text files, not whitespace errors.
