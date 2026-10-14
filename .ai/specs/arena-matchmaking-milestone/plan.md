# Arena matchmaking and 100 ms PvP milestone

## 1. Objective

Add server-authoritative Duel 1v1 and Team 3v3 queues, match lifecycle, teams, spawn assignment, friendly-fire protection, win declaration, binary network fields, and a client Arena panel. Verify a simulated 100 ms RTT match with existing prediction and lag compensation.

## 2. Tasks

1. Add a renderer-independent `ArenaManager` to `glory_game` for queue membership, match mode/state, team assignment, deterministic red/blue spawn transforms, 3-second countdown, round state, disconnect handling, and winner detection.
2. Extend the network protocol with matchmaking join/leave requests and snapshot arena state, including each player's team, mode, countdown, and winning team. Keep decoding bounded and reject malformed enum values/payload sizes.
3. Integrate arena state into `ServerSimulation`: accept queued players, lock movement and combat during countdown/waiting/finished states, enforce friendly-fire rules, apply team spawn positions at match start, and finish when one team has no living players.
4. Add a client-side Arena Matchmaking panel in `DebugPanel`, wired to send join/leave packets and display state, countdown, team colors, member HP, and victory/defeat from authoritative snapshots.
5. Add `tests/arena_matchmaking_tests.cpp` and register the 25th CTest. Cover 1v1/3v3 queue sizes, team/spawn assignment, lifecycle locks, friendly fire, winner, protocol validation, and a loopback match with injected 100 ms RTT using prediction and lag compensation.
6. Build and run all CTest cases and both client/server smoke tests; write the test report and independent review, then tick both Phase 5 roadmap items only if all required checks pass.

## 3. Files to create or update

- `game/include/glory/game/arena.hpp`
- `game/src/arena.cpp`
- `network/include/glory/network/network.hpp`
- `network/src/network.cpp`
- `server/include/glory/server/server_simulation.hpp`
- `server/src/server_simulation.cpp`
- `server/src/main.cpp`
- `engine/include/glory/engine/application.hpp`
- `engine/include/glory/engine/debug_panel.hpp`
- `engine/src/debug_panel.cpp`
- `game/src/main.cpp`
- `tests/arena_matchmaking_tests.cpp`
- `CMakeLists.txt`
- `PLAN.md`
- `.ai/specs/arena-matchmaking-milestone/{plan,test-report,review}.md`

## 4. Risks and mitigations

- Queue/disconnect churn can leave stale participants: remove clients from queues and active matches deterministically and recalculate outcomes.
- Client requests cannot dictate match state/team: the server validates requests and owns queue assignment, countdown, spawn, combat, and winner state.
- Existing free-for-all PvP must remain usable outside matchmaking: preserve current behavior when no arena round is active, and apply team restrictions only to matched players.
- UI changes may grow the DebugPanel API: pass compact arena request/state values and keep game rules out of `glory_engine`.
- The 100 ms milestone is a deterministic simulated RTT test, not a claim of physical Internet quality; report this scope explicitly.

## 5. Acceptance criteria

- 1v1 starts at two queued clients; 3v3 starts at six, with three players per team and correct distinct team spawn points.
- Countdown lasts 3 seconds and blocks movement/attacks; InRound enables them; same-team damage is rejected; eliminated teams produce an authoritative winner.
- Snapshot round-trips arena state and team IDs; malformed matchmaking requests are rejected.
- A loopback test injects 50 ms each way, exercises predicted input and lag-compensated attack, and verifies authoritative damage and match result.
- Build passes, all 25 CTest tests pass, and `glory_server --smoke-test` plus `glory_client --smoke-test` pass.
