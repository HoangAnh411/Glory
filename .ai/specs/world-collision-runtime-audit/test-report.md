# Test Report: World Collision Runtime Audit

## Results

- `cmake --build build`: PASS.
- `ctest --test-dir build --output-on-failure`: PASS, 37/37 tests. The collision suite includes a regression at the boss's reported embedded position using the boss capsule (radius 0.55 m, height 2.2 m).
- `build/glory_client.exe --smoke-test`: PASS on OpenGL 4.5; HDR target resize completed; 120 fixed updates completed. Startup from `D:\Data\Glory\build` reported the repository asset root, 76 colliders, arena center `(0,0)`, radius `34`, and noclip off.
- `build/glory_client.exe --walk-test`: PASS for 30, 60, and 144 render FPS; each run executed 19,200 fixed updates and checked the live update callback's player and boss positions against world colliders and the arena boundary.
- Launched the same `glory_client.exe --walk-test` while the process working directory was `%TEMP%`: PASS at all three FPS values. The executable still loaded `D:\Data\Glory\assets\data\world_map.json` and `world_colliders.json`.
- `build/glory_client.exe --capture-colliders`: PASS; 14 PNG/JSON captures written. All eight `arena_edge_0` through `arena_edge_7` images were opened and visually checked with collision wireframes enabled.

## Bounds Audit

`captures/collision-runtime-audit.log` contains the complete per-instance table. It reports 76 rendered static instances, 76 matching colliders, 0 missing colliders, 0 mismatches above 0.15 m, and maximum AABB error `0.0000038147 m` (float precision). The AABBs use the same prop instance transform and mesh local bounds as rendering. Large terrain meshes remain AABB approximations.

## Walk Evidence

- `captures/walk-test-before-30.png` and `captures/walk-test-before-30.json` preserve a controlled run with the new depenetration pass temporarily disabled; it fails at boss step 1442 and `bush_flower_cluster#11`.
- `captures/walk-test-after-30.png` and `captures/walk-test-after-30.json` preserve the same scenario after restoring the fix; it passes.
- `captures/walk-test-30.png` and `.json`
- `captures/walk-test-60.png` and `.json`
- `captures/walk-test-144.png` and `.json`

Each JSON stores the per-fixed-step player and boss coordinates, arena boundary, pass status, and failure detail. The PNG plots their paths, the radius-34 safety boundary, and all collision boxes. All final traces stay inside the boundary.

## Initial Failure and Fix

Before the solver change, the runtime-path walk test failed at fixed step 1442 for the boss at `(8.249999, 0, 5.565706)`, reporting `bush_flower_cluster#11`. Adjacent/overlapping AABBs could undo a previous depenetration as the solver processed boxes in order. The solver now repeatedly removes the deepest overlap first. The failure-point regression and all three full walk runs pass afterward.

## Other Checks

- `glory_bootstrap.exe`: PASS.
- `git diff --check`: PASS after removing the existing whitespace at `game/src/main.cpp`'s live update callback.
- No gameplay damage, attack hitbox, or combo code was changed.

## Limitations

- Arena containment is still a circular safety boundary from `world_colliders.json`; eight edge captures verify the authored collision wireframes around that limit, but the map does not define a continuous physical perimeter wall.
- Terrain hills and cliffs use mesh-derived world AABBs, so their collision shape is a conservative box approximation.
- The screenshot supplied by the user is not available as a repository file, so the two pointed dark blue shapes cannot be conclusively attributed from that original image. The generated captures show the blockout character and its attached dark blue lower-body/weapon geometry; they do not establish which exact mesh part matches the reported pair.
