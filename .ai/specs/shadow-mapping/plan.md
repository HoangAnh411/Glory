# Shadow Mapping Plan

## 1. Objective

Add real-time directional shadow mapping with a depth-texture framebuffer and a two-pass renderer. Keep light-space matrix calculation in a CPU-only GLM module, render the skinned model and a ground receiver, and expose shadow enable/bias controls in DebugPanel. GPU resources must be destroyed while the OpenGL context is still alive.

## 2. Tasks

1. Add CPU-only `calculateLightSpaceMatrix` with validated light direction, center, orthographic half extent, and near/far planes. Test orientation and orthographic coverage without creating an OpenGL context.
2. Extend `GlApi` with framebuffer lifecycle, attachment, completeness, draw/read buffer, texture border color, and any needed framebuffer binding procedures.
3. Add RAII `ShadowMap` owning a depth texture and FBO, using a 2048x2048 depth component texture, linear filtering, clamp-to-border with a white border, and completeness validation.
4. Add an RAII ground-plane mesh below the sample model. Extend the main lighting shader with light-space position, 3x3 PCF, configurable bias, shadow enable, and a flat base-color path for the floor.
5. Add a depth-only shader pass that uses the same GPU skin palette as the lighting pass; render the model and floor to the shadow map, then restore framebuffer/viewport and render floor plus textured model with shadows.
6. Expose shadow enable and bias state through `ModelRenderer`; add DebugPanel checkbox and slider. Keep member construction/destruction order so all GL resources are released before context teardown.
7. Register CPU shadow-matrix tests in CTest, build, run all seven tests and the 120-tick smoke test, then write Tester/Reviewer reports and tick only Shadow mapping.

## 3. Files to Create / Modify

- `engine/include/glory/engine/gl_api.hpp`, `engine/src/gl_api.cpp` — OpenGL FBO procedures and texture border color procedure.
- `engine/include/glory/engine/shadow_math.hpp`, `engine/src/shadow_math.cpp` — CPU-only directional light view/projection matrix.
- `engine/include/glory/engine/shadow_map.hpp`, `engine/src/shadow_map.cpp` — RAII depth texture/FBO.
- `engine/include/glory/engine/ground_plane.hpp`, `engine/src/ground_plane.cpp` — RAII floor geometry used as shadow receiver and caster.
- `engine/include/glory/engine/shader_program.hpp`, `engine/src/shader_program.cpp` — light-space, shadow-map, bias, enable, and base-color uniforms.
- `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp` — depth and lighting passes, shared skinned palette, and shadow controls.
- `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp` — shadow checkbox and bias slider.
- `tests/shadow_tests.cpp`, `CMakeLists.txt` — CPU matrix tests and CTest registration.
- `.ai/specs/shadow-mapping/test-report.md`, `review.md`, `PLAN.md` — test/review records and final roadmap state.

## 4. Risks and Mitigations

- **Framebuffer completeness and driver support:** verify the FBO status during construction and throw a descriptive error after releasing partially-created resources.
- **GL resource lifetime:** declare `GlApi` before shadow map, shaders, model and floor so reverse destruction releases all GPU objects before the application destroys the context.
- **Skinned depth mismatch:** use a depth vertex shader with the same joint palette and skinning inputs as the main shader.
- **Light direction parallel to the up vector:** choose a stable alternate up axis in the CPU matrix helper and reject zero/non-finite direction vectors.
- **Shadow acne / outside-map darkness:** use slope-scaled bias, 3x3 PCF and a white clamp-to-border depth texture; return fully lit for projected coordinates outside the map.
- **Existing application state:** restore the default framebuffer and window viewport before the lighting pass; preserve the existing texture unit 0 and reserve unit 1 for the shadow map.

## 5. Acceptance Criteria

- `calculateLightSpaceMatrix` is independent of GL headers/context and rejects invalid extents/planes/direction. CPU tests verify light orientation, center mapping and orthographic bounds.
- `GlApi` loads all required framebuffer functions, and `ShadowMap` creates a complete depth-only FBO with white clamp-to-border and RAII cleanup.
- The shadow pass renders floor and model depth; the depth shader applies the model's current GPU skin palette.
- The lighting pass restores framebuffer and viewport, samples the depth texture on unit 1, applies 3x3 PCF with adjustable slope bias, preserves ambient illumination, and can disable shadows.
- The floor receives visible shadows and has an upward normal below the sample model.
- DebugPanel has an Enable Shadow checkbox and Shadow Bias slider.
- Seven CTest tests pass, including CPU-only shadow math tests; `glory_client.exe --smoke-test` exits successfully after 120 fixed ticks.
- Tester and Reviewer reports exist; only Shadow mapping is ticked after PASS.
