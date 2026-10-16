# Renderer Performance Notes

## A2 HDR Baseline

- Date: 2026-10-16
- Build: CMake/Ninja, `glory_client.exe --smoke-test-1080p`
- Drawable: 1920x1080 (confirmed by SDL drawable pixel size)
- GPU: NVIDIA GeForce RTX 3050 Ti Laptop GPU
- Driver/OpenGL: NVIDIA 617.42 / OpenGL 4.5.0
- Sampling: asynchronous timer queries, average of the latest 240 completed GPU frames
- Render mode: RGBA16F scene target, ACES tonemap, exposure 1.0; no MSAA/FXAA

| GPU pass | Average time (ms) |
| --- | ---: |
| Shadow | 0.106 |
| Opaque scene | 0.315 |
| Sky | 0.010 |
| Particle and overlays | 0.045 |
| Tonemap | 0.071 |
| Total GPU frame (timestamp interval) | 0.582 |

Pass measurements use `GL_TIME_ELAPSED`; total GPU frame uses asynchronous `GL_TIMESTAMP` endpoints because elapsed queries cannot be nested. Results are available after a three-frame query ring and do not block while pending. These figures are a renderer smoke-test baseline with a lightweight scene, not a representative combat workload or an FPS guarantee.

## A3 Atmosphere Smoke Comparison

- Date: 2026-10-16
- Build: CMake/Ninja, `build/glory_client.exe --smoke-test-1080p`
- Drawable: 1920x1080
- GPU: NVIDIA GeForce RTX 3050 Ti Laptop GPU, driver 617.42, OpenGL 4.5
- Sampling: asynchronous GPU timer queries; 240-frame averages
- Workload: lightweight smoke scene; actor shadow draws are currently included in particle/overlay timing

| GPU pass | A2 baseline (ms) | Current (ms) |
| --- | ---: | ---: |
| Shadow | 0.106 | 0.120 |
| Opaque scene | 0.315 | 0.336 |
| Sky | 0.010 | 0.010 |
| Particle and overlays | 0.045 | 0.113 |
| Tonemap | 0.071 | 0.066 |
| Total GPU frame | 0.582 | 0.677 |

The measured total is below the requested 3 ms smoke-scene budget; this is not a representative combat workload. Per-cascade measurements are unavailable because cascaded shadow rendering is not connected yet. The overlay pass increased because boss/remote renderers now draw their own depth maps.
