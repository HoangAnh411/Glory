# A3: Cascaded Lighting and Sunset Atmosphere

## 1. Objective
Replace the single shadow/light setup with stable cascades, shared character shadow participation, a non-black ambient floor, character fill/rim, and a configurable fantasy sunset sky/fog. Keep rendering changes inside the engine and preserve gameplay state.

## 2. Tasks
### A3a commit: `feat(render): add cascaded shadows and ambient lighting`
1. Add CPU cascade split and snap math with unit tests. (Implemented as preparation only.)
2. Replace the single depth target with configurable cascades, PCF quality, blending and debug. (Not implemented; runtime remains single-map.) Ambient and character fill/rim are implemented.
3. Route actors through shared shadow resources. (Actors render their own depth pass and sample their own map; maps are not shared with player/world.)
4. Add F3 controls for shadows, cascade count/splits/far distance, map size/sample quality, cascade debug, ambient, fill/rim, sun direction/color/intensity.
5. Build, test, run smoke, collect timer-query readings, review and commit A3a.

### A3b commit: `feat(render): add sunset sky and height fog`
1. Replace the plain sky gradient with layered sunset colors and a soft solar disc/glow.
2. Add distance exponential-squared and height fog with finite-safe math. (Fog color is a fixed warm/cool blend, not sampled per view direction.)
3. Add F3 presets Sunset/Sunrise/Noon/Old and ambient, fill/rim, fog, and sky toggles. (Cascade controls remain outstanding.)
4. Extend tests/smoke, collect 1080p per-pass timings, update performance notes and roadmap, review and commit A3b.

## 3. Files to Touch
- `engine/include/glory/engine/shadow_math.hpp`, `engine/src/shadow_math.cpp`, `tests/shadow_tests.cpp`, `CMakeLists.txt`
- `engine/include/glory/engine/shadow_map.hpp`, `engine/src/shadow_map.cpp`
- `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp`
- `engine/include/glory/engine/render_pipeline.hpp`, `engine/src/render_pipeline.cpp`
- `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp`
- `engine/src/application.cpp`
- A3 spec reports, `docs/perf.md`, `PLAN.md`

## 4. Risks & Mitigations
- Separate `ModelRenderer` objects currently own separate shadow resources. A3a must explicitly pass actor renderer/model references to the primary scene renderer so actor bodies are drawn into the primary cascade maps and sample those same maps.
- Shader changes are embedded GLSL, so runtime smoke validates compilation/linking in addition to CPU unit tests.
- Cascades and PCF increase GPU work; expose map size/sample count and gather real timer-query results at 1080p.
- Smoke mode may not represent combat density; report the measured workload and any budget miss without inventing values.

## 5. Acceptance Criteria
- Boss and remote body models cast and receive the same cascaded shadows as the player.
- Cascade splits are monotonic, finite, and blended between uniform/log distributions; snapped matrices are stable to sub-texel camera movement.
- Hemisphere minimum prevents zero ambient; character fill/rim is isolated to player/boss/remotes.
- F3 exposes requested controls and independent A3b feature switches/presets.
- Sky and fog outputs remain finite for edge cases; near characters are not materially fogged.
- Each A3 slice independently builds, passes CTest and smoke, and is committed separately.
- Actual GPU timer results are recorded when available; A3 effects do not modify gameplay, hitboxes, or combos.
