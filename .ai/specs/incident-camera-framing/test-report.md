# Test report: camera framing

- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 37/37.
- `build/glory_client.exe --smoke-test-1080p` — PASS at 1920x1080, 120 fixed ticks; GPU frame 1.317 ms on RTX 3050 Ti.
- `build/glory_client.exe --capture` — PASS, six updated PNG/JSON pairs.
- Measured player mesh NDC center: center/boss `(-0.299,-0.244)`, pillar `(-0.192,-0.142)`, wall `(-0.392,-0.346)`, sun `(-0.252,-0.213)`, away `(-0.193,-0.143)`, boss-near `(-0.191,-0.150)`.
- Arm/blocker measurements: center/boss `3.300 m`, `lamp_wooden_lantern`; pillar `5.000 m`, none; wall `2.500 m`, `terrain_cliff_rock`; toward-sun `3.652 m`, `lamp_wooden_lantern`; away and boss-near `5.000 m`, none.
- Visual review of `captures/center_boss.png` still shows an oversized model cropped at the top and right; projected local AABB telemetry does not agree with the visible actor. Camera fix is therefore not accepted as complete; the discrepancy requires further tracing.
