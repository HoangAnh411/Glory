# Review: World Collision Decoration Filtering

## 1. Required Fixes

- None found. The new movement participation flag defaults to blocking for compatibility, is checked by both movement/depenetration passes, and is propagated to camera obstruction and debug views.

## 2. Recommended

- Consider OBB/capsule trunk colliders later if the world gains denser tree placement; current yaw-transformed trunk AABBs can be wider than the authored local footprint.
- Add a content audit that requires each new prop type to declare whether it blocks movement, instead of relying on the current default.

## 3. Suggestions

- During playtesting, review whether benches, lamps, and carts should remain solid in narrow paths; their current blocking behavior is intentional in this change.

## Review Outcome

The plan is met. Decorative bush and terrain bounds remain visible in debug but no longer stop actors or the camera. Solid structures remain blockers, trees use authored trunk-sized bounds, and the server collision integration test targets a real fence. Build, 37/37 CTest, client smoke, bootstrap smoke, and live-update walk tests at 30/60/144 FPS pass. No gameplay hitbox, combo, or damage code changed.
