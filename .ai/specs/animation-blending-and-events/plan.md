# Animation Blending and Events Plan

## 1. Objective

Extend the existing CPU skeletal animation logic with pose interpolation, cross-fade state transitions, and loop-safe clip events. Expose Idle/Run/Attack triggers, fade duration, blend weight, and a bounded event log in the existing DebugPanel. Keep pose and event logic independent of OpenGL, preserve GPU palette evaluation, and maintain existing application/test behavior.

## 2. Tasks

1. Add CPU-only `AnimationEvent`, `AnimationPose`, clip event storage, pose evaluation/blending helpers, safe quaternion shortest-path interpolation, and event dispatch/callback support to `animation.hpp/.cpp`.
2. Add a small animation state controller that transitions between named Idle/Run/Attack clips over a configurable fade duration, blends local node TRS poses, handles missing channels via bind pose, and exposes current state/blend factor.
3. Integrate the controller and event listeners with Model/ModelRenderer. Evaluate the GPU skin palette from the blended pose; dispatch events over normal and wrap-around time advances. Preserve direct clip selection/playback where useful.
4. Add/debug Idle, Run, Attack controls, fade-duration slider, blend-factor display, and recent event log to DebugPanel.
5. Supply at least three representative sample clips with attack/audio events in the existing sample asset or CPU fixture tests; add CPU unit tests for alpha endpoints/midpoint, quaternion shortest path, state transition and event timing including wrap-around.
6. Build, run all CTest cases and the prescribed 120-tick smoke test; record the actual results in this spec's test report. Review the implementation and record findings in review.md.
7. Tick only the two completed Giai đoạn 2 items in PLAN.md after Tester and Reviewer pass.

## 3. Files to Create / Modify

- `engine/include/glory/engine/animation.hpp`, `engine/src/animation.cpp` — pose blending, state machine, event schema/dispatcher and loop-safe event traversal.
- `engine/include/glory/engine/model.hpp`, `engine/src/model.cpp` — transition/update/event/palette integration.
- `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp` — expose state and event controls to the UI while retaining GPU skinning.
- `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp` — trigger buttons, fade control, blend weight and event log.
- `engine/src/gltf_asset.cpp`, `assets/models/starter_crate.gltf`, `assets/models/starter_crate.bin` — parse event metadata and provide distinct Idle/Run/Attack sample clips with attack events.
- `tests/skeletal_animation_tests.cpp`, `tests/asset_loader_tests.cpp` — independent CPU blending/events coverage and sample asset validation.
- `CMakeLists.txt` — only if test registration or asset fixture wiring needs changes.
- `.ai/specs/animation-blending-and-events/test-report.md`, `review.md`, `PLAN.md` — Tester/Reviewer outcomes and final roadmap updates.

## 4. Risks and Mitigations

- **Different animation channels:** build complete local poses from bind pose before blending so missing channels retain their base transforms.
- **Quaternion sign ambiguity:** flip the target quaternion when the dot product is negative before slerp; normalize the result.
- **Events across loop boundaries / large delta:** traverse each unwrapped time segment in order and define endpoint inclusion to avoid dropped events or duplicate firing.
- **State transitions during an active fade:** use the current blended pose as the next transition's source, preventing visual jumps.
- **Sample animation data shares a single skeleton:** give each named clip its own rotation keys and validate the resulting poses through CPU tests.
- **Scope and old worktree state:** inspect and preserve existing user changes; modify only the listed animation, renderer/UI, test/assets, spec and roadmap files.

## 5. Acceptance Criteria

- Blending at alpha 0, 0.5 and 1 returns source, exact midpoint and target TRS poses; rotation uses normalized shortest-path slerp.
- The CPU animation API has no OpenGL/GPU dependencies; the GPU palette is computed from the resulting blended hierarchy.
- Idle, Run and Attack can be triggered by the basic state controller with configurable fade duration and current blend weight.
- Clips carry named/time-stamped events with extensible parameters; registered `std::function<void(const AnimationEvent&)>` listeners receive events crossed during normal playback and loop wrap, and do not receive future events early.
- DebugPanel exposes state triggers, fade-duration adjustment, blend factor and a bounded recent event log.
- CPU CTest covers blend endpoints/midpoint, event crossing/no-early-fire/wrap-around, state transitions, and blended hierarchy/palette behavior.
- Prescribed build and all six CTest tests pass; `glory_client.exe --smoke-test` exits successfully after 120 ticks.
- Tester and Reviewer reports exist; only the two assigned roadmap items are ticked after PASS.
