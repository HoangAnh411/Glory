# Incident fix: camera framing

## Goal
Correct the clipped player framing while preserving gameplay and the minimum camera arm length.

## Evidence and implementation
- Use the already committed six-view captures and extend camera telemetry to represent projected model bounds, not only a guessed anchor point.
- Keep the spring arm on its target-to-camera line; report lateral displacement and the nearest blocker.
- Apply a deterministic rightward look-at composition offset so the player projects to the left-lower safe region.
- Fade the nearest camera-blocking static prop when collision clamps the arm.

## Files
- `engine/include/glory/engine/third_person_camera.hpp`
- `engine/include/glory/engine/model_renderer.hpp`
- `engine/src/model_renderer.cpp`
- `engine/include/glory/engine/debug_panel.hpp`
- `engine/src/debug_panel.cpp`
- `engine/src/application.cpp`
- `tests/third_person_camera_tests.cpp`

## Acceptance
- CPU test verifies unobstructed NDC framing and minimum 2.5 m arm.
- Captures include camera placement and projected player bounds.
- Build, CTest, 1080p smoke and six capture PNGs pass.
