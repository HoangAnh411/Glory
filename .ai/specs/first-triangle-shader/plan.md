# First Shader Triangle Plan

## 1. Objective

Render an interleaved-position/color triangle through an OpenGL 4.5 Core shader program, with every OpenGL operation and resource owned by `glory_engine`. Dynamically load all used OpenGL entry points through SDL3, report shader/program diagnostics, and release shader/program/VAO/VBO resources before the SDL GL context is destroyed. Preserve the existing fixed timestep, input callback, DLL copy rules, and smoke-test contract.

## 2. Tasks

1. Add an engine OpenGL function table that resolves each required OpenGL procedure with `SDL_GL_GetProcAddress` and fails with the missing procedure name.
2. Add RAII shader compilation/linking with GLSL `#version 450 core`, compile/link status checks, and complete info-log diagnostics. Temporary shader objects must be deleted after linking and on every failure path; the linked program must be deleted at renderer teardown.
3. Add an engine vertex buffer/array owner for three interleaved vertices containing `vec3` position and `vec3` color, configuring both vertex attributes.
4. Add a triangle renderer that clears, binds the program/VAO, and issues a three-vertex `glDrawArrays` call using only loaded function pointers.
5. Integrate rendering into the existing application loop while leaving event processing and fixed-timestep/input updates intact. Scope renderer destruction before context destruction on both normal and error exits.
6. Run the requested build, existing CTest suite, and 120-tick application smoke test; record real commands/results.
7. Independently review engine/game boundaries, function loading, diagnostics, cleanup paths, and preservation of prior features; record findings.

## 3. Files to Create or Modify

- `engine/include/glory/engine/gl_api.hpp` — OpenGL types/constants/function table and SDL procedure loading contract.
- `engine/src/gl_api.cpp` — load every required function pointer and produce actionable missing-symbol errors.
- `engine/include/glory/engine/shader_program.hpp` — non-copyable RAII shader program interface.
- `engine/src/shader_program.cpp` — shader compilation, detailed logs, linking, shader/program cleanup.
- `engine/include/glory/engine/vertex_buffer.hpp` — non-copyable VAO/VBO interface.
- `engine/src/vertex_buffer.cpp` — upload interleaved vertices, configure position/color attributes, draw triangle.
- `engine/include/glory/engine/triangle_renderer.hpp` — renderer interface and RAII-owned resources.
- `engine/src/triangle_renderer.cpp` — GLSL 450 shaders and frame rendering.
- `engine/src/application.cpp` — create/use/destroy renderer within the GL context lifetime; preserve input and fixed updates.
- `CMakeLists.txt` — compile engine renderer sources while retaining input tests and user DLL-runtime copying.
- `.ai/specs/first-triangle-shader/test-report.md` — actual verification results.
- `.ai/specs/first-triangle-shader/review.md` — independent code review.
- `PLAN.md` — tick only this item after PASS and review.

## 4. Risks and Mitigations

- **OpenGL 1.1 import on Windows:** call only dynamically loaded procedure pointers for rendering; do not invoke linked legacy `gl*` entry points.
- **Function-pointer ABI/loading failure:** use SDL/OpenGL procedure typedefs and validate every required address before rendering; report the unresolved symbol.
- **Partial initialization leaks:** use local shader guards and constructor failure cleanup; keep renderer objects scoped so they destruct before `SDL_GL_DestroyContext`.
- **Shader driver diagnostics:** query compile/link status and info-log length, then include stage and driver log in exceptions.
- **Regression to input/game loop:** retain existing `InputSystem`, fixed timestep, callback signature, CLI, smoke tick count, and `SDL_GL_SwapWindow` behavior.
- **Build configuration regression:** add only engine source files and preserve all existing CMake logic, including runtime DLL copies and CTest registrations.

## 5. Acceptance Criteria

- The engine loads all OpenGL procedures it calls through SDL3; the game layer has no OpenGL include or `gl*` call.
- Valid GLSL 450 vertex/fragment shaders compile/link; failures report shader/program logs and clean up created objects.
- A VAO/VBO stores three position/color vertices and renders them with `glDrawArrays(GL_TRIANGLES, 0, 3)`.
- Program, shader, buffer, and vertex-array resources are released before the GL context on normal shutdown and renderer initialization failure.
- Existing build succeeds; all existing CTest tests pass; `glory_client.exe --smoke-test` completes exactly 120 ticks and exits successfully.
- Existing input/fixed timestep behavior and CMake runtime DLL-copy logic remain present; only then may `PLAN.md` be ticked.
