# C1 Review

## Must Fix
- None found for the C1 scope.

## Should Fix
- Authored prop bounds are local AABBs transformed into world-axis-aligned AABBs. At arbitrary yaw these can be wider than the actual oriented object; C3 should share this same approximation with the camera and document remaining cases.
- The circular arena is currently data/debug geometry; enforcement belongs to C2.

## Suggestions
- Add named collider labels in the debug overlay when more world authoring tools are added.
- Use authored OBBs for large rotated houses where conservative AABB expansion meaningfully blocks paths.

## Result
- Collision loading remains in the `game` module and contains no OpenGL dependency. Debug projection is isolated in the engine debug panel.
- Noclip only disables the controller's world collision resolution; it does not alter combat hitboxes or combo state.
- Build, CTest 37/37, smoke, and collider screenshot inspection passed.
