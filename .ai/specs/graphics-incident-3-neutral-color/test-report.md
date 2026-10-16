# Test report: neutral color

- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 37/37. Added a regression fixture confirming glTF baseColorFactor values remain linear (0.5/0.25/0.75).
- `build\glory_client.exe --smoke-test-1080p` — PASS; 1920x1080, HDR framebuffer complete, 120 fixed ticks.
- RTX 3050 Ti GPU frame: 1.617 ms. Per-pass values are in `docs/perf.md`.
- Visual confirmation of the color balance requires interactive screenshots; smoke does not capture preset images.
