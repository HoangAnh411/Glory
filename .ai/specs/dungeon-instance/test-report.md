# Test Report: Dungeon Instance

## Commands and results

- `cmake -S . -B build -G Ninja` — PASS; CMake configured and generated the Ninja build.
- `cmake --build build` — PASS; client, server, and test targets linked.
- `ctest --test-dir build --output-on-failure` — PASS; 30/30 tests, including `glory_dungeon_instance`.
- `glory_server.exe --smoke-test --port 7794 --database build\dungeon-smoke-final.sqlite3` — PASS; completed 120 fixed ticks at 60 Hz.
- `glory_client.exe --smoke-test` — PASS; SDL3/OpenGL initialized and completed 120 fixed ticks.

## Dungeon test coverage

- Two Shadow Crypt instances receive different IDs and independent boss health; party members can share one instance.
- Boss damage and timer countdown; Boss defeat transitions to `Completed`; timeout and full-party wipe transition to `Failed`.
- Leave returns the saved origin and closes/reclaims an empty instance.
- Entry, leave, and state packets round-trip; truncated payloads and unknown result values are rejected.
- Account database tests verify dungeon EXP is persisted for the character owner and reject rewards for a non-owning session.

## Scope note

Smoke tests exercise server/client startup and fixed-tick execution. They do not connect multiple clients to exercise party entry, snapshot isolation, boss combat, or completion rewards end to end.
