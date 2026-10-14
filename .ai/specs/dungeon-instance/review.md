# Review: Dungeon Instance

## Must fix

- None found in the reviewed scope.

## Should fix

- Add a multi-client ENet integration scenario for party entry, cross-instance snapshot isolation, boss defeat, and reward delivery. The current unit suite tests the manager/protocol/database behavior independently; smoke tests do not exchange dungeon packets between clients.
- Dungeon boss damage currently uses a server-side range check, primary attack input, and a 30-tick cooldown with fixed damage. Before production, connect this encounter to validated server combat hit events and per-instance boss AI.

## Suggestions

- Move dungeon templates and rewards into validated data files when more than the initial Shadow Crypt is introduced.
- Add explicit server telemetry for instance creation, completion, failure, and cleanup.

## Review checks

- Instance IDs map to distinct boss health, timers, and spawn coordinates. Server snapshots include only the recipient's instance members and that instance boss; open-world snapshots filter active dungeon participants.
- Entry and leave require a session that owns the selected character. Party entry requires all members to be online, in the same zone, and not already assigned to an instance.
- Completion grants configured EXP and an inventory item through existing persistence. If inventory persistence cannot accept the item, the item drops at the saved origin. Completion/failure restores each member to the saved position; empty instances are reclaimed.
- Packet codecs validate IDs, enum results, flags, sizes, timers, and boss health ranges. Dungeon lifecycle rules remain in `glory_game`, outside the renderer.
- Validation: CMake configure/build PASS, 30/30 CTest PASS, server and client smoke tests PASS (details in `test-report.md`).
