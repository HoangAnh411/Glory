# A3a Test Report

- Build: `cmake --build build` — PASS.
- Tests: `ctest --test-dir build --output-on-failure` — PASS, 37/37.
- Smoke: `build/glory_client.exe --smoke-test-1080p` — PASS; OpenGL 4.5, 1920x1080 drawable, framebuffer resize checks passed, 120 fixed ticks.
- GPU timing from the smoke run: shadow 0.171 ms, scene 0.357 ms, sky 0.011 ms, particle/overlay 0.126 ms, tonemap 0.066 ms, total 0.762 ms.
- CPU checks cover blended cascade split monotonicity/end distance and texel-grid snap behavior.
- Limitation: runtime still uses one shadow map per renderer, not cascaded shadow maps. Boss/remotes now render a depth pass and sample their own map, but they do not share the player's map or cast onto ground/other actors. PCF remains the previous 3x3 kernel; cascade quality/map-size controls are not implemented.
