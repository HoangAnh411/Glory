# A3c: Runtime Cascaded Shadows and Shared Scene Lighting

## Objective
Finish A3 by making RenderPipeline own one shadow atlas consumed by every opaque scene object, and by integrating stable cascades, PCF, debug controls, GPU timings, and sky-aligned fog.

## Tasks
1. Convert the single depth map to a resizeable 2x2 atlas (one tile per cascade), owned by the frame pipeline.
2. Compute blended log/uniform split distances and camera-frustum light matrices; snap each projection to its texel grid.
3. Render the ground, props and all actors into each active cascade with conservative per-cascade bounds culling.
4. Draw player, boss and remote models in the main opaque pass using one shared atlas and same cascade sampling shader.
5. Add rotated Vogel PCF with quality sample count, slope/normal bias, transition blending, cascade debug colors and F3 controls.
6. Add cascade and actor shadow GPU timing slots; remove actor shadow work from particle/overlay pass.
7. Share sky color evaluation between sky generation and per-pixel fog; synchronize lighting presets and protect near actors from fog.
8. Extend CPU tests and smoke checks; run build, CTest, 1080p smoke, record measured results, review, and commit.

## Files
`engine/include/glory/engine/shadow_map.hpp`, `engine/src/shadow_map.cpp`, `engine/include/glory/engine/shadow_math.hpp`, `engine/src/shadow_math.cpp`, `engine/include/glory/engine/render_pipeline.hpp`, `engine/src/render_pipeline.cpp`, `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp`, `engine/src/application.cpp`, `engine/src/debug_panel.cpp`, `engine/src/skybox.cpp`, `engine/include/glory/engine/directional_light.hpp`, `tests/shadow_tests.cpp`, `game/src/main.cpp`, `docs/perf.md`, `PLAN.md`, and this spec folder.

## Risks
- Atlas tile resolution doubles allocation dimensions; 4096 tiles require an 8192 square depth texture (256 MiB at 32-bit depth). Check max texture size and allocation completeness; expose 1024/2048/4096.
- Conservative actor/prop bounds may retain some off-cascade geometry but must not cull visible casters.
- Shadow acne/peter-panning require visual tuning; unit tests cannot validate them, so report the practical limitation without claiming visual proof absent a capture.

## Acceptance
- One atlas is allocated and sampled for all scene casters/receivers; no actor-owned shadow targets remain.
- Runtime renders 3 or 4 cascades, split and snap tests pass, every cascade uses rotated 4/8/16 PCF and blends its transition.
- F3 controls required settings and cascade visualization.
- GPU timings identify every cascade and actor shadow work independently of particle/overlay.
- Sky/fog colors follow look direction and near characters retain contrast.
- Build, all CTest tests and smoke pass; actual 1080p timer results are written to perf docs.
