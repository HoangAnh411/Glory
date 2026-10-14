# Review: Chat, Party, Guild

## Must fix

- None found in the reviewed scope.

## Should fix

- Add a multi-client ENet integration test in a later networking pass. Current tests cover pure routing, social managers, database operations, and codecs independently; smoke tests only exercise application startup and fixed-tick execution.

## Suggestions

- Consider persisting party membership only if the product later needs parties to survive disconnects or server restarts. Current party state is intentionally runtime-only.
- Consider adding moderation controls and rate limits before exposing chat to public servers.

## Review checks

- Sender identity and chat recipients are derived server-side from the active character roster; client-supplied sender names and System messages are not trusted.
- Social requests require a valid session that owns the selected character. Party membership changes are synchronized to party members, and member vitals are refreshed periodically.
- Guild membership and rank mutations use the database API, and guild sync is refreshed for affected current and former members, including after leader departure.
- Packet codecs bound string/list/payload sizes; game rules live in `glory_game`, separate from ImGui rendering.
- Validation evidence: build PASS, 29/29 CTest PASS, server smoke PASS, client smoke PASS (see `test-report.md`).
