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
