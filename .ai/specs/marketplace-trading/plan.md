# Marketplace Trading Plan

## 1. Objective
Implement a server-authoritative marketplace with persistent character gold, listings, atomic purchase/cancellation, binary network messages, a client debug panel, and focused automated coverage. Preserve existing repository changes and migrate existing SQLite databases without data loss.

## 2. Tasks
1. Extend `AccountDatabase` with an idempotent `characters.gold` migration, marketplace schema, authenticated listing/browse/buy/cancel APIs, and transaction-safe inventory/gold operations.
2. Add bounded binary codecs and packet types for browse/list/buy/cancel and gold synchronization.
3. Route authenticated marketplace operations in `glory_server`; refresh online runtime inventory and gold after successful operations.
4. Add client state, actions, network request/response handling, and an ImGui marketplace panel with browse/search, sell, and own listings.
5. Add `tests/marketplace_tests.cpp`, register CTest #31, covering persistence, routing data, rollback, and packet round trips.
6. Configure/build, run all CTest tests, run server/client smoke tests, record results, review implementation, then tick the roadmap item.

## 3. Files to touch
- `.ai/specs/marketplace-trading/{plan.md,test-report.md,review.md}`
- `server/include/glory/server/account_database.hpp`, `server/src/account_database.cpp`, `server/src/main.cpp`
- `network/include/glory/network/network.hpp`, `network/src/network.cpp`
- `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp`, `engine/src/debug_panel.cpp`
- `game/src/main.cpp`
- `tests/marketplace_tests.cpp`, `CMakeLists.txt`, `PLAN.md`

## 4. Risks & Mitigations
- Existing databases may predate gold: inspect `PRAGMA table_info` and apply `ALTER TABLE` only when required.
- Purchase races or partial writes could duplicate items/currency: serialize database operations and use `BEGIN IMMEDIATE` with rollback on every failed condition.
- Network input may be malformed or oversized: validate enums, strings, counts, and packet payload lengths in codec and server handlers.
- Online runtime inventory can diverge from the database after a market action: reload persisted character state after a successful transaction before syncing it.

## 5. Acceptance Criteria
- Existing and in-memory SQLite databases initialize with gold and market schema.
- Listing consumes exactly the requested inventory quantity; browse supports item type, substring search, and pagination.
- Purchase transfers item and gold and marks the listing Sold atomically; insufficient funds/full inventory leave all state unchanged.
- Only the seller can cancel an Active listing; cancellation restores the item atomically.
- Authenticated packets are handled by the server and client UI exposes browse, sell, and own listings with gold balance.
- Marketplace codec round trips and malformed payloads are rejected.
- Build succeeds, all 31 CTest tests pass, server/client smoke tests pass, and reports/review are written before PLAN.md is checked.
