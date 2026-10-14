# Kế hoạch: Chia zone và interest management

## Mục tiêu

Thêm spatial grid 2D theo zone vào `glory_game`, lọc snapshot và loot theo AOI cho từng client, hỗ trợ chuyển zone do server xác thực và cung cấp trạng thái/nút thử nghiệm qua DebugPanel. Logic vùng và truy vấn không phụ thuộc renderer.

## Thiết kế

1. Tạo `game/include/glory/game/spatial_grid.hpp` và `game/src/spatial_grid.cpp`:
   - `ZoneId` với các zone Plains, Arena, Forest; `SpatialEntity` gồm ID, loại, zone, vị trí và dữ liệu render/network tùy ứng dụng giữ ở module riêng.
   - `SpatialGrid` cell 32m theo X/Z, tọa độ cell bằng floor để hỗ trợ vị trí âm; hỗ trợ upsert, remove, query theo zone/radius với kiểm tra khoảng cách bình phương.
2. Mở rộng snapshot/packet:
   - Thêm zone hiện tại cùng tổng entity server và entity AOI nhìn thấy vào snapshot.
   - Thêm request đổi zone kèm session/character/target và notification authoritative kèm zone/spawn position.
   - Giới hạn payload theo giới hạn packet ENet hiện hữu; decode từ chối enum, số đếm, tọa độ sai.
3. Server:
   - Duy trì zone cho mỗi kết nối và spawn point cố định theo zone.
   - Chỉ nhận yêu cầu chuyển zone nếu session sở hữu character đã chọn; teleport player, cập nhật zone rồi gửi notification.
   - Mỗi tick dựng spatial grid từ players, boss và loot; cho từng client query bán kính 40m cùng zone, chỉ gửi player/boss tương ứng; gửi world loot sync theo cùng zone/AOI.
   - Không dùng broadcast chung cho snapshot hoặc loot đã lọc.
4. Client/UI:
   - Thêm ZonePanelState/callback vào application interface và tab Zone & AOI có zone, server entity count, visible count, teleport Zone 1/2.
   - Nhận zone notification; khi đổi zone thì reset interpolation/proxy list và cập nhật state trước snapshot zone mới.
5. Test: `tests/zone_interest_tests.cpp` kiểm thử cell âm/dương, AOI cập nhật khi entity di chuyển/đổi zone, chuyển zone/spawn, snapshot codec và zone codec; đăng ký CTest số 28.

## File dự kiến

- `CMakeLists.txt`
- `game/include/glory/game/spatial_grid.hpp`, `game/src/spatial_grid.cpp`
- `network/include/glory/network/network.hpp`, `network/src/network.cpp`
- `server/include/glory/server/server_simulation.hpp`, `server/src/server_simulation.cpp`, `server/src/main.cpp`
- `engine/include/glory/engine/application.hpp`, `engine/include/glory/engine/debug_panel.hpp`, `engine/src/application.cpp`, `engine/src/debug_panel.cpp`
- `game/src/main.cpp`
- `tests/zone_interest_tests.cpp`
- `.ai/specs/zone-and-interest-management/{plan.md,test-report.md,review.md}`
- `PLAN.md` chỉ tick sau build/test/review PASS.

## Tiêu chí nghiệm thu

- Spatial grid xác định cell đúng với floor, trả đúng entity thuộc zone trong radius, cập nhật/xóa không để lại entry cũ.
- Snapshot/loot từng client chỉ chứa entity trong zone và radius; thống kê visible không vượt tổng count.
- Zone transition xác thực session/ownership, cập nhật authoritative spawn, client dọn proxy/interpolation zone cũ.
- Zone & AOI panel cung cấp trạng thái và nút teleport 1/2.
- Codec round-trip và từ chối payload sai; build, 28 CTest, server/client smoke PASS.
