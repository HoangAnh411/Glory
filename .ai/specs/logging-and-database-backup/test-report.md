# Logging and Database Backup Test Report

## Commands executed

- `cmake -S . -B build -G Ninja` — PASS; configure/generate succeeded. CMake emitted existing dependency deprecation and optional LibUSB/PkgConfig warnings.
- `cmake --build build` — PASS; all changed targets compiled and linked. Final build reported `ninja: no work to do`.
- `ctest --test-dir build -R glory_logging_backup --output-on-failure` — PASS, 1/1.
- `ctest --test-dir build --output-on-failure` — PASS, 35/35.
- `glory_server.exe --smoke-test --database build/logging-backup-cli/accounts.sqlite3 --log-file build/logging-backup-cli/periodic-final.log --log-level DEBUG --backup-dir build/logging-backup-cli/backups --backup-interval 1` — PASS, exit code 0; server completed 120 ticks and logged a verified periodic backup from the worker thread.
- `glory_server.exe --backup-now --database build/logging-backup-cli/accounts.sqlite3 --backup-dir build/logging-backup-cli/backups --log-level WARN --log-file build/logging-backup-cli/backup-now-final.log` — PASS, exit code 0; output included `integrity_check=ok`, and the process exited without starting ENet.
- `glory_client.exe --smoke-test` — PASS, exit code 0; SDL3/OpenGL and 120 fixed ticks passed.

## Coverage

- Logger created a nested log directory, filtered messages at two configured thresholds, emitted all five levels, verified the timestamp/level/category format, and retained all 20 concurrent writer messages as intact lines.
- Online backup ran while the source `AccountDatabase` remained open. Integrity verification returned `ok`; a new `AccountDatabase` authenticated the backed-up account and recovered the character, 4,321 gold, both inventory slots, and both equipment records exactly.
- Invalid SQLite input failed integrity verification. Four timestamped backup files were unique; rotating to two retained the lexicographically newest pair and preserved an unrelated `.sqlite3` file.
- Periodic backup ran on a background worker and logged its duration/result without affecting the server's 120-tick smoke completion. `--backup-now` created and verified a timestamped backup before the listener setup path.

## Initial test adjustment

The first focused CTest run failed because the expected logger line count was one too high. The test had four baseline emitted lines plus 20 concurrent lines; correcting the expected count made the focused test and full suite pass. No production-code test failure remained.

## Overall

PASS. Full build, focused CTest, all 35 CTests, periodic and one-shot backup CLI paths, and client/server smoke checks completed successfully.
