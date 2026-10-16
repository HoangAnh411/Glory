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

## Graphics Incident 1 Diagnostic Smoke

RTX 3050 Ti Laptop GPU, 1920x1080, HDR on, 240-frame GPU query average after adding shader diagnostic selection and finite guards:

| Pass | Time (ms) |
|---|---:|
| Shadow cascades 0/1/2/3 | 0.112 / 0.144 / 0.135 / 0.054 |
| Boss/remote shadow | 0.008 |
| Scene | 0.903 |
| Sky | 0.011 |
| Particle/overlay | 0.001 |
| Tonemap | 0.052 |
| Total GPU frame | 1.490 |

This is a smoke measurement, not a before/after visual diagnosis. No screenshot capture was available in the automated run.

Camera-collision iteration smoke on the same RTX 3050 Ti Laptop GPU at 1920x1080 measured 1.488 ms total GPU frame (shadow cascades 0.111/0.141/0.127/0.051 ms; boss/remote 0.006 ms; scene 0.907 ms; sky 0.011 ms; particle/overlay 0.001 ms; tonemap 0.065 ms). Timing variance is expected between short smoke runs.

Neutral-color iteration smoke on the same GPU at 1920x1080 measured 2.309 ms total GPU frame (shadow cascades 0.101/0.147/0.131/0.263 ms; boss/remote 0.006 ms; scene 1.477 ms; sky 0.010 ms; particle/overlay 0.001 ms; tonemap 0.067 ms). This remains below the 3 ms target for the smoke scene.

After adding the linear-factor regression fixture, a repeat smoke measured 1.617 ms total (cascades 0.076/0.115/0.123/0.080 ms; boss/remote 0.006 ms; scene 1.077 ms; sky 0.002 ms; particle/overlay 0.001 ms; tonemap 0.051 ms). Short-run GPU averages vary; both runs were below 3 ms.

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
