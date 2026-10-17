# C3 Test Report

## Commands
- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 37/37 tests.
- `build/glory_client.exe --smoke-test-1080p` — PASS, 120 fixed ticks; HDR target resize and render complete at 1920x1080.
- `build/glory_client.exe --capture-colliders` — PASS; produced six PNG/JSON captures.

## Focused checks
- Boss chase against a static AABB remains outside the wall; overlapping boss spawn resolves to a non-overlapping side.
- Overlapping player/boss capsules receive opposite horizontal displacement; non-overlapping capsules remain stationary.
- Camera ray stops at a large shared AABB at 4.8 m and reports its name; a prop with largest dimension below 1.2 m is ignored.
- Existing collision regression suite remains green.

## Capture inspection
- Opened `captures/center_boss_colliders.png` and `captures/wall_colliders.png`.
- Center view shows loaded arena prop bounds around the play area. Wall view shows the boundary and nearby authored bounds; capture telemetry reports the camera stop and blocker where applicable.
- Six captures: `center_boss_colliders`, `pillar_colliders`, `wall_colliders`, `toward_sun_colliders`, `away_from_sun_colliders`, `boss_near_colliders`.

| Capture | Arm desired/actual | Collider reported | Player NDC |
|---|---:|---|---|
| center_boss | 5.00 / 5.00 m | none | (0.000, -0.339) |
| pillar | 5.00 / 5.00 m | none | (-0.020, -0.373) |
| wall | 5.00 / 2.00 m | terrain_cliff_rock | (0.021, -0.693) |
| toward_sun | 5.00 / 5.00 m | none | (0.017, -0.381) |
| away_from_sun | 5.00 / 4.215 m | signpost_wooden | (-0.025, -0.410) |
| boss_near | 5.00 / 5.00 m | none | (-0.007, -0.339) |

## Smoke GPU telemetry
- 1920x1080, RTX 3050 Ti Laptop GPU: shadow cascades 0.067/0.110/0.125/0.065 ms; boss/remote shadow 0.005 ms; scene 1.153 ms; sky 0.001 ms; particle overlay 0.001 ms; tonemap 0.070 ms; total 1.663 ms.
