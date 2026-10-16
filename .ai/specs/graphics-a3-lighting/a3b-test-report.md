# A3b Test Report

- Build: `cmake --build build` — PASS.
- Tests: `ctest --test-dir build --output-on-failure` — PASS, 37/37.
- Smoke: `build/glory_client.exe --smoke-test-1080p` — PASS; 1920x1080 drawable, complete HDR framebuffer, three resize recreations, 120 fixed ticks.
- Runtime shader validation: PBR and generated skybox shaders compiled and linked on NVIDIA OpenGL 4.5.
- GPU: shadow 0.120 ms, scene 0.336 ms, sky 0.010 ms, particle/overlay 0.113 ms, tonemap 0.066 ms, total 0.677 ms.
- CPU tests cover fog finiteness and distance/height attenuation. Full CSM runtime coverage is absent because that feature is not implemented.
