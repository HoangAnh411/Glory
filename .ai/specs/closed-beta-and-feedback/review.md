# Review — Closed Beta Feedback

## 1. Phải sửa (Critical / Required)

- Không còn mục phải sửa. Feedback packets use bounded lengths and append their packet IDs without renumbering existing protocol values. Server persistence resolves the account from its own session map, confirms character ownership, validates diagnostics JSON, rate limits at the authenticated account level, and uses parameterized SQLite statements. Export escapes user text and refuses to overwrite the open source database.

## 2. Nên sửa (Recommended)

- Add a live ENet integration test that logs in, enters a character, submits feedback, observes `Success`, reconnects with the same account, and observes `RateLimited`. Current coverage tests the codec, database ownership/rate limit, CLI export, and server/client startup separately; it does not exercise the entire feedback packet dispatch over ENet.
- Consider a configurable retention/administrative deletion policy for feedback records before a public beta with long-running storage.

## 3. Gợi ý (Suggestions)

- Add operator documentation for protecting exported feedback files because they may contain player-provided text and hardware/platform diagnostics.
- Consider adding a user-visible maximum length counter to the ImGui subject and description fields; the protocol rejects values over 120 and 600 bytes respectively.

## Review Result

**Approved.** The implementation matches the accepted plan and root roadmap constraints. Build, 36/36 CTests, server/client smoke tests, and CLI JSON export checks pass. The interactive F8 flow is compile-verified but was not manually exercised in this automated environment.
