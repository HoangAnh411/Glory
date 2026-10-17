# Test Report: World Collision Decoration Filtering

## Results

- `cmake --build build`: PASS.
- `ctest --test-dir build --output-on-failure`: PASS, 37/37 tests.
- `build/glory_client.exe --smoke-test`: PASS; OpenGL 4.5 context, HDR target resize, 120 fixed ticks, and collision data startup passed. Runtime reported 76 prop colliders, arena center `(0,0)`, radius `34 m`, and noclip off.
- `build/glory_bootstrap.exe`: PASS (`Glory project scaffold ready`).
- `glory_client.exe --walk-test`, launched from `build/collision-decoration-validation`: PASS at 30/60/144 render FPS, with 19,200 live fixed updates per run.
- `git diff --check`: PASS.

## Regression Coverage

- Loaded data marks six bush props and all hill/cliff scenery bounds non-blocking.
- Solid benches and fences remain blocking.
- Tree props use narrow, authored trunk bounds rather than the canopy mesh bounds.
- CPU movement test crosses a non-blocking decorative bound and stops at a narrow trunk collider.
- Server integration test now approaches the authored farm fence, rather than relying on a broad hill AABB as an accidental blocker.

## Walk-Test Evidence

Walk traces are in `build/collision-decoration-validation/captures/walk-test-30.png`, `walk-test-60.png`, and `walk-test-144.png`, with matching JSON traces. The top-down PNG was opened and checked: blue outlines denote non-blocking decoration, green outlines denote physical blockers, and player/boss traces remain inside the arena boundary. These outputs were kept under `build/` so existing user captures were not overwritten.

## Limitations

- Tree trunk colliders are authored upright AABBs, not mesh-based capsules; at non-cardinal yaw, the world-space AABB is wider than the local trunk footprint.
- Benches, fences, lamps, carts, barrels, signs, wells, buildings, towers, and windmills remain solid and can still block movement as physical props.
- The live scripted walk test validates player and boss containment/collider overlap at its scripted routes; it does not exhaustively route through every prop instance.
