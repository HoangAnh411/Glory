# Test report: black ground

- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 37/37.
- `build/glory_client.exe --smoke-test-1080p` — PASS at 1920x1080, 120 fixed ticks; GPU frame 1.420 ms on RTX 3050 Ti.
- `build/glory_client.exe --capture` — PASS, six views generated.
- Opened `captures/pillar.png`: the ground is visibly gray and receives contact shadows; the previous RGB-near-zero floor is gone without changing ambient strength.
- Root cause confirmed by shader/VAO interface: `fragmentSource` multiplies by `vColor.rgb`, while the ground VAO had no location-5 color attribute. Explicit white vertex colors restore the intended material albedo.
