# Review: Headless Dedicated Server

## 1. Phải sửa

- Không phát hiện lỗi bắt buộc. `glory_server` compiles its simulation source directly and links only `glory_game`; all simulation code uses game types and standard C++ timing, with no engine or platform rendering dependency.

## 2. Nên sửa

- Move the client callback's remaining gameplay session orchestration into a shared `glory_game` session abstraction in a later refactor. The dedicated server currently composes the same game-domain classes in its own headless session.
- Add an integration test for SIGINT/SIGTERM shutdown and real-time scheduler drift; current automated tests verify deterministic fixed dt/tick accumulation, while smoke verifies a real 120-tick run.

## 3. Gợi ý

- Add server-side input validation and per-player sessions alongside the upcoming transport/netcode work.
- Emit periodic structured logs and operational metrics once the server is deployed beyond local smoke testing.

## Kết luận

Đạt mục tiêu của bước này. `ServerSimulation` nạp world/skills JSON và cập nhật controller, collision, combat, boss, status và trigger ở 60 Hz. CLI hỗ trợ `--smoke-test`, `--ticks N` và chạy liên tục đến tín hiệu dừng. Build, 19 CTest, server smoke và client smoke đều PASS.
