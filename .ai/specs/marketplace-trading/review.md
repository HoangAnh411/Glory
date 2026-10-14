# Marketplace Trading Review

## 1. Phải sửa
- Không phát hiện lỗi bắt buộc còn tồn tại. Gold, inventory, and listing status changes execute inside SQLite `BEGIN IMMEDIATE` transactions and roll back when validation fails. Server handlers verify the session and selected character before performing character-scoped actions.

## 2. Nên sửa
- None for this milestone. The binary codecs bound string, page-size, and listing-count inputs; inventory restoration is refreshed from committed DB state after market mutations.

## 3. Gợi ý
- Add a total-result count to browse responses so the client can disable Next at the last page.
- As social and marketplace persistence APIs grow, consider extracting transaction orchestration from `AccountDatabase` into dedicated service classes while keeping the SQLite operations atomic.

## Reviewer decision
PASS. The implementation meets the marketplace plan and the repository architecture rules: domain mutations are server-authoritative, rendering remains in the engine UI, old character databases receive a safe additive gold migration, and tests/build/smoke checks pass.
