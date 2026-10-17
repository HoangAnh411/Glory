# A2: HDR, Linear Color, Tonemapping, and GPU Timing

## 1. Objective
Render the complete 3D frame into a resize-safe RGBA16F target, convert color inputs to linear light, tone map to the default framebuffer with ACES, measure GPU work asynchronously, and let F3 compare against the direct legacy path. Identify and make the blue arena debug line opt-in.

## 2. Tasks
1. Add a shared render pipeline with RGBA16F color and depth attachments, resize validation, a fullscreen ACES pass, exposure control, and asynchronous query rings.
2. Render the player, ground, props, sky, particles, boss, and remote players into one target; render ImGui after tone mapping to the default framebuffer.
3. Use sRGB internal formats for color textures and skybox; linearize glTF factors, shader-authored colors, and particle colors; keep UI colors unchanged. Use explicit shader sRGB encoding for the direct comparison path and manual sRGB encoding after ACES for HDR.
4. Add independent GPU timings for shadow, opaque scene, sky, particles/overlays, tonemap, and total GPU frame. Poll results only after availability and expose recent samples to F3.
5. Add F3 controls/status/exposure/history, disable the blue arena boundary debug polyline by default and expose its own toggle, and extend smoke mode to exercise framebuffer creation and resize.
6. Build, run relevant tests and the smoke run, capture actual 1080p GPU timings when timer queries are supported, document results, review, and commit the A2 slice.

## 3. Files to Touch
- `engine/include/glory/engine/gl_api.hpp`, `engine/src/gl_api.cpp`
- New `engine/include/glory/engine/render_pipeline.hpp`, `engine/src/render_pipeline.cpp`
- `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp`
- `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp`
- `engine/src/application.cpp`, `game/src/main.cpp`, `CMakeLists.txt`
- `engine/src/texture2d.cpp`, `engine/src/gltf_asset.cpp`, `engine/src/model.cpp`
- `engine/src/skybox.cpp`, `engine/src/particle_renderer.cpp`
- `tests/` relevant renderer/color tests
- `PLAN.md`, `docs/perf.md`, `.ai/specs/graphics-a2-hdr-linear-tone/`

## 4. Risks & Mitigations
- Avoid nested `GL_TIME_ELAPSED` queries: use elapsed queries per pass and timestamp endpoints for total-frame duration, with a three-frame asynchronous ring.
- Preserve the legacy comparison path: render directly to framebuffer 0 and explicitly encode linear shader output to sRGB; HDR mode performs ACES and manual sRGB encoding only in the fullscreen shader.
- Remote players and boss use separate renderer objects: pass the same frame pipeline through every overlay draw and preserve the HDR framebuffer binding.
- Headless/unsupported drivers may not expose timer queries: detect missing entry points and report timing as unavailable without blocking rendering.
- Resize zero dimensions are ignored until the drawable is valid; every recreated framebuffer is checked and errors identify dimensions/status.

## 5. Acceptance Criteria
- All 3D passes, including overlays and particles, are contained in RGBA16F plus depth when enabled; ImGui runs afterward on framebuffer 0.
- FBO resize/recreation passes completeness checks; smoke mode exercises at least one resize.
- ACES exposure defaults to 1.0 and is adjustable in F3; the bypass toggle displays active mode/exposure.
- Texture albedo and sky gradient use sRGB decoding; material and particle colors are linear in the lighting/compositing path; manual sRGB encoding happens once after ACES.
- F3 exposes per-pass and total GPU timings plus recent-frame plots, or a clear unsupported status.
- The blue arena perimeter line is identified and disabled by default behind an F3 toggle.
- Build, relevant tests, and smoke run pass. Record real 1080p timings in `docs/perf.md` if supported.
- No gameplay, hitbox, combo, MSAA/FXAA, or cascaded-shadow changes.

## HDR Framing Regression Addendum
- Record SDL point size, drawable pixel size/DPI scale, HDR attachment size, pass viewport/scissor, and the scene projection when enabling HDR and on resize.
- Reset viewport/scissor for each relevant render pass and use drawable pixel dimensions for target allocation and projection aspect.
- In `--capture`, draw a magenta marker at the renderer's root-joint world position and verify it with `glReadPixels` after tonemapping at the projected pixel. Fail capture when no marker is found within two pixels.
- Compare six fixed views with HDR on/off at 1080p and additional drawable sizes; keep camera roadmap status unchanged until visual and pixel checks pass.

## Implementation Notes
- Output uses manual sRGB conversion in the ACES fullscreen shader; `GL_FRAMEBUFFER_SRGB` remains disabled, so no second encoding path is active.
- Per-pass `GL_TIME_ELAPSED` and total-frame `GL_TIMESTAMP` queries use a nonblocking three-frame ring.
- Blue guide source: the radius-35 arena `AddPolyline` HUD overlay, formerly always drawn; now controlled in F3 and off by default.
- Build, 37 tests, resize smoke, and 1080p GPU timing completed successfully. See `test-report.md` and `docs/perf.md`.
