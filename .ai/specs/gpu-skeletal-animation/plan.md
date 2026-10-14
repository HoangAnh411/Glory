# GPU Skeletal Animation Plan

## 1. Objective

Add a minimal but complete glTF skeletal-animation path: parse skin/joint hierarchy, inverse-bind matrices, vertex joint/weight attributes and animation channels; evaluate local/global poses on CPU using GLM interpolation; upload the skin palette to a GPU-skinned vertex shader; and provide playback/scrubbing controls in the engine DebugPanel. Preserve static-model rendering, lighting, fixed timestep and existing tests/smoke behavior.

## 2. Tasks

1. Define CPU animation data types (`SkeletonNode`, skin palette joint mapping, TRS animation keys/channels/clips) and an `Animator` that samples LINEAR/STEP channels, uses vector interpolation and quaternion slerp, computes hierarchical globals, and outputs `global * inverseBind` palettes.
2. Extend `GltfAsset` parsing for skin nodes, all node parents/TRS, inverse binds, glTF animation sampler/channel data, and `JOINTS_0`/`WEIGHTS_0`; validate attributes, indices, key times, and a bounded GPU joint count. Preserve static assets with default influence values.
3. Extend Mesh interleaved data/VAO setup for integer joint indices and float weights.
4. Extend shader uniforms and ModelRenderer for a bounded joint matrix palette and skin-enabled flag; deform position and normal before the existing Blinn-Phong shader, while static meshes take an identity/no-skin path.
5. Add Model/Animator playback state and DebugPanel clip selection/display, Play/Pause and timeline scrub controls.
6. Add a small animated glTF sample and CPU-only CTest coverage for parser output, interpolation, hierarchy and palette computation.
7. Run prescribed build, all CTest tests and 120-tick smoke test; record Tester/Reviewer reports and tick the roadmap item only after PASS.

## 3. Files to Create or Modify

- `engine/include/glory/engine/animation.hpp`, `engine/src/animation.cpp` — CPU skeleton, clip, interpolation, pose and playback.
- `engine/include/glory/engine/gltf_asset.hpp`, `engine/src/gltf_asset.cpp` — parsed joint/weight, node hierarchy, inverse binds and animation clips.
- `engine/include/glory/engine/mesh.hpp`, `engine/src/mesh.cpp` — joint/weight vertex attributes and integer VAO layout.
- `engine/include/glory/engine/gl_api.hpp`, `engine/src/gl_api.cpp` — integer vertex attribute pointer if needed.
- `engine/include/glory/engine/shader_program.hpp`, `engine/src/shader_program.cpp` — skin flag and joint palette uniforms.
- `engine/include/glory/engine/model.hpp`, `engine/src/model.cpp` — animation playback and palette ownership.
- `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp` — GPU skin shader and per-frame palette upload.
- `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp` — animation play/pause/scrub/clip diagnostics.
- `engine/src/application.cpp` — advance model animation by frame delta and feed controls.
- `assets/models/starter_crate.gltf`, `assets/models/starter_crate.bin` — skinned animated sample retaining existing texture.
- `tests/skeletal_animation_tests.cpp`, `tests/asset_loader_tests.cpp`, `CMakeLists.txt` — CPU tests and CTest registration.
- `.ai/specs/gpu-skeletal-animation/test-report.md`, `review.md`, `PLAN.md` — role reports and final checkbox.

## 4. Risks and Mitigations

- **glTF node order is not guaranteed topological:** resolve parents by node identity and evaluate globals recursively with cycle/state checks.
- **JOINTS values index skin joint order, not glTF node order:** keep an explicit palette-slot-to-skeleton-node mapping and validate every vertex influence against it.
- **Malformed weights/accessors:** validate accessor types/counts, finite nonnegative weights, normalize nonzero sums, reject invalid indices.
- **Uniform capacity:** cap palette size at 64 joints and reject unsupported larger skins with a clear message.
- **Non-linear interpolation modes:** support glTF LINEAR and STEP; reject CUBICSPLINE explicitly until implemented.
- **Static model regressions/resource lifetime:** use zero/identity influence defaults and `u_HasSkin`; retain RAII GPU owners and ensure model is destroyed before GL context.

## 5. Acceptance Criteria

- Parser extracts skeleton hierarchy, joint mapping, inverse bind matrices, vertex JOINTS/WEIGHTS and at least translation/rotation/scale animation data.
- CPU-only tests verify vector interpolation, quaternion slerp, parent-child global transform and skin palette calculation.
- GPU shader blends four joint matrices and skins positions/normals; static models remain supported without skin data.
- DebugPanel shows clip name and offers Play/Pause and timeline scrubbing; application advances animation using elapsed time.
- Animated glTF sample loads and renders; all five existing tests plus the new animation/parser tests pass; smoke exits cleanly after 120 ticks with no GL lifetime errors.
