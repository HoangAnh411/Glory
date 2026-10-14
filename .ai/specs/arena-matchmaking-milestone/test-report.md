# Test report: Arena matchmaking and 100 ms milestone

Date: 2026-10-14

## Commands run

- `cmake -S . -B build -G Ninja` — PASS.
- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 25/25 tests.
- `.\build\glory_server.exe --smoke-test --port 17779` — PASS, 120 fixed ticks, exit code 0.
- `.\build\glory_client.exe --smoke-test` — PASS, SDL3/OpenGL context and 120 fixed ticks, exit code 0.
- `git diff --check` — PASS.

## Arena coverage

- Duel waits for two clients, assigns Red/Blue, uses separate team spawns, counts down for three seconds, enters the round, disables friendly fire, and declares Red winner when Blue HP reaches zero — PASS.
- Team 3v3 waits for six clients, assigns three per team and distributes players across team spawn lanes — PASS.
- Server simulation keeps player movement and HP unchanged during countdown and includes match state/team IDs in authoritative state — PASS.
- Matchmaking request and arena snapshot binary codecs round-trip; invalid queue mode is rejected — PASS.
- Two ENet loopback clients connect and both matchmaking requests reach the server — PASS.
- Test delays the loopback Ping/Pong path by approximately 50 ms per direction; measured application RTT was 122 ms — PASS.
- Local prediction moves before a server response; a historical attack hits the target at the rendered position, applies 25 damage, and preserves the target's current X=4 transform — PASS.
- Entire existing and new CTest suite — 100% passed (25/25).

## Scope

The RTT is measured on loopback with application-side delay injection. This verifies the latency-sensitive code paths deterministically; it is not a measurement of two physical machines over the public Internet.
