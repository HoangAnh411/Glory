# Bot Load Testing Test Report

## Commands executed

- `cmake -S . -B build -G Ninja` — PASS (configured and generated; only existing dependency/toolchain warnings).
- `cmake --build build` — PASS (`ninja: no work to do` after all changed targets had compiled and linked successfully).
- `ctest --test-dir build -R glory_load_test_bots --output-on-failure` — PASS, 1/1.
- `build/glory_bot_runner.exe --bots 8 --ticks 120` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 34/34.
- `build/glory_server.exe --smoke-test` — PASS, exit code 0.
- `build/glory_client.exe --smoke-test` — PASS, exit code 0.

## Load-test results

- Requested/connected bots: 8/8 (100%). Each bot used its own ENet client, completed login, character-list, character-create, and enter-world packet exchanges, and received snapshots.
- Server ticks: 120/120. Average `ServerSimulation::tick`: 0.0426908 ms; maximum: 0.0991 ms (average target < 16.6 ms).
- Network application totals: 1,984 packets sent, 1,984 packets received, 1,614,512 bytes in both directions; excludes ENet framing and connection handshake.
- Bot workload: 960 client input packets processed and 960 authoritative snapshots decoded.
- Cleanup: 0 server players remained after disconnect.
- Auth boundary: ENet and authoritative simulation are real; auth/character/world responses are deterministic in-process protocol responses, so this validates packet flow without the production database.

## Initial test adjustment

The first run exposed that an input sent at the end of a coordinator iteration had not yet been serviced by the client host before the server's poll. The coordinator now services clients after sending input and then drains server events before advancing the simulation. The final run processed all 960 input packets. Client input uses the reliable ENet channel in this harness to make the fixed-tick acceptance count deterministic. Movement directions, yaw, and attack choices use a per-bot seeded pseudo-random sequence.

## Smoke-test output

- Server: `Headless server smoke test passed: 120 fixed ticks at 60 Hz`.
- Client: `SDL3/OpenGL 4.5 context and 120 fixed ticks passed`.

## Overall

PASS. Build, focused integration test, all 34 CTests, bot CLI, and both application smoke tests completed successfully.
