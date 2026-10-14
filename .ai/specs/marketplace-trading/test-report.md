# Marketplace Trading Test Report

## Commands executed
- `cmake -S . -B build -G Ninja` — PASS.
- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 31/31 tests.
- `.\build\glory_server.exe --smoke-test --database data\marketplace-smoke.sqlite3` — PASS, 120 fixed ticks at 60 Hz.
- `.\build\glory_client.exe --smoke-test` — PASS, SDL3/OpenGL context and 120 fixed ticks.
- `git diff --check` — PASS (Git printed only line-ending normalization warnings for existing modified files).

## Marketplace coverage
- Legacy SQLite schema gains `characters.gold` with the 1000 default while retaining existing character data — PASS.
- Account registration, seller inventory deduction, filtered/search listing browse, successful purchase, and buyer/seller gold transfer — PASS.
- Failed purchase with insufficient gold leaves buyer gold unchanged — PASS.
- Failed purchase with a full inventory leaves buyer gold and listing status unchanged — PASS.
- Only the seller can cancel an active listing; cancellation succeeds for seller, and sold listings cannot be cancelled — PASS.
- Gold and an active listing survive database close/reopen — PASS.
- Browse/list/buy/cancel and gold binary codecs round-trip — PASS.

## Overall
PASS. All 31 registered CTest tests pass; server and client smoke tests pass.
