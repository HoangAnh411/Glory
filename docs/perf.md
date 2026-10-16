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

## A3a/A3b Smoke Comparison (Before A3c)

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

These historical measurements precede A3c and are superseded by the shared-atlas timings below. At that point, actor-owned depth draws were included in particle/overlay and cascades were not yet connected.

## A3c Shared CSM High Smoke

- Date: 2026-10-16
- Build: CMake/Ninja, `build/glory_client.exe --smoke-test-1080p`
- Drawable: 1920x1080; NVIDIA GeForce RTX 3050 Ti Laptop GPU, driver 617.42, OpenGL 4.5
- Quality: 4 cascades, 2048 pixels per atlas tile, 16-tap High PCF
- Sampling: asynchronous GPU queries, 240 completed frames
- Workload: smoke scene with shared atlas. This is not a representative combat scene.

| GPU pass | Average time (ms) |
| --- | ---: |
| Shadow cascade 0 | 0.119 |
| Shadow cascade 1 | 0.153 |
| Shadow cascade 2 | 0.135 |
| Shadow cascade 3 | 0.057 |
| Boss/remote shadow work (timestamp sum) | 0.007 |
| Opaque scene | 0.611 |
| Sky | 0.018 |
| Particle/overlay | 0.001 |
| Tonemap | 0.069 |
| Total GPU frame | 1.243 |

The final measurement uses the corrected camera-depth interpolation for cascade frusta. It is below the 3 ms target on this smoke workload; it is not a representative combat scene. Earlier tuning runs used a faulty cascade frustum interpolation and are not comparable. Cascade shadow work and actor timing are now separated from particle/overlay.
