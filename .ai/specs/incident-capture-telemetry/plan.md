# Incident capture and camera telemetry

## Objective
Add live F3 camera/player measurements and a deterministic six-view `--capture` mode that saves PNG screenshots plus JSON camera data under `captures/`.

## Tasks
1. Expose desired/actual arm, signed applied shoulder offset, lateral spring-arm displacement, blocker identity, lock state, player NDC, and alive/dead state in F3.
2. Add a capture-only camera pose API and six named stable scene setups.
3. Read the rendered back buffer, save PNGs, and serialize the same camera telemetry to JSON.
4. Build, CTest, run smoke and `--capture`; inspect all six generated images and record measured evidence.

## Files
- `engine/include/glory/engine/application.hpp`
- `engine/src/application.cpp`
- `engine/include/glory/engine/third_person_camera.hpp`
- `engine/include/glory/engine/debug_panel.hpp`
- `engine/src/debug_panel.cpp`
- `engine/include/glory/engine/gl_api.hpp`
- `engine/src/gl_api.cpp`
- `engine/src/stb_image_write_impl.cpp`
- `game/src/main.cpp`
- `CMakeLists.txt`
- `.ai/specs/incident-capture-telemetry/`

## Risks
- Pixel readback requires the default back buffer and vertical row flipping.
- The six capture positions are diagnostic scenarios and must not change normal gameplay.

## Acceptance
- F3 shows all requested live fields and accurately reports current state.
- `--capture` writes six 1920x1080 PNGs and corresponding JSON files.
- The generated images are opened and reviewed; measured camera values are recorded.
- Build, CTest, and smoke pass; no game simulation changes are made.
