# Test report: incident capture and camera telemetry

- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 37/37.
- `build\glory_client.exe --smoke-test-1080p` — PASS; OpenGL 4.5, 1920x1080, 120 fixed ticks; GPU frame 1.369 ms.
- `build\glory_client.exe --capture` — PASS; generated six 1920x1080 PNGs and JSON files under `captures/`.
- Camera measurements: center `(actual arm 5.000 m, NDC -0.0895/-0.1254, no blocker)`; pillar `(5.000 m, -0.0895/-0.1254, no blocker)`; wall `(2.500 m, -0.1816/-0.2544, terrain_cliff_rock)`; toward sun `(5.000 m, -0.0895/-0.1254, no blocker)`; away from sun `(4.110 m, -0.1093/-0.1531, signpost_wooden)`; boss near `(5.000 m, -0.0895/-0.1254, no blocker)`.
- All six images were opened. They reproduce the near-black ground and show the player model clipped at the right edge in several views, despite the projected player anchor being near the center. This disagreement means the current point-NDC metric is not yet a valid measurement of the visible mesh bounds; it will be corrected during camera investigation.
