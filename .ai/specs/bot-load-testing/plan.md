# Bot Load Testing Plan

## 1. Objective

Add a headless ENet bot swarm that exercises the authoritative server simulation and reports connection, network, and fixed-tick performance. The default run uses 8 bots for 120 server ticks, with no rendering or database dependency.

## 2. Tasks

1. Implement a `BotClient` state machine (`Disconnected`, `Connecting`, `Authenticating`, `EnteringWorld`, `SimulatingLoop`, `Disconnecting`) that owns one `NetworkClient`, sends the existing login/character/world protocol, emits seeded pseudo-random movement and attack inputs, and decodes snapshots.
2. Implement a `LoadTestCoordinator` that creates an ENet loopback server and `ServerSimulation`, admits up to the simulation's current capacity of 8 bots, handles protocol events, applies inputs, ticks the simulation, broadcasts encoded snapshots, and disconnects/cleans every bot.
3. Collect completed tick count, average/max `ServerSimulation::tick` duration, sent/received packet and byte counts, and successful connection rate. Keep timing scoped to simulation ticks and state that distinction in the summary.
4. Add the standalone `glory_bot_runner --bots N --ticks N` CLI with validated arguments and a concise `Server Load Test Summary` report.
5. Register `tests/load_test_bot_tests.cpp` as the 34th CTest. Run at least 8 independent loopback clients through auth/world entry and 120 ticks of input and snapshots; verify metrics, tick budget, and disconnect cleanup.
6. Build all targets, run all 34 CTests and the dedicated server/client smoke tests; write actual results to `test-report.md`, review against this plan and `PLAN.md`, fix required findings and re-verify, then mark the roadmap item complete.

## 3. Files to touch

- `server/include/glory/server/load_test.hpp` — bot state and load-test result/coordinator API.
- `server/src/load_test.cpp` — headless bot protocol behavior, in-process loopback server orchestration, metrics, and report output.
- `tools/bot_runner.cpp` — CLI entry point.
- `tests/load_test_bot_tests.cpp` — ENet loopback integration and acceptance assertions.
- `CMakeLists.txt` — compile the load-test module and CLI; register CTest #34.
- `.ai/specs/bot-load-testing/plan.md` — this plan.
- `.ai/specs/bot-load-testing/test-report.md` — terminal commands/results.
- `.ai/specs/bot-load-testing/review.md` — reviewer findings.
- `PLAN.md` — tick only “Load test bằng bot” after successful verification.

## 4. Risks & Mitigations

- `ServerSimulation` currently supports at most 8 sessions. Default and acceptance runs use exactly 8; reject unsupported bot counts clearly rather than silently dropping clients.
- The standalone harness has no configured production database credentials. It will exercise the real ENet transport and login/character/world packet exchange with deterministic in-process protocol responses, then run the real `ServerSimulation`; the report will identify this auth boundary.
- Wall-clock tick timing varies by host and CI load. Measure only the simulation tick call, require average below 16.6 ms, and report maximum without making the maximum a hard acceptance threshold.
- Concurrent ENet traffic and simulation run on the same coordinator thread to preserve the production event-loop model and avoid shared-state races.
- Existing worktree contains many uncommitted files. Limit edits to the listed feature files and preserve all unrelated edits.

## 5. Acceptance Criteria

- `glory_bot_runner --bots 8 --ticks 120` completes a full loopback session and prints tick timing, packet/byte totals, and connected bot rate.
- Eight bots each own an independent headless `NetworkClient`, complete the documented app-level login/character/world exchange, send changing movement/attack input on all 120 ticks, and receive authoritative snapshots.
- Metrics show 120 completed ticks, 8/8 successful connections, nonzero packets/bytes sent and received, and average simulation tick duration below 16.6 ms.
- All bots disconnect and `ServerSimulation::playerCount()` returns to zero.
- The new load-test test passes as CTest registration 34, the full 34-test suite passes, and both `glory_server --smoke-test` and `glory_client --smoke-test` pass.
- Test and reviewer reports record actual outcomes; roadmap is ticked only after all required checks pass.
