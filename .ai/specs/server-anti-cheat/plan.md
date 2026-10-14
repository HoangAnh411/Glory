# Kế hoạch: Chống gian lận phía server

## 1. Mục tiêu

Thêm kiểm tra chuyển động và hành động trong `ServerSimulation`, lưu bộ đếm vi phạm theo session, snapback khi phát hiện dịch chuyển bất thường, từ chối skill đang hồi chiêu và kick session sau ngưỡng vi phạm. Gửi dữ liệu cảnh báo/telemetry qua giao thức mạng và cung cấp tab ImGui để quan sát, phát injection có kiểm soát. Logic kiểm tra đặt trong server/game, độc lập renderer.

## 2. Danh sách task nhỏ

1. Tạo `ServerAntiCheat` thuần C++ trong module server: kiểm tra vận tốc từ vị trí authoritative giữa tick, phân loại speed/teleport/collision, kiểm tra cooldown và quản lý ngưỡng kick; cung cấp telemetry hiện tại.
2. Tích hợp tracker theo từng `PlayerSession` trong `ServerSimulation`: kiểm tra sau bước controller; khi vi phạm vị trí thì phục hồi transform hợp lệ, đặt vận tốc về 0 và cập nhật số vi phạm; chặn action cooldown trước khi `CombatSystem` có thể gây damage; expose telemetry/kick state cho server loop và unit test.
3. Mở rộng network protocol bằng packet anti-cheat có phiên bản/giới hạn payload để gửi cảnh báo, chỉ số vi phạm và yêu cầu disconnect có lý do; server loop gửi packet và ngắt peer khi session vượt ngưỡng. Client giải mã và hiển thị cảnh báo.
4. Bổ sung tab `Anti-Cheat Monitor` vào `DebugPanel`: vận tốc, giới hạn cho phép, số lần vi phạm; các nút injection gửi hành động test qua luồng input hiện có. Không cho client authoritative tọa độ.
5. Viết `tests/anti_cheat_tests.cpp`, đăng ký CTest, kiểm tra speed/teleport snapback, cooldown từ chối damage và kick policy. Build, chạy toàn bộ CTest và smoke test server/client.
6. Ghi kết quả thật vào `test-report.md`, rà soát theo plan và `PLAN.md`, ghi `review.md`. Chỉ tick roadmap sau khi build/test PASS.

## 3. File cần sửa / tạo mới

- Tạo `server/include/glory/server/server_anti_cheat.hpp`, `server/src/server_anti_cheat.cpp`.
- Sửa `server/include/glory/server/server_simulation.hpp`, `server/src/server_simulation.cpp`, `server/src/main.cpp`.
- Sửa `network/include/glory/network/network.hpp`, `network/src/network.cpp`.
- Sửa `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp` và `game/src/main.cpp` để truyền/hiển thị telemetry và hành động debug.
- Tạo `tests/anti_cheat_tests.cpp`, sửa `CMakeLists.txt` để đưa source và CTest vào build.
- Sửa `PLAN.md` chỉ ở bước tổng kết, sau khi reviewer xác nhận PASS.
- Tạo `.ai/specs/server-anti-cheat/test-report.md` và `.ai/specs/server-anti-cheat/review.md`.

## 4. Rủi ro tiềm ẩn và giảm thiểu

- Server đang mô phỏng input thay vì nhận vị trí từ client; không thêm vị trí client-authoritative. Speed/teleport được đo trên transform server và API debug chỉ tạo input kiểm thử qua cùng luồng nhận input.
- Dodge, knockback, hiệu ứng tăng tốc có thể vượt tốc độ đi bộ; giới hạn phải dựa trên trạng thái server biết được và tolerance 15%, tránh false positive.
- Teleport server-side hợp lệ (spawn/zone/arena) phải đặt lại baseline anti-cheat để không bị đánh nhầm.
- Cooldown hiện do `CombatSystem` giữ; kiểm tra phải diễn ra trước khi action được bắt đầu và hitbox tạo damage, đồng thời không đổi cooldown thành công.
- Kick cần tránh iterator invalidation khi server loop xóa session; thu thập client IDs rồi disconnect sau vòng lặp.
- Thay đổi protocol phải giữ tương thích decoder hiện tại và giới hạn chuỗi/payload.
- Worktree có nhiều file chưa commit của các giai đoạn trước; chỉ sửa những file liệt kê và giữ nguyên các phần thay đổi sẵn có.

## 5. Tiêu chí chấp nhận

- Server dùng tracker độc lập với renderer, tolerance mặc định 15%, phân biệt speed/teleport/collision và khôi phục vị trí hợp lệ khi vượt giới hạn.
- Mỗi client session có telemetry riêng; sau 8 vi phạm liên tiếp policy đánh dấu kick với lý do `Kicked for anti-cheat violation`.
- Lệnh skill đang cooldown không tạo hit event hoặc sát thương và tăng counter cooldown.
- Packet cảnh báo round-trip được qua codec; server có thể gửi telemetry, snapback warning và kick warning; client xử lý packet.
- Debug panel có tab hiển thị vận tốc/giới hạn/counters và ba nút injection tương ứng.
- `glory_anti_cheat_tests` qua đủ speed, teleport, cooldown, kick; toàn bộ CTest hiện có và mới chạy PASS; `glory_server --smoke-test` và `glory_client --smoke-test` PASS.
- `test-report.md` và `review.md` có kết quả terminal/review thực tế; mục roadmap chỉ tick sau khi đạt các tiêu chí kiểm chứng.
