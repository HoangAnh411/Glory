# Test Report: Chat, Party, Guild

## Results

- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 29/29 tests.
- `glory_server.exe --smoke-test --port 7791 --database build\social-smoke-review.sqlite3` — PASS; server completed 120 fixed ticks at 60 Hz.
- `glory_client.exe --smoke-test` — PASS; SDL3/OpenGL initialized and client completed 120 fixed ticks.

## Social coverage

- `glory_social_system_tests` verifies World, Zone, Party, Guild, Whisper, and System chat recipient routing; UTF-8 and message-length validation; party invite/accept/leave/kick, HP updates and leader transfer; SQLite guild creation, duplicate-name rejection, invitations, rank changes, leader transfer, and database reopen; plus chat/party/guild packet round-trips and truncated-packet rejection.
- The full regression run includes all 29 registered CTest tests.

## Notes

- Party and guild invitations are runtime state and expire when the server stops. Guild membership and rank are persisted in SQLite.
- Smoke tests validate startup and fixed-tick execution; they do not simulate multiple connected clients exchanging social packets.
