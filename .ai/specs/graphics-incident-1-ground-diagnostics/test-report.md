# Test report: ground diagnostics

- `cmake --build build` — PASS. GLSL runtime compilation was also exercised by smoke.
- `ctest --test-dir build --output-on-failure` — PASS, 37/37.
- `build\glory_client.exe --smoke-test-1080p` — PASS; NVIDIA GeForce RTX 3050 Ti, 1920x1080, HDR framebuffer complete, 120 fixed ticks.
- GPU frame average: 1.490 ms. Per-pass values are recorded in `docs/perf.md`.
- Visual diagnosis limitation: automated smoke does not expose the F3 debug view to a screenshot capture path, so it cannot establish which view reproduces the reported black ground.
