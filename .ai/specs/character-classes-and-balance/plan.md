# Character Classes and Balance Plan

## 1. Objective
Add validated JSON class definitions for Warrior, Assassin, and Mage; bind each class to its own skills, stats, movement and dodge tuning; apply defense mitigation in combat; validate class selection on the server; expose class selection and class details in ImGui.

## 2. Tasks
1. Add `ClassCatalog` and `CharacterClassDefinition` in `glory_game`, with strict JSON validation, lookup, class skill registry construction, and the shared defense mitigation formula.
2. Extend `skills.json` with six class-bound skills (including stun, bleed, burn, and slow) and create `classes.json` with the requested base stats/speeds and skill ids.
3. Add class selection/details to the account panel; apply selected class skill registry, movement/dodge configuration, attack scaling and Max HP when playing locally or entering a character.
4. Validate `CreateCharacterRequest.className` against the class catalog on the server and configure the authoritative player session with the selected class stats and skill registry.
5. Apply outgoing attack scaling and defense mitigation to PvP damage, and retain class Max HP/Defense in authoritative player state.
6. Add `tests/character_class_balance_tests.cpp` and CTest #32 for catalog validation, class stats/skills, mitigation, and class combat characteristics.
7. Build, run all 32 CTest tests and both smoke tests; record Tester and Reviewer reports; tick PLAN.md only after passing review.

## 3. Files to touch
- `.ai/specs/character-classes-and-balance/{plan.md,test-report.md,review.md}`
- `assets/data/classes.json`, `assets/data/skills.json`
- `game/include/glory/game/classes.hpp`, `game/src/classes.cpp`
- `game/include/glory/game/combat.hpp`, `game/src/combat.cpp`, `game/include/glory/game/character_controller.hpp`, `game/src/character_controller.cpp`
- `server/include/glory/server/server_simulation.hpp`, `server/src/server_simulation.cpp`, `server/src/main.cpp`
- `network/include/glory/network/network.hpp`
- `engine/include/glory/engine/application.hpp`, `engine/src/debug_panel.cpp`
- `game/src/main.cpp`, `tests/character_class_balance_tests.cpp`, `CMakeLists.txt`, `PLAN.md`

## 4. Risks & Mitigations
- Class skill ids can drift from skill data: validate every referenced id at catalog load and fail startup on missing/duplicate skills.
- Existing characters may contain legacy class names: keep server class validation authoritative for new characters; map unknown legacy characters to Warrior at session entry without rewriting saved identity.
- UI and server stats can diverge: load the same class/skill JSON on each process and use server-side class configuration for online snapshots and damage.
- Existing combat tests assume default behavior: keep zero-defense targets unchanged and preserve default controller/session behavior until a class is explicitly applied.

## 5. Acceptance Criteria
- All three requested classes load with the specified HP/Attack/Defense/Speed and Assassin dodge cooldown.
- Each class resolves only its listed skill ids; skill effects include Warrior stun, Assassin bleed, and Mage burn/slow.
- Invalid class JSON, unknown skill references, and invalid server class selections are rejected.
- `DamageTaken = max(1, Damage * 100 / (100 + Defense))` is finite, monotonic with Defense, and used by authoritative PvP damage.
- Class HP, movement speed, dodge cooldown, outgoing Attack scaling, and active skill set are applied on character entry.
- The client offers class selection and displays class description/stats and current class-exclusive skills.
- Build, all 32 CTest tests, and server/client smoke tests pass; reports and review exist before roadmap completion.
