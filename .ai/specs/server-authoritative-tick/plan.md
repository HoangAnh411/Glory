# Server Authoritative Tick Plan

## 1. Mục tiêu

Mở rộng server fixed tick 60 Hz thành authoritative multiplayer simulation: session riêng theo ClientId, input áp dụng cho đúng người chơi, xử lý movement/collision/dodge/PvP trên server, broadcast snapshot world có tick/entity state và hiển thị state authoritative trên client.

## 2. Tasks

1. Bổ sung schema `ServerSnapshotPacket` cùng encode/decode nhị phân có kiểm tra finite, giới hạn entity/string/packet; giữ payload ENet an toàn với MTU.
2. Thêm player sessions vào `ServerSimulation`, cấp spawn lân cận, lưu input theo client, cập nhật từng CharacterController/CombatSystem mỗi tick và dọn session khi disconnect.
3. Tích hợp NetworkServer nhận input riêng từng client, chạy tick cố định 60 Hz, broadcast full snapshot và giữ world/boss state.
4. Client giải mã snapshot mới nhất, hiển thị server tick/count/authoritative position/HP trong DebugPanel; không dùng server packet để gửi ngược vị trí hoặc sát thương.
5. Thêm `server_authoritative_tests`: hai client ENet loopback, movement độc lập, PvP damage, snapshot HP và disconnect cleanup.
6. Chạy configure/build, 21 CTest và smoke test server/client; ghi báo cáo, review và chỉ sau đó tick PLAN.md.

## 3. Files

- Sửa `network/include/glory/network/network.hpp`, `network/src/network.cpp`.
- Sửa `server/include/glory/server/server_simulation.hpp`, `server/src/server_simulation.cpp`, `server/src/main.cpp`.
- Sửa `game/include/glory/game/combat.hpp`, `game/src/combat.cpp`, `game/src/main.cpp`.
- Sửa `engine/include/glory/engine/application.hpp`, `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp`, `engine/src/application.cpp`.
- Sửa `CMakeLists.txt`; tạo `tests/server_authoritative_tests.cpp`.
- Tạo báo cáo trong `.ai/specs/server-authoritative-tick/`.

## 4. Rủi ro và giảm thiểu

- Snapshot có kích thước lớn hơn datagram an toàn: giới hạn 8 player, độ dài string và tổng payload, từ chối dữ liệu vượt giới hạn.
- Nhiều combat instances có thể xử lý target khác nhau: cập nhật target hurtbox/HP từ authoritative session mỗi tick và chỉ áp dụng damage từ server CombatSystem.
- Spawn gần nhau có thể gây overlap: phân bố player theo offset có deterministic quanh Player spawn.
- Client input không đáng tin: nhận chỉ button masks/yaw, validate finite yaw và không nhận position/HP/damage từ client.
- Input tick có thể thiếu: giữ trạng thái held gần nhất theo session và xóa khi disconnect.

## 5. Tiêu chí nghiệm thu

- Hai ClientId có entity/controller/HP/combat độc lập và spawn riêng; một client di chuyển không đổi state client còn lại.
- Server tự tính collision, dodge/I-frame và combat damage/knockback tại fixed 60 Hz.
- Snapshot encode/decode giữ serverTick, players (position, velocity, yaw, HP, animation, dodge/invulnerability, active skill) và boss state; malformed/oversize/non-finite data bị từ chối.
- Loopback test hai client chứng minh movement, PvP hit/HP snapshot và cleanup disconnect.
- Client DebugPanel hiện server tick/count và authoritative player position/HP.
- Build, toàn bộ 21 CTest, smoke-test server/client PASS; ghi report/review rồi tick đúng roadmap item.
