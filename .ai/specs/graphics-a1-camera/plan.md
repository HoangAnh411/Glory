# A1: Third-Person Camera Upgrade

## 1. Objective
Replace the tight third-person framing with a shoulder camera using a 65-degree FOV, 5-meter arm, 0.5-meter right shoulder offset, and a chest-height look target. Add a collision-aware spring arm and retain the old third-person setup as an F3 comparison option. Keep the free camera on F1 and preserve all gameplay inputs and simulation behavior.

## 2. Tasks
1. Update camera configuration and math for shoulder offset, configurable FOV, and smoothed arm distance.
2. Resolve camera obstruction against static world props and the arena boundary without modifying gameplay collision or hitboxes.
3. Keep both upgraded and legacy third-person cameras available; expose an F3 selector while preserving F1 free-camera switching.
4. Add/update camera checks for defaults, shoulder framing, distance clamping, obstruction response, and valid view/projection matrices.
5. Build the project and run the camera test and application smoke test.

## 3. Files to Touch
- `engine/include/glory/engine/third_person_camera.hpp`
- `engine/include/glory/engine/model_renderer.hpp`
- `engine/src/model_renderer.cpp`
- `engine/include/glory/engine/debug_panel.hpp`
- `engine/src/debug_panel.cpp`
- `engine/src/application.cpp`
- `tests/third_person_camera_tests.cpp`
- `PLAN.md` (only mark the camera item complete after validation)

## 4. Risks & Mitigations
- Static props do not expose collision meshes. Use conservative scaled bounds for camera-only obstruction checks; do not feed them into gameplay physics.
- Shoulder offset can make lookAt basis unstable at extreme pitch. Derive the lateral axis from yaw and retain existing pitch clamps.
- F3 changes must not be overwritten by F1/gameplay input. Keep separate free-camera and enhanced/legacy third-person state.

## 5. Acceptance Criteria
- Enhanced camera defaults: 5.0 m arm, 65-degree FOV, 0.5 m right shoulder offset, and 1.4 m chest-height aim point.
- Spring arm shortens smoothly before reaching a static prop or arena boundary and does not place the camera beyond that obstruction.
- F3 switches between enhanced and legacy third-person views; F1 still switches third-person/free camera.
- Camera changes do not alter simulation, hitboxes, or combo behavior.
- CMake build succeeds, camera tests pass, and the configured smoke run starts and exits successfully.
- No A1 roadmap item is checked until all validation passes.

## Implementation Notes
- Static props use conservative camera-only spherical bounds; the camera also stays inside the visible arena perimeter.
- Build, camera test, and 120-tick SDL/OpenGL smoke run passed. Visual capture remains for in-game confirmation.
