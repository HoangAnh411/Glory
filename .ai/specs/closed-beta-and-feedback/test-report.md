# Test Report — Closed Beta Feedback

## Commands Executed

- `cmake --build build` — PASS, exit code 0.
- `ctest --test-dir build --output-on-failure` — PASS, 36/36 tests, exit code 0 (final run: 3.55 s).
- `build/glory_server.exe --smoke-test --database build/closed-beta-validation/accounts.sqlite3 --log-file build/closed-beta-validation/server.log` — PASS, 120 server ticks at 60 Hz, exit code 0.
- `build/glory_client.exe --smoke-test` — PASS, SDL3/OpenGL 4.5 context and 120 fixed ticks, exit code 0.
- `build/glory_server.exe --database build/closed-beta-validation/accounts.sqlite3 --log-file build/closed-beta-validation/export.log --export-feedback build/closed-beta-validation/export.json` — PASS, exported and exited with code 0.
- PowerShell `ConvertFrom-Json` on `build/closed-beta-validation/export.json` — PASS; empty database exported a valid empty JSON array.

## Acceptance Checks

- Packet request and response round-trip; truncated packet, ratings outside 1–5, and overlength subject/description are rejected; all ratings 1–5 round-trip — PASS (`glory_closed_beta_feedback`).
- Feedback insert requires a valid session and owned character; invalid rating and malformed diagnostics JSON are rejected — PASS.
- Repeated valid submission is rate limited per authenticated account — PASS.
- Export preserves quoted text and emits diagnostics as a valid nested JSON object — PASS, parsed and asserted with nlohmann JSON in the CTest.
- Export refuses to overwrite the SQLite source database — PASS.
- Server export CLI creates a parseable JSON file and exits — PASS.
- F8 feedback UI and diagnostics code compile as part of `glory_client` — PASS; interactive form behavior was not manually exercised.
- Beta guide documents startup, feature checklist, collection, and release preparation — PASS by review.

## Test Iteration Note

The first focused CTest run found that the test attempted to remove its temporary directory while an input stream was open on Windows. The test now closes that stream before cleanup. A later review also moved the rate limit from connection IDs to authenticated account IDs and added repeat-submission coverage. The final full build, 36-test run, and both smoke tests all pass.

## Overall Result

**PASS** — required build, 36 CTests, server/client smoke tests, and feedback export CLI validation succeeded.
