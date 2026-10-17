# A2 Test Report

## Commands
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`
- `build/glory_client.exe --smoke-test-1080p`
- `git diff --check`

## Result: PASS
- Build completed successfully.
- All 37 CTest tests passed.
- 1080p smoke run created and checked the RGBA16F/depth framebuffer, resized it twice, restored the drawable size, and completed 120 fixed ticks.
- Timer query support was available. GPU timings are recorded in `docs/perf.md`.
- OpenGL renderer: NVIDIA GeForce RTX 3050 Ti Laptop GPU, driver 617.42, OpenGL 4.5.0.
- Confirmed SDL drawable size: 1920x1080. Latest 240-frame averages: shadow 0.106 ms, scene 0.315 ms, sky 0.010 ms, particle/overlay 0.045 ms, tonemap 0.071 ms, total GPU frame 0.582 ms.
- `git diff --check` completed without whitespace errors (Git reports only LF-to-CRLF notices).

## Shader and resize checks
- Application smoke launch compiled all runtime shaders and reported framebuffer completeness.
- HDR resize count was 3 on the smoke path: initial allocation, half-size, and restored drawable size.

## HDR Framing Regression Retest (2026-10-17)
- Root cause: `engine/src/render_pipeline.cpp:18` assigned `vUv = position * 0.5` to a fullscreen triangle whose clip-space positions span `(-1..3)`. Interpolation therefore sampled only the lower-left half of the HDR texture on both axes, enlarging and shifting that image quadrant. The corrected assignment is `vUv = position`.
- Pixel test: `--capture` places a magenta marker at the renderer root-joint world position, tonemaps, reads the default framebuffer, and fails if no magenta pixel is found within 2 px of the matrix-projected location. Before the shader fix, HDR source read `(255,0,255)` while the expected default-framebuffer pixel read `(143,141,137)`; the nearest magenta was hundreds of pixels away. The preserved failure image is `captures/hdr-marker-failure.png`.
- After fix: six 1080p HDR and six `--capture-no-hdr` images were generated. Five angles place the root joint in-frame and pass the marker pixel check; marker expected/detected coordinates match. The wall-obstruction view projects the root joint outside the image, so its JSON correctly reports `marker_in_frame: false` and does not claim a pixel pass. Paired player placement differs by at most 0.26 px horizontally and 0.24 px vertically across the six views.
- Resolution runs: six captures were generated at 1280x720 (`--capture-720p`) and at 1920x1080 with `SDL_WINDOW_HIGH_PIXEL_DENSITY` (`--capture-high-dpi`); in-frame markers pass at both sizes. The latter reports SDL points/pixels `1920x1080` / `1920x1080`, DPI `1x1`; Windows 150% desktop scaling was unavailable, so a non-1x OS scale is not verified here.
- Diagnostics: at 1080p SDL points, drawable pixels, HDR target, and scene/sky/particle/tonemap viewports are all `1920x1080`; scene projection aspect is derived from drawable pixels. Scissor is disabled for these passes. Shadow atlas viewports are the expected 2048x2048 tile rectangles. HDR and bypass use the same view/projection matrices. H1, H3, and H4 were not observed; H2 was reproduced and fixed.
- Retest commands: `cmake --build build` PASS; `ctest --test-dir build --output-on-failure` PASS (37/37); `build/glory_client.exe --smoke-test-1080p` PASS, framebuffer recreated three times. GPU average: shadow cascades 0.065/0.101/0.120/0.065 ms, boss/remote 0.005 ms, scene 0.918 ms, sky 0.001 ms, particle/overlay 0.001 ms, tonemap 0.075 ms, total 1.428 ms.
- Visual review: `captures/center_boss.png` and `captures/center_boss_no_hdr.png` show matching character placement and an uncropped head. The wall-obstruction capture keeps the pelvis outside frame in both paths; this is existing camera framing behavior, not an HDR discrepancy, and remains unresolved in this scoped fix.
