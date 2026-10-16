# Test report: camera collision

- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 37/37; camera test checks pitch clamp, arm minimum, orbit, and finite matrices.
- `build\glory_client.exe --smoke-test-1080p` — PASS; 1920x1080, HDR framebuffer complete, 120 fixed ticks.
- GPU frame average on RTX 3050 Ti: 1.488 ms; per-pass timings in `docs/perf.md`.
- Interactive blocker identity and collision behavior were not image-captured in automation.
