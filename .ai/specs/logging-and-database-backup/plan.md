# Logging and Database Backup Plan

## 1. Objective

Add a thread-safe structured server logger and online SQLite backup, integrity verification, and retention rotation. Expose log configuration and one-shot/periodic backups through `glory_server` without stopping its simulation loop.

## 2. Tasks

1. Implement `StructuredLogger` with Debug/Info/Warn/Error/Critical levels, the seven server categories, dynamic minimum-level filtering, timestamped format, optional append-only file output, parent-directory creation, and synchronized console/file writes.
2. Implement `DatabaseBackupManager` using SQLite's online backup API in bounded page batches with busy/locked retry, destination-path validation, timestamped unique backup names, `PRAGMA integrity_check`, and retention rotation limited to the manager's own backup filename pattern.
3. Add `glory_server` options `--log-level`, `--log-file`, `--backup-dir`, `--backup-interval`, and `--backup-now`; initialize logging before server setup, emit useful system/network/auth/database/anti-cheat entries, run backup-now and integrity verification before starting ENet, and schedule interval backups from the server loop on a background worker.
4. Add `tests/logging_backup_tests.cpp` as CTest #35. Verify filtering, timestamp/category format, nested file creation, logger concurrency, a hot backup of account/character/gold/inventory/equipment data, integrity, restoration through a second `AccountDatabase`, and retention rotation.
5. Configure/build, run the focused test and all 35 CTests, invoke backup-now with an isolated temporary database, run both application smoke tests, and record actual outcomes in `test-report.md`.
6. Review implementation against this plan and root `PLAN.md`, record all three review groups in `review.md`, fix any required findings and retest, then tick only “Logging và backup database” in the roadmap.

## 3. Files to touch

- `server/include/glory/server/structured_logger.hpp` — logger API, severity and category enums.
- `server/src/structured_logger.cpp` — timestamp formatting, filtering, file/console synchronization.
- `server/include/glory/server/database_backup.hpp` — backup manager API.
- `server/src/database_backup.cpp` — online backup, integrity checking, timestamp naming, and rotation.
- `server/src/main.cpp` — CLI options, logger calls, backup-now path, and periodic backup scheduling.
- `tests/logging_backup_tests.cpp` — logger and SQLite backup coverage.
- `CMakeLists.txt` — server sources/library and CTest #35 wiring.
- `.ai/specs/logging-and-database-backup/plan.md` — this plan.
- `.ai/specs/logging-and-database-backup/test-report.md` — actual build/test/smoke results.
- `.ai/specs/logging-and-database-backup/review.md` — reviewer findings.
- `PLAN.md` — tick only after all verification and review steps pass.

## 4. Risks & Mitigations

- A hot backup can encounter active SQLite writes. Use `sqlite3_backup_step` in bounded batches, retry `SQLITE_BUSY`/`SQLITE_LOCKED`, and always finish/close both handles through RAII.
- A failed backup must not be mistaken for a valid artifact. Verify with `PRAGMA integrity_check`, report the SQLite detail, and remove only an incomplete destination created by the current operation.
- Rotation must not remove arbitrary files. Restrict candidates to regular `.sqlite3` files named `glory-backup-*`, sort by timestamped name, and retain the requested newest count.
- Timestamp collisions can overwrite backups. Use millisecond timestamps and add a numeric suffix when a name already exists.
- Periodic backup work must not stall simulation ticks. Schedule it by steady clock on a background worker, use SQLite online incremental steps, and log duration/result; never run it from the render/client path.
- The worktree contains prior uncommitted changes. Preserve them and make only the feature-scoped edits listed above.

## 5. Acceptance Criteria

- Logger output matches `[YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [Category] message`; file parent directories are created, level filtering is correct and mutable, and concurrent writes produce intact lines.
- Online backup succeeds while the source `AccountDatabase` remains open; integrity check passes and a new database instance reads the exact account, character, gold, inventory, and equipment values.
- Timestamped backups are unique and rotation keeps only the requested newest owned backup files without touching unrelated files.
- `glory_server` accepts and applies all requested CLI options; `--backup-now` creates, verifies, reports, and exits without opening a listener; periodic backups execute while the server runs.
- The new test is CTest #35; all 35 tests and both server/client smoke tests pass. Reports contain real command outcomes, and roadmap changes only after PASS.
