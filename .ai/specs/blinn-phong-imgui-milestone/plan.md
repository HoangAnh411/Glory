# Blinn-Phong and Debug UI Milestone Plan

## 1. Objective

Complete the final rendering/debug items in Phase 1 by shading the sample textured glTF model with configurable directional Blinn-Phong lighting and adding an engine-owned Dear ImGui diagnostics panel. Keep ImGui/OpenGL details inside `engine/`, leave game simulation independent, preserve fixed timestep/input mapping and the 120-tick smoke test, and destroy UI/model GPU resources before the GL context.

## 2. Tasks

1. Fetch Dear ImGui at pinned tag `v1.91.8`; compile its SDL3 and OpenGL3 official backend translation units against the existing SDL3 dependency.
2. Add engine `DirectionalLight` data and a CPU-only yaw/pitch-to-direction helper, with validation/unit tests registered in CTest.
3. Extend the model shader for world-space position and normal, view position, texture sampling, and ambient/diffuse/Blinn-Phong specular terms. Add all corresponding uniforms through the engine renderer.
4. Add an engine `DebugPanel` facade whose public header does not expose ImGui; display FPS, frame time, fixed ticks, camera position/yaw/pitch and editable light direction/color/strength/shininess.
5. Forward SDL events to the ImGui SDL3 backend; only suppress camera keyboard/mouse input while the matching ImGui capture flag is active. Keep game fixed updates unchanged.
6. Ensure initialization failures are handled and ImGui backends/context plus renderer/model resources are destroyed while the SDL OpenGL context remains current.
7. Update CMake and application integration without changing DLL copying, existing CTest targets or smoke-test tick semantics.
8. Run the prescribed build, full CTest suite and `glory_client.exe --smoke-test`; record exact results, review findings, and tick both feature items plus the Phase 1 milestone only after PASS.

## 3. Files to Create or Modify

- `CMakeLists.txt` — pin/fetch ImGui, compile official SDL3/OpenGL3 backends, add light test.
- `engine/include/glory/engine/directional_light.hpp`, `engine/src/directional_light.cpp` — light parameters and deterministic direction conversion.
- `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp` — engine-only ImGui facade, diagnostics, capture state, and controls.
- `engine/include/glory/engine/gl_api.hpp`, `engine/src/gl_api.cpp` — load integer/float/vector uniforms needed by the shader.
- `engine/include/glory/engine/shader_program.hpp`, `engine/src/shader_program.cpp` — typed uniform setters.
- `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp` — world-space Blinn-Phong shader and light/camera uniforms.
- `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp` — UI/render frame integration, filtered camera input, fixed-tick display, and lifetime ordering.
- `tests/directional_light_tests.cpp` — CPU-only direction/default parameter checks.
- `.ai/specs/blinn-phong-imgui-milestone/test-report.md`, `review.md` — Tester and Reviewer outputs.
- `PLAN.md` — tick two features and Phase 1 milestone after verification.

## 4. Risks and Mitigations

- **No upstream ImGui CMake project:** populate the pinned source via FetchContent and define a small local target with only core files and official SDL3/OpenGL3 backend files.
- **Input conflicts:** process all SDL events in both input and ImGui systems; filter only camera movement/mouse look according to ImGui capture flags, not game simulation.
- **Resource lifetime:** construct UI and renderer inside a scope whose destruction completes before context/window teardown; make DebugPanel clean up partially initialized backends.
- **Lighting coordinate mismatch:** use model/world-space positions and normals, world-space camera position, and a normalized directional-light vector; test CPU direction math separately.
- **Regression/smoke environment:** retain 120 fixed-tick accounting and run the actual Windows build, CTest, and executable smoke command.

## 5. Acceptance Criteria

- Dear ImGui v1.91.8 official SDL3 and OpenGL3 backends compile and initialize/shutdown safely.
- Debug panel displays FPS, frame time, tick count and camera position/yaw/pitch, and provides live controls for light yaw/pitch, color, ambient, diffuse, specular and shininess.
- The sample textured model uses a GLSL Blinn-Phong shader with ambient, diffuse and halfway-vector specular terms using the supplied directional light and camera position.
- UI mouse/keyboard capture suppresses corresponding camera input without stopping game fixed updates or breaking input mappings.
- A new context-free directional-light unit test passes, as do all existing CTest tests; smoke test completes 120 ticks and exits cleanly.
- UI/backend/model resources are destroyed before SDL destroys the GL context; build, tests and smoke test pass before all three roadmap checkboxes are ticked.
