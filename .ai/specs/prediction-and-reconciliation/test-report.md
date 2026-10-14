# Báo cáo kiểm thử: Client prediction và reconciliation

Ngày chạy: 2026-10-14

## Lệnh đã thực thi

- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 23/23 tests.
- `./build/glory_server.exe --smoke-test --port 17779` — PASS, 120 fixed ticks ở 60 Hz, exit code 0.
- `./build/glory_client.exe --smoke-test` — PASS, SDL3/OpenGL 4.5 và 120 fixed ticks, exit code 0.

## Kiểm thử prediction/reconciliation

- Input sequence và authoritative ACK round-trip qua binary codec: PASS.
- Snapshot truyền ACK, grounded/dodge và knockback state: PASS.
- Client đổi vị trí ngay khi predict, không chờ snapshot: PASS.
- ACK chỉ xóa input đã xác nhận, input mới hơn vẫn nằm trong buffer: PASS.
- Repeated ACK không so authoritative state với vị trí dự đoán hiện tại sai tick: PASS.
- Misprediction vượt 0.05 m rewind về state authoritative và replay input còn chờ, gồm knockback impulse: PASS.
- Server authoritative test xác nhận sequence đã xử lý xuất hiện trong snapshot gửi về client: PASS.

## Tổng quan

Build và toàn bộ 23 CTest đều PASS (`100% tests passed out of 23`). Server và client smoke-test đều PASS. Trong lần build đầu, compiler phát hiện thiếu include GLM `vec2` trong phần khôi phục hướng ngang; đã sửa và build cuối cùng thành công. Không còn lỗi test.
