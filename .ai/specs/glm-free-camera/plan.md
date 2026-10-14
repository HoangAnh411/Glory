# GLM and Free-Fly Camera Plan

## 1. Objective

Integrate GLM as a reproducible CMake dependency and add a context-independent free-fly camera in `engine/`. Update the triangle renderer to transform world-space position/color vertices using a view-projection uniform and depth testing. Camera motion must consume existing platform-neutral input frames on fixed updates, and camera math must be unit-testable without creating SDL windows or GL contexts.

## 2. Tasks

1. Fetch GLM 1.0.1 through CMake FetchContent and link `glm::glm` into the engine and camera test.
2. Extend the shared input control contract and SDL translator with left/right Control keys while preserving existing mappings.
3. Implement `FreeCamera` using GLM vectors/matrices: position, yaw, pitch, FOV, aspect ratio, near/far planes, view/projection/view-projection matrices, clamped mouse-look, and normalized W/S/A/D/Space/Ctrl movement scaled by fixed delta.
4. Extend the engine GL function table to load `glGetUniformLocation`, `glUniformMatrix4fv`, and `glEnable` dynamically.
5. Add a matrix uniform setter to `ShaderProgram`; update the vertex shader to apply `u_ViewProjection`; enable depth testing and pass the camera matrix through `TriangleRenderer`.
6. Integrate aspect updates from drawable size and camera input updates inside the fixed-timestep callback without changing game callback/input semantics or the 120-tick smoke contract.
7. Add CTest camera unit coverage for view matrix, perspective/aspect, pitch clamping, mouse look, and movement using only CPU-side camera code.
8. Run build, all CTest tests, and the SDL/OpenGL smoke test; record actual results.
9. Review math/render separation, GL loader and uniform behavior, integration, cleanup, and regression constraints; record the review.

## 3. Files to Create or Modify

- `CMakeLists.txt` — fetch/link GLM, add camera source and camera test while preserving current CMake DLL/test logic.
- `shared/include/glory/input/input_frame.hpp` — add Control key identifiers.
- `engine/src/input_system.cpp` — translate SDL Control scancodes.
- `engine/include/glory/engine/camera.hpp` — context-independent camera interface and parameters.
- `engine/src/camera.cpp` — GLM camera math and input-driven movement/look.
- `engine/include/glory/engine/gl_api.hpp` — declare uniform and depth-test procedure pointers.
- `engine/src/gl_api.cpp` — load the new OpenGL procedures.
- `engine/include/glory/engine/shader_program.hpp` — expose matrix uniform update.
- `engine/src/shader_program.cpp` — cache/validate uniform location and upload matrices.
- `engine/include/glory/engine/triangle_renderer.hpp` — accept the camera view-projection matrix.
- `engine/src/triangle_renderer.cpp` — apply `u_ViewProjection` and enable depth testing.
- `engine/src/application.cpp` — update camera at fixed ticks and aspect ratio from drawable dimensions.
- `tests/camera_tests.cpp` — GL/SDL-window-free camera math and movement tests.
- `.ai/specs/glm-free-camera/test-report.md` — real test results.
- `.ai/specs/glm-free-camera/review.md` — reviewer findings.
- `PLAN.md` — tick this task only after tests and review pass.

## 4. Risks and Mitigations

- **Dependency availability:** use the requested stable GLM 1.0.1 FetchContent tag; no system-installed GLM is required.
- **Platform coupling in camera:** include only GLM and shared input types in camera files; no SDL or OpenGL headers/context usage.
- **Unstable mouse-look:** apply sensitivity to relative mouse deltas and clamp pitch to `[-89, 89]` degrees.
- **Diagonal movement speed:** normalize the combined movement vector before applying speed and fixed delta.
- **Invalid projection parameters:** reject non-positive/non-finite aspect ratios and invalid FOV/near/far configuration.
- **Matrix convention mismatch:** use GLM's OpenGL `perspective` and `value_ptr` with `GL_FALSE` transpose; shader multiplies `u_ViewProjection * vec4(position, 1.0)`.
- **Regression risk:** preserve fixed-timestep callback ordering, all existing controls, remapping API, CTest registrations, renderer RAII and DLL-copy logic.

## 5. Acceptance Criteria

- CMake fetches GLM 1.0.1 and the project builds without requiring a system GLM install.
- Camera math and movement code has no SDL/OpenGL context dependency and has unit tests registered with CTest.
- View, projection, and view-projection matrices are valid; aspect ratio updates projection; pitch stays clamped; mouse and keyboard input move the camera with fixed delta.
- Vertex shader applies the uploaded view-projection matrix; engine loads/uses uniform functions and enables depth testing through procedure pointers.
- Existing fixed timestep and input mapping tests pass, the application smoke test exits after exactly 120 fixed ticks, and renderer RAII/context order remains intact.
- The requested `test-report.md` and three-section `review.md` are written; tick `PLAN.md` only after PASS and no required review fixes.
