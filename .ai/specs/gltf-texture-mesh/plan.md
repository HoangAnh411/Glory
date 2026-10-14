# Textured glTF Mesh Plan

## 1. Objective

Implement the adjacent roadmap items as one engine asset/rendering slice: decode PNG/JPEG with `stb_image`, parse glTF/GLB mesh data with `cgltf`, upload an indexed textured mesh to OpenGL, and render a sample textured crate. Keep CPU asset parsing/decoding independently testable without a graphics context; keep `game/` limited to selecting the asset path and existing simulation/input logic.

## 2. Tasks

1. Add pinned CMake FetchContent dependencies for stb and cgltf; compile each single-header implementation in exactly one engine translation unit.
2. Add CPU-side `ImageData` loading from filesystem and memory with `stbi_image_free` ownership safety, forced RGBA output, dimensions validation, and actionable errors.
3. Add a CPU-side glTF asset parser using cgltf parse/load/validate/free RAII. Extract POSITION, NORMAL, TEXCOORD_0, indices, and base-color texture source; support external image URIs and GLB buffer-view images.
4. Extend `GlApi` for texture allocation/binding/upload/parameters/deletion/active unit, integer uniforms, and indexed drawing. Keep all GL calls routed through loaded pointers.
5. Add RAII `Texture2D`, indexed `Mesh` (VAO/VBO/EBO), and `Model` composition in `engine/`; GPU resources must be destroyed before the context.
6. Replace triangle-only shader/render data with position/normal/UV attributes, view-projection transform, sampled texture, sampler binding, and `glDrawElements`.
7. Let `game/` select the sample model path through application configuration; preserve its simulation and action-map responsibilities and do not expose graphics APIs there.
8. Add a repeatable sample crate `.gltf` plus texture `.png` under `assets/`.
9. Add CPU-only CTest coverage for valid/invalid image decoding and valid/invalid glTF parsing, including extracted vertex/index/texture metadata.
10. Build, run all CTest cases and the existing 120-tick smoke test; record real results. Review architecture, malformed input handling, GPU/CPU cleanup, and regressions.
11. Tick both roadmap items only after Tester PASS and Reviewer reports no required fixes.

## 3. Files to Create or Modify

- `CMakeLists.txt` — fetch stb/cgltf, add engine loader/model sources and CPU asset test; preserve GLM, existing tests, OpenGL and user DLL-copy logic.
- `engine/include/glory/engine/gl_api.hpp` and `engine/src/gl_api.cpp` — texture, sampler, indexed-draw procedure pointers.
- `engine/include/glory/engine/image_loader.hpp` and `engine/src/image_loader.cpp` — CPU decoded RGBA data API.
- `engine/src/stb_image_impl.cpp` — sole `STB_IMAGE_IMPLEMENTATION` translation unit.
- `engine/include/glory/engine/gltf_asset.hpp` and `engine/src/gltf_asset.cpp` — CPU mesh/image source data and cgltf parsing.
- `engine/src/cgltf_impl.cpp` — sole `CGLTF_IMPLEMENTATION` translation unit.
- `engine/include/glory/engine/texture2d.hpp` and `engine/src/texture2d.cpp` — RAII texture upload/binding.
- `engine/include/glory/engine/mesh.hpp` and `engine/src/mesh.cpp` — interleaved vertex and EBO ownership/draw.
- `engine/include/glory/engine/model.hpp` and `engine/src/model.cpp` — parsed asset plus GPU mesh/texture composition.
- `engine/include/glory/engine/shader_program.hpp` and `engine/src/shader_program.cpp` — sampler uniform support.
- `engine/include/glory/engine/model_renderer.hpp` and `engine/src/model_renderer.cpp` — textured position/normal/UV shader and model draw.
- `engine/include/glory/engine/application.hpp` and `engine/src/application.cpp` — model path config and scoped renderer lifetime.
- `engine/include/glory/engine/vertex_buffer.hpp`, `engine/src/vertex_buffer.cpp`, `engine/include/glory/engine/triangle_renderer.hpp`, `engine/src/triangle_renderer.cpp` — replace triangle-only geometry/render path with indexed model path.
- `game/src/main.cpp` — select the sample asset path without using graphics APIs.
- `assets/models/starter_crate.gltf` and `assets/textures/starter_crate.png` — sample textured 3D asset.
- `tests/asset_loader_tests.cpp` — context-free image and glTF tests.
- `.ai/specs/gltf-texture-mesh/test-report.md` and `review.md` — Tester/Reviewer artifacts.
- `PLAN.md` — tick the two requested adjacent items after acceptance.

## 4. Risks and Mitigations

- **Dependency drift:** pin cgltf to `v1.14` and stb to a resolved immutable commit in FetchContent.
- **Header implementation ODR violations:** define each implementation macro in one dedicated `.cpp` only.
- **Malformed or unsupported glTF data:** validate parse/load results and required attributes/material/indices; return errors instead of dereferencing absent accessors.
- **Index/accessor mismatch:** validate accessor dimensions/counts and every decoded index against the vertex count.
- **Image decode/upload failures:** keep decoded bytes under RAII/vector ownership, include stb diagnostics, and delete partially created GPU texture names.
- **GLB embedded texture lifetime:** copy buffer-view image bytes into owned CPU data before `cgltf_free`.
- **GPU cleanup ordering:** renderer/model live in a scope that exits before `SDL_GL_DestroyContext`; Mesh and Texture2D are non-copyable RAII owners.
- **Regression:** retain fixed timestep, camera, input mapping, shader uniform flow, smoke tick count, existing CTest entries and runtime DLL copy section.

## 5. Acceptance Criteria

- CMake fetches pinned stb/cgltf revisions without system packages, and both implementations are compiled once.
- CPU loaders handle the included sample PNG and glTF, reject invalid/malformed files safely, and expose positions, normals, UVs, indices and texture source for tests without a GL context.
- Engine uploads RGBA texture and indexed mesh through dynamically loaded OpenGL functions; GLSL uses position/normal/UV attributes, samples `u_Texture`, and model draw uses `glDrawElements`.
- Game code only configures the model asset path and continues simulation/input behavior; no GL or loader API is used from `game/`.
- CPU image/parser tests and all pre-existing CTest tests pass; smoke test exits after 120 ticks with resources destroyed before the GL context.
- Both requested roadmap checkboxes are ticked only after all role outputs are complete and PASS.
