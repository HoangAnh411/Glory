# Báo cáo TESTER: Chống gian lận phía server

## Lệnh đã chạy

- `cmake -S . -B build -G Ninja` — PASS.
- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 33/33 CTest.
- `.\build\glory_anti_cheat_tests.exe` — PASS, gồm kiểm tra cảnh báo reliable tới client trước khi kết nối loopback bị kick.
- `.\build\glory_server.exe --smoke-test` — PASS, 120 tick server headless ở 60 Hz.
- `.\build\glory_client.exe --smoke-test` — PASS, OpenGL 4.5 và 120 fixed tick.
- `git diff --check` — PASS, không có lỗi whitespace.

## Các tiêu chí đã kiểm tra

- Speed vượt mức cho phép bị từ chối; vị trí authoritative không đổi và bộ đếm Speed tăng — PASS.
- Dịch chuyển 50 m trong một tick bị phân loại Teleport; vị trí authoritative được giữ nguyên — PASS.
- Di chuyển xuyên vật cản trong `CollisionWorld` bị từ chối — PASS.
- Dịch chuyển dọc bất thường vượt giới hạn vận tốc nhảy bị tính là Speed violation — PASS.
- Skill `heavy_strike` lần đầu được nhận; yêu cầu lại khi cooldown còn thời gian bị từ chối và HP boss không đổi — PASS.
- Vi phạm lặp lại đủ ngưỡng đưa session vào danh sách kick; cảnh báo reliable tới client rồi `NetworkServer::disconnect` đóng kết nối loopback — PASS.
- Codec cảnh báo anti-cheat, probe vị trí và skill request round-trip — PASS.
- Hồi quy toàn bộ 33 test hiện được CTest đăng ký — PASS.
- Smoke test độc lập của `glory_server` và `glory_client` — PASS.

## Ghi chú terminal

Configure có cảnh báo CMake từ dependency FetchContent/GLM và thiếu PkgConfig/LibUSB tùy chọn; configure, build và test vẫn hoàn tất với exit code 0. Không có lỗi còn mở.
