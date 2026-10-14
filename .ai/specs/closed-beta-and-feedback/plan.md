# Closed Beta Feedback — Implementation Plan

## 1. Objective

Provide authenticated closed beta players with a bounded feedback form, persist submissions in SQLite with client diagnostics, and let operators export submissions as valid JSON. Preserve existing packet IDs and build/test conventions.

## 2. Tasks

1. Append request/response packet IDs and bounded binary codecs. Reject invalid category, rating, lengths, malformed UTF-8-independent payload boundaries, and packets over the ENet size limit.
2. Extend `AccountDatabase` with a feedback schema, authenticated character ownership validation, parameterized insertion, and JSON export with correct escaping.
3. Integrate feedback packet handling in the server: require selected/authenticated character, limit each authenticated account to one accepted submission per 60 seconds (including reconnects), and always answer valid requests with a response status.
4. Add a closed beta feedback window toggled by F8 in the ImGui debug panel. Collect platform, OpenGL renderer, smoothed FPS, RTT, zone, position, class, and level at submission time; display the server response.
5. Add `--export-feedback <path.json>` to the server, documentation for beta operation and gameplay checklist, and CTest coverage for validation, persistence/export, and codec boundaries.
6. Build, run all 36 CTests, exercise export CLI and server/client smoke tests, then record actual test and review results. Tick the final roadmap item only after all required checks pass.

## 3. Files to touch

- `.ai/specs/closed-beta-and-feedback/{plan.md,test-report.md,review.md}`
- `network/include/glory/network/network.hpp`, `network/src/network.cpp`
- `server/include/glory/server/account_database.hpp`, `server/src/account_database.cpp`, `server/src/main.cpp`
- `engine/include/glory/engine/application.hpp`, `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp`
- `game/src/main.cpp`
- `tests/closed_beta_feedback_tests.cpp`, root `CMakeLists.txt`
- `docs/CLOSED_BETA_GUIDE.md`, root `PLAN.md`

## 4. Risks & Mitigations

- Existing ENet packets are limited to 1200 bytes. Set explicit subject/description/diagnostics limits whose encoded sum remains below that bound; decoders reject oversized/trailing data.
- Feedback must not be spoofable across accounts. Resolve the account from the server-side session token and require the selected character to belong to that account; never accept account IDs from the client.
- JSON export must tolerate quotes, backslashes, and control characters. Use tested JSON escaping and report directory/write/SQLite errors to the caller.
- Existing worktree contains prior uncommitted stages. Keep changes limited to this feature and inspect diffs before finalizing.
- Client UI needs renderer and platform information without moving gameplay logic into rendering; gather diagnostics at UI submission and only send them as untrusted diagnostic text.

## 5. Acceptance Criteria

- Feedback request and response round-trip through binary codecs; invalid ratings/categories/lengths and malformed payloads are rejected.
- SQLite stores feedback only for a valid authenticated session owning the submitted character; export produces parseable JSON with all stored fields and diagnostics.
- Server rate limits submissions to one per authenticated account per 60 seconds and replies with `Success`, `InvalidLength`, or `RateLimited`.
- F8 opens the feedback form; it shows category, rating, subject/description inputs and collected diagnostics, sends through the authenticated network session, and reports success/failure.
- `glory_server --export-feedback <path.json>` exports and exits with a meaningful status.
- `docs/CLOSED_BETA_GUIDE.md` documents server/client operation, the requested core feature checklist, feedback collection, and launch preparation.
- New `glory_closed_beta_feedback` test is CTest #36; full build and all 36 tests pass, and server/client smoke tests pass before the roadmap checkbox is ticked.
