# Review

## Phai sua

- None. The requested blocking and bounds alignment are implemented and covered by CPU and live route checks.

## Nen sua

- None for this scoped fix.

## Goi y

- Tree entries are intentionally reported as `AUTHORED-TRUNK` because their collision is a narrow trunk footprint while the foliage remains passable. A separate task should only broaden those bounds after a specific tree traversal location is confirmed.
- The existing `--capture-colliders` camera scenario still fails with `Capture root-joint marker is behind the camera`; it should be addressed with the camera capture work rather than mixed into this prop data fix.

## Checklist

- Shared `CollisionWorld` remains the source of client and server movement collision.
- No solver, combat hitbox, combo, damage, or cooldown code changed.
- Gate openings remain compound four-part shapes.
- Existing dirty captures and collision regression files were not reset or staged.
- `PLAN.md` was not ticked because this is a follow-up to completed collision roadmap items.
