# Review: World Collision Runtime Audit

## Must Fix

- None found in the reviewed runtime collision path after the live boss overlap regression passes.

## Should Fix

- Replace the circular arena safety limit with authored perimeter colliders if the playable arena is expected to follow non-circular walls.
- Replace terrain hill/cliff AABBs with walkable terrain or mesh-derived collision shapes where actors are expected to traverse them.

## Suggestions

- Keep the `--walk-test` and collider AABB audit in a repeatable CI job so client/server collision wiring cannot silently regress.
- The dark pointed shapes from the user-provided screenshot need a source screenshot or a camera capture matching that location to identify their mesh/material conclusively.

## Decision

Build, CTest, client smoke, executable-relative asset lookup, per-prop bounds audit, 14 collider captures, and all three live-update walk-test runs pass. The result is suitable as a collision-path fix. The audit does not claim that a continuous visible wall collider exists; the radius-34 circle remains the authoritative safety boundary.
