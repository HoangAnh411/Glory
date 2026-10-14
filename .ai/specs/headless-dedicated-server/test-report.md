# Test Report: Headless Dedicated Server

## Commands executed

- `cmake -S . -B build -G Ninja` — PASS (exit code 0).
- `cmake --build build` — PASS (exit code 0).
- `ctest --test-dir build --output-on-failure` — PASS, 19/19 tests (exit code 0).
- `.\build\glory_server.exe --smoke-test` — PASS (exit code 0).
- `.\build\glory_server.exe --ticks 5` — PASS; reported exactly 5 ticks / 0.0833333 simulated seconds.
- `.\build\glory_client.exe --smoke-test` — PASS (exit code 0).

## Acceptance coverage

- Server simulation runs exactly 120 fixed ticks and accumulates 2.0 simulation seconds — Passed (`glory_server_simulation`).
- Server-side character controller resolves collision against the sample world wall — Passed (`glory_server_simulation`).
- Server-side combat emits a hit and applies skill damage without any render context — Passed (`glory_server_simulation`).
- Server loads `world_spawns.json` and `skills.json`, then reports player, boss and active trigger state — Passed (server smoke output).
- `glory_server` links only `glory_game`; GLM and nlohmann/json arrive through `glory_game` public dependencies. It does not reference or link `glory_engine`, SDL3, OpenGL, ImGui or miniaudio — Passed (CMake target/source inspection and successful standalone target build).
- Client render loop regression check — Passed; output: `SDL3/OpenGL 4.5 context and 120 fixed ticks passed`.

## Test iteration note

The first new server test run exposed assumptions in the test fixture: it sampled collision state only at the final tick and used a movement key/camera orientation that did not approach the wall; the combat fixture also moved the target before the server tick overwrote it from the Boss state. The test now observes collision across all ticks, moves forward toward the wall, and positions the Boss spawn for combat. The full CTest suite and both smoke tests passed after these corrections.

## Final result

**PASS** — Build, all 19 CTest cases, headless server smoke test, finite `--ticks` run, and client smoke test succeeded.
