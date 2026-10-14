# Báo cáo kiểm thử ENet UDP

## Kết quả tổng quan

**PASS** — Configure, build, 20/20 CTest và smoke test server/client đều thành công trên Windows 11 với Ninja và LLVM-MinGW Clang.

## Lệnh đã chạy

- `cmake -S . -B build -G Ninja` — exit code 0.
- `cmake --build build` — exit code 0.
- `ctest --test-dir build --output-on-failure` — 20/20 test PASS, 0 FAIL.
- `./build/glory_server.exe --smoke-test --port 17777` — exit code 0; server mở UDP port 17777, chạy đủ 120 tick và báo `Headless server smoke test passed`.
- `./build/glory_client.exe --smoke-test` — exit code 0; SDL3/OpenGL context và 120 fixed ticks PASS.

## Test ENet loopback

`glory_network` PASS trên `127.0.0.1` với port ephemeral:

- ENet connect và reliable handshake, ClientId được gán: PASS.
- ClientInput chuyển qua cả reliable channel 0 và unreliable sequenced channel 1: PASS.
- ServerSnapshot response được client nhận: PASS.
- Ping/Pong và đo RTT: PASS.
- Client/server quan sát disconnect: PASS.
- Encode/decode input buttons và camera yaw: PASS.

## Ghi chú

CMake báo warning upstream hiện có về chính sách tương thích GLM và cách gọi FetchContent của miniaudio/ImGui; các warning không làm configure/build thất bại. Smoke test server dùng port 17777 để tránh xung đột với service có thể đang chiếm port mặc định 7777.
