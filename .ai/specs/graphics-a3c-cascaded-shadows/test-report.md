# A3c Test Report

- Build: `cmake --build build` — PASS.
- CTest: `ctest --test-dir build --output-on-failure` — PASS, 37/37.
- Smoke: `build/glory_client.exe --smoke-test-1080p` — PASS; OpenGL 4.5, 1920x1080, HDR framebuffer complete with resize recreations, 120 fixed ticks.
- Runtime shader validation: shared cascade PBR shader and preset sky shader compiled/linked.
- CPU tests: 3/4 cascade split endpoint/order, invalid split rejection, light-space matrix finiteness, sub-texel snap stability, conservative clip sphere visibility, and fog finite/distance/height behavior passed.
- GPU final High timings (ms): cascade0 0.119, cascade1 0.153, cascade2 0.135, cascade3 0.057, boss/remote 0.007, scene 0.611, sky 0.018, particle 0.001, tonemap 0.069, total 1.243.
- Performance target: PASS on the smoke workload; this is not a representative combat scene. See `docs/perf.md`.
