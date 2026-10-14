# Test Report: Character Classes and Balance

## Environment
- Platform: Windows, repository build directory `build/`.
- Build command: `cmake --build build` — PASS (client rebuilt and linked successfully).

## Automated Tests
- Command: `ctest --test-dir build --output-on-failure`
- Result: PASS, 32/32 tests, 0 failures.
- The new `glory_character_class_balance` test passed, covering JSON catalog loading, all requested class stats, class-specific skills and effects, invalid class lookup, damage mitigation, and combat hit behavior including ranged Mage Fireball.

## Smoke Tests
- Server: `build/glory_server.exe --smoke-test --database build/class-balance-smoke.sqlite3` — PASS; completed 120 fixed ticks at 60 Hz.
- Client: `build/glory_client.exe --smoke-test` — PASS; SDL3/OpenGL 4.5 context initialized and completed 120 fixed ticks.

## Repository Checks
- `git diff --check` — PASS; no whitespace errors (Git printed only line-ending normalization notices for existing modified files).

## Notes
- The headless server smoke path exercises the default simulation without selecting a character; class-specific server application is covered by code-path review and the class catalog/combat unit test, not by this smoke command.
