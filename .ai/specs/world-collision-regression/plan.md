# World Collision Regression Fix

## 1. Objective

Fix the current world movement collision errors: the player must not pass through solid walls, and must be able to traverse openings that are visibly clear. Identify which loaded collider or collision-resolution behavior causes each error before changing behavior. Keep the game collision logic in `glory_game`, independent of rendering.

## 2. Current Findings

- `PLAN.md` marks prior wall and opening work complete, so this is a regression follow-up and does not advance the first unchecked roadmap item A3.
- `CollisionWorld::loadFromFiles` constructs collision from `assets/data/world_map.json` and `assets/data/world_colliders.json`; per-instance overrides can replace or compose local bounds.
- `CollisionWorld::resolve` applies capsule movement in substeps, resolves against each blocking AABB, then resolves remaining overlaps. Existing CPU coverage asserts gate traversal, authored cliff blocking, and decoration pass-through, but does not identify the user's current route or the exact obstructing/penetrable collider.
- Reproducing the controller route from spawn found `lamp_wooden_lantern#10` blocks the center south road at Z=-7.75. The old live gate routes began at Z=+/-16, so they bypassed the obstruction; the north route also ran only five seconds and ended before reaching the gate from spawn.
- The initial route reached the deeper cliffs at X=+/-13.46, but visible near-spawn hill meshes `terrain_hill_green#63/#64/#68/#69` were globally non-blocking. The player could enter those visible side walls before reaching the deeper cliff colliders.
- The worktree already contains untracked collision captures and walk-test outputs. Preserve them; write any new captures to a distinct validation location.

## 3. Tasks

1. Inspect the current runtime collision load path and existing authored map data; reproduce or instrument the reported behaviors using the current map and the standard player capsule.
2. For representative visible wall crossings and clear gaps, record player start/end positions and the responsible collider (or prove that no collider is queried). Compare collision wireframes against the rendered map where a client capture is needed.
3. Correct only the demonstrated causes. Keep `lamp_wooden_lantern#10` non-blocking and author blockers for the four near-spawn hill instances, while leaving distant decorative hills non-blocking. Change `collision.cpp` only if the solver itself permits penetration or blocks a valid gap. Keep client and server using the shared game collision world.
4. Add CPU regression coverage in `tests/collision_tests.cpp` for both spawn-to-gate routes and both spawn-side walls. Extend the live walk-test to cover these routes from spawn and run long enough to cross the north gate.
5. Run the configured build, CTest suite, client/bootstrap smoke checks, and a relevant live collision walk check. Save new artifacts separately and inspect any generated collision captures.
6. Write `.ai/specs/world-collision-regression/test-report.md` and `review.md`. Update `PLAN.md` only if the change establishes a new roadmap item; do not re-mark already completed collision milestones.

## 4. Files to Touch

- `.ai/specs/world-collision-regression/plan.md`
- `assets/data/world_colliders.json` if reproduction shows incorrect authored shapes
- `game/src/collision.cpp` only if reproduction shows a solver defect
- `tests/collision_tests.cpp`
- `game/src/main.cpp` only if a narrowly scoped live collision regression route is needed
- `.ai/specs/world-collision-regression/test-report.md`
- `.ai/specs/world-collision-regression/review.md`

New generated captures, if needed, must use a new directory or filenames that do not overwrite existing user artifacts.

## 5. Risks and Mitigations

- The user has not specified exact wall/gap locations. First use existing map geometry and reproducible routes; if the failures cannot be identified, ask for a location or screenshot before changing unrelated collision data.
- AABB approximations can trade wall penetration for false blocking. Keep authored parts localized and verify both sides of each opening.
- Movement is shared by client and server. Keep any solver correction in `glory_game` and cover the shared resolver with CPU tests.
- Existing untracked capture files are user data; do not modify or remove them.

## 6. Acceptance Criteria

- The reproduced visible solid walls block the standard player capsule from both approach directions without tunneling.
- The reproduced visibly clear routes remain traversable in both directions without collision from unrelated neighboring bounds or arena recovery logic.
- The visible hill walls immediately east and west of spawn block the player before it enters their mesh bounds; distant hills remain non-blocking.
- Both north and south gate openings are reachable from the player spawn along the central roads.
- Automated tests fail on the relevant old behavior and pass with the fix, using the loaded world collision data where applicable.
- Build, CTest, smoke checks, and relevant live runtime verification pass; results are recorded in `test-report.md`.
- `review.md` confirms the change follows this plan and the architectural rules in `PLAN.md`.
