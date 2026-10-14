# Input Capture and Remapping Plan

## 1. Objective

Implement keyboard and mouse input with a strict boundary: the engine translates SDL3 events into platform-neutral physical-input frames, while the game layer maps those inputs to actions and supports runtime rebinding. The action mapping must remain testable without SDL or graphics.

## 2. Tasks

1. Add a small shared input contract containing a platform-neutral set of keyboard/mouse controls and frame transitions (pressed and released controls).
2. Add an engine `InputSystem` that consumes SDL3 events and produces the shared input frame, including continuous held state and mouse motion; ignore unsupported controls and repeated key-down events for edge transitions.
3. Integrate input polling in the application loop without changing window-close behavior. Preserve Escape-to-quit in the engine.
4. Add a game `ActionMap` that binds physical controls to named gameplay actions, computes pressed/held/released action states each frame, and permits rebinding with validation.
5. Add deterministic unit tests for default bindings, state transitions, rebinding, invalid inputs, and independent action mapping. Register them with CTest.
6. Build and run CTest plus the existing SDL/OpenGL smoke test; record exact commands and results in `test-report.md`.
7. Review implementation against this plan and the root roadmap; record findings in `review.md`.

## 3. Files to Create or Modify

- `shared/include/glory/input/input_frame.hpp` — platform-neutral control identifiers and input frame.
- `engine/include/glory/engine/input_system.hpp` — engine-only SDL event input API; SDL types remain outside shared and game APIs.
- `engine/src/input_system.cpp` — SDL3 event translation and held-state tracking.
- `engine/include/glory/engine/application.hpp` — pass input frames to the fixed-update callback.
- `engine/src/application.cpp` — poll input events and provide input frames on fixed updates.
- `game/include/glory/game/action_map.hpp` — action identifiers/states and configurable bindings.
- `game/src/action_map.cpp` — binding validation and action-state evaluation.
- `game/src/main.cpp` — consume the game action map from the application callback.
- `tests/input_mapping_tests.cpp` — SDL-free mapping and rebinding tests.
- `CMakeLists.txt` — compile the new sources and register tests, preserving existing user DLL-copy changes.
- `.ai/specs/input-remapping/test-report.md` — actual test execution results.
- `.ai/specs/input-remapping/review.md` — independent review findings.
- `PLAN.md` — tick only the input/remapping item after all roles pass.

## 4. Risks and Mitigations

- **Game/platform coupling:** keep all public input types free of SDL and OpenGL, with shared value types used by engine and game.
- **Lost transitions between fixed updates:** accumulate pressed/released edges while polling and consume them on the next fixed update; held state remains persistent.
- **SDL key repeat:** ignore repeated key-down events for pressed edges while retaining held state.
- **Invalid or conflicting bindings:** reject `Unknown` controls and invalid actions; allow intentional shared bindings so one input can trigger multiple actions.
- **Application callback compatibility:** update all call sites and validate through full build, unit tests, and the current smoke-test executable.
- **Existing user CMake change:** preserve DLL-copy logic and verify the final CMake diff retains it.

## 5. Acceptance Criteria

- SDL keyboard, mouse-button, and relative mouse-motion events are captured in the engine and no SDL types leak into shared/game APIs.
- Game actions expose correct pressed/held/released transitions and can be rebound at runtime.
- Invalid actions/controls are rejected without corrupting existing bindings; multiple actions may intentionally share an input.
- Unit tests exercise defaults, transitions, rebinding, invalid input, and shared bindings and pass via CTest.
- CMake configure/build succeeds; existing SDL/OpenGL smoke test still runs successfully.
- `test-report.md` and `review.md` are present, and the root roadmap item is ticked only after PASS and a review with no required fixes.
