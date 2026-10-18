# Review: World Collision Regression Fix

## 1. Must Fix

- None found.

## 2. Should Fix

- None found.

## 3. Suggestions

- None.

## Review Notes

- The change uses an instance-specific non-blocking override for `lamp_wooden_lantern#10` and blocking overrides for only the four near-spawn hill instances. Distant hills and other lamps retain their existing collision behavior.
- CPU tests use the real loaded map/collider data and cover both visible spawn-side hill walls plus both spawn-to-gate routes.
- The live walk-test uses the actual fixed-update callback, starts each route at spawn, checks both near-spawn wall stops and north/south gate passage, and writes traces under the feature spec directory. It identifies blockers `terrain_hill_green#64` at X=-7.85 m and `terrain_hill_green#69` at X=+7.85 m; gate routes pass to Z=+/-45 m.
- Build, all CTest tests, client smoke, bootstrap smoke, and the 30/60/144 FPS live walk-test passed as documented in `test-report.md`.
- The final validation wrote its captures under the feature spec directory. An initial baseline walk-test ran from the repository root and may have regenerated existing untracked `captures/walk-test-*.png/json`; this is disclosed in `test-report.md` because the original file contents were not recorded.
