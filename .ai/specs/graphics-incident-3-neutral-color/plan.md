# Graphics incident 3: neutral color baseline

## Objective
Set Neutral as startup lighting, reduce Sunset saturation/intensity, and remove incorrect sRGB decoding from glTF linear values.

## Tasks
1. Preserve glTF baseColorFactor, emissiveFactor, and COLOR_0 values as linear, as required by glTF.
2. Add a Neutral F3 preset and procedural sky colors; make Neutral the renderer and light default.
3. Reduce Sunset light saturation and diffuse intensity while keeping the preset selectable.
4. Build, test, and smoke; document affected sources and visual limits.

## Files
- `engine/src/gltf_asset.cpp`
- `engine/include/glory/engine/directional_light.hpp`
- `engine/include/glory/engine/model_renderer.hpp`
- `engine/src/model_renderer.cpp`
- `engine/src/skybox.cpp`
- `engine/src/debug_panel.cpp`
- `tests/asset_loader_tests.cpp`
- `PLAN.md`
- `docs/perf.md`
- `.ai/specs/graphics-incident-3-neutral-color/`

## Risks
- Existing authored values may have been chosen after seeing the incorrect conversion and may need visual retuning.
- The generated cubemap used for reflections remains its own static environment; the visible sky responds to the selected preset.

## Acceptance
- Startup and F3 select Neutral by default.
- Sunset uses a less saturated warm light and reduced diffuse strength.
- glTF linear material and vertex values are not passed through sRGB conversion a second time.
- Build, CTest, and 1080p smoke pass; gameplay is unaffected.
