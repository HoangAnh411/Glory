# Dungeon Instance Implementation Plan

## Goal

Add a renderer-independent dungeon instance lifecycle, authoritative server membership and isolated snapshots, binary protocol, client controls/status panel, and the 30th CTest target. Preserve all existing work and keep normal-zone and arena behavior intact.

## Design

- Add `DungeonManager` to `glory_game`. A template describes the dungeon ID/name, duration, player spawn, boss spawn, and boss maximum HP. Each instance owns an independent timer, boss health, state, and player IDs. It supports solo or party entry, damage, ticking, leave, and cleanup.
- Add bounded dungeon request/response/state packets to `glory_network`. Validate dungeon IDs, results, instance IDs, timers, and health values in codecs.
- Integrate selected-character requests in `glory_server`: validate session ownership; enter the caller or the caller's online party as one group; save each member's origin zone/position; teleport the group to an instance-specific location; route snapshots only to members of that instance; apply authoritative attack input to only that instance boss; tick completion, timeout, and wipe states; restore members on leave/terminal state; clean up empty/closed instances.
- Add `DungeonPanelState` and callbacks to the client application/debug panel. Show available dungeon, solo/party entry, leave, dungeon state, timer, boss health, and clear banner. Handle response/state packets in the client.
- Add `tests/dungeon_instance_tests.cpp` and register `glory_dungeon_instance_tests` as CTest 30. Cover isolation, lifecycle, cleanup, and protocol round-trip/bounds.

## Files

- `game/include/glory/game/dungeon.hpp`, `game/src/dungeon.cpp`
- `network/include/glory/network/network.hpp`, `network/src/network.cpp`
- `server/include/glory/server/account_database.hpp`, `server/src/account_database.cpp`, `server/src/main.cpp`
- `engine/include/glory/engine/application.hpp`, `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp`, `engine/src/application.cpp`
- `game/src/main.cpp`
- `CMakeLists.txt`, `tests/account_database_tests.cpp`, `tests/dungeon_instance_tests.cpp`
- `.ai/specs/dungeon-instance/{plan.md,test-report.md,review.md}`, `PLAN.md`

## Acceptance

- Two instances of the same dungeon have distinct IDs and independent boss HP/timers/state.
- Solo and party entry assign only the intended online characters. Snapshots contain only same-instance players and that instance's boss; non-instance snapshots exclude dungeon participants.
- Boss defeat, timer expiration, and all-members-defeated transition to the proper terminal state; leaving/terminal return restores each member to their saved open-zone location and removes empty instances safely.
- Packet codecs round-trip valid requests/responses/state and reject malformed/oversized data.
- Dungeon UI exposes entry, leave, timer, boss health, and completion state.
- Build passes, all 30 CTest tests pass, and server/client smoke tests complete.

## Assumptions and boundaries

- The first dungeon is a single template named `The Shadow Crypt`; party entry uses the current party membership and requires every member to be online and in the same open zone.
- Dungeon invites/membership are runtime-only. Completion grants EXP through the existing character `exp` field and a configured item through the existing inventory persistence; no new reward schema is introduced.
- Existing server combat does not expose per-client hit events. Dungeon boss damage will be applied server-side from validated primary-attack input for a member in the same instance and within the encounter range; boss health remains instance-owned and does not use the shared world boss.
