# C2: Swept Player Capsule Movement and Arena Bounds

## Objective
Make all player displacement paths use stable capsule movement against loaded static AABBs and the arena boundary, including run, dodge, jump, knockback, spawn depenetration, and low-frame-rate steps.

## Tasks
1. Replace the fixed 256-substep cap with safe radius-based subdivision so each integrated displacement segment is at most half the capsule radius.
2. Resolve horizontal overlaps iteratively, remove velocity into each contact normal, and preserve tangential motion at corners.
3. Enforce the circular arena boundary using capsule radius and keep the floor at y=0.
4. Ensure spawn/teleport and knockback overlap is depenetrated by the same collision world.
5. Add CPU tests for 30/60/144 Hz normal and dodge movement, diagonal wall slide, corner/acute contacts, low steps, spawn overlap, and 10,000 bounded random steps.
6. Build, run CTest, smoke, and commit C2 separately.

## Files
- `game/src/collision.cpp`, `game/include/glory/game/collision.hpp`
- `game/src/character_controller.cpp` if integration requires adjustment
- `tests/collision_tests.cpp`
- `PLAN.md`, `.ai/specs/world-collision-c2/`

## Risks
- Large yaw-transformed prop AABBs may constrain paths conservatively; this is data geometry, not a solver issue.
- Circle boundary resolution applies at all heights so a jump cannot escape over the arena rim.
- Keep movement health, dodge invulnerability, hitboxes, and combo processing unchanged.

## Acceptance
- No capsule tunnels through AABBs at tested normal/dodge speeds and frame rates.
- Diagonal movement preserves nonzero tangential velocity; repeated corner contacts remain stable.
- No random test position leaves the radius-inset arena or drops below y=0.
- Build, CTest, and smoke pass.
