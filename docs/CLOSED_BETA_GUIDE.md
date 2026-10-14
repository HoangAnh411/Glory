# Closed Beta Operations Guide

## Start the server

Build with `cmake -S . -B build -G Ninja` and `cmake --build build`, then launch:

```powershell
./build/glory_server.exe --port 7777 --database data/accounts.sqlite3 --log-level INFO --log-file data/logs/server.log --backup-dir data/backups --backup-interval 3600
```

The server listens on UDP port 7777 by default. Keep the database and backup directories on persistent storage. To export feedback for review:

```powershell
./build/glory_server.exe --database data/accounts.sqlite3 --export-feedback reports/closed-beta-feedback.json
```

The export command writes all submissions as JSON and exits. Backups can be verified with `--backup-now`.

## Start the client

Run `./build/glory_client.exe --connect 127.0.0.1:7777` for a local server, or replace the address with the beta server address. Register or log in, create/select a character, then enter the world. Press F8 to open the Closed Beta Feedback form.

## Beta play checklist

- [ ] Create and play each of the three character classes; note movement, survivability, and skill feel.
- [ ] Fight the world Boss and exercise its attack phases, dodge windows, and rewards.
- [ ] Enter a Dungeon Instance, complete the encounter, and verify party member state and rewards.
- [ ] Play PvP Arena matches in both 1v1 and 3v3 queues.
- [ ] List, browse, buy, and cancel items in the trading market.
- [ ] Exercise party invitations, chat, guild creation, invitations, and membership changes.
- [ ] Submit at least one actionable feedback report through F8, including reproduction steps where possible.

## Feedback collection and release preparation

1. Ask testers to include expected versus observed behavior, reproducible steps, and the character/class involved. The client attaches OS, renderer, average FPS, RTT, zone, position, class, and level diagnostics.
2. Export feedback after each test session with `--export-feedback`, then triage Bug reports by severity and group Balance/Suggestion notes by feature.
3. Reproduce high-priority reports, record fixes and retest evidence, and communicate known issues to beta participants.
4. Before an official release, run the full CTest suite and client/server smoke checks, verify a fresh database backup, review the feedback backlog, and prepare rollback and support procedures.
