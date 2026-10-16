# Review: camera collision

## Must fix
- No gameplay state or collision/hitbox code is modified.

## Recommended
- Current bounds use static glTF vertex extents; animated deformation and mesh node transforms are not represented. Camera side/up sliding for a blocker closer than 2.5 m is not yet implemented.
- The arena perimeter is still modeled as a circle.

## Suggestions
- Add world-space model bounds that include node transforms, and capture F3 blocker output in a close-wall/boss scene.

## Result
- Unit tests, build, and 1080p smoke pass. The hard 2.5 m minimum is enforced, so an unusually close obstruction may still be intersected pending slide/fade handling.
