# Graphics incident 2: third-person camera collision

## Objective
Keep the third-person camera at least 2.5 m from its target, above ground, and prevent oversized prop bounds from pulling it into the player. Expose the active blocker in F3.

## Tasks
1. Clamp camera pitch to -10..55 degrees, effective arm to 2.5 m minimum, and camera height to 0.4 m.
2. Replace static-prop sphere intersection with ray vs model-derived local AABB transformed by prop yaw/scale; ignore props whose largest world-space dimension is below 1.2 m.
3. Expose blocker name and clear distance in F3, add camera regression assertions, and validate build/CTest/smoke.

## Files
- `engine/include/glory/engine/third_person_camera.hpp`
- `engine/include/glory/engine/model.hpp`
- `engine/src/model.cpp`
- `engine/include/glory/engine/model_renderer.hpp`
- `engine/src/model_renderer.cpp`
- `engine/src/debug_panel.cpp`
- `tests/third_person_camera_tests.cpp`
- `PLAN.md`
- `.ai/specs/graphics-incident-2-camera-collision/`

## Risks
- Model local bounds are static bind-pose bounds and do not follow animated deformation.
- Arena perimeter remains a circular boundary approximation.

## Acceptance
- Unit tests verify pitch and arm minimums.
- F3 reports the nearest blocker type and camera clearance.
- Build, CTest, and 1080p smoke pass without gameplay changes.
