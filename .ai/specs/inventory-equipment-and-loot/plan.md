# Kế hoạch: Nhân vật, inventory, trang bị, rơi đồ

## 1. Mục tiêu

Thêm item catalog JSON cùng các hệ thống inventory/equipment/loot thuần logic vào `glory_game`; mở rộng SQLite để lưu trạng thái của từng character; đồng bộ hành động authoritative qua ENet; hiển thị và thao tác từ DebugPanel. Dữ liệu gameplay không phụ thuộc renderer.

## 2. Danh sách task nhỏ

1. Tạo item definition/catalog, Inventory 24 ô, EquipmentManager 3 slot và LootDropSystem trong game module; viết JSON item mẫu.
2. Mở rộng AccountDatabase bằng bảng `inventories`, `equipments`, migration an toàn khi mở database cũ, kiểm tra quyền sở hữu character, và lưu/đọc hai tập dữ liệu trong transaction.
3. Thêm codec cho inventory/equipment sync, equip/unequip, world loot sync, pickup, simulate drop và action response; xác thực giới hạn packet/list/string.
4. Nối server với ItemCatalog và runtime theo selected character; load trang bị/túi khi chọn nhân vật, xác thực session/ownership ở equip/unequip/pickup, lưu thay đổi trước khi sync, phát loot khi boss chết và hỗ trợ nút mô phỏng loot qua request xác thực.
5. Thêm InventoryPanelState/callback không phụ thuộc game logic vào engine; client nạp item catalog, render túi đồ/slot/stats/loot và gửi thao tác qua mạng.
6. Viết `tests/inventory_equipment_tests.cpp`: JSON, add/remove/move, equip/unequip/stat, SQLite save/load, boss loot/pickup theo khoảng cách, codec round-trip.
7. Build, chạy đủ 27 CTest và smoke test server/client. Sửa lỗi nếu có theo tối đa 3 vòng; reviewer kiểm tra ownership, transaction và authoritative gameplay. Chỉ sau khi PASS mới tick roadmap.

## 3. File cần sửa / tạo mới

- `CMakeLists.txt`
- `assets/data/items.json`
- `game/include/glory/game/inventory.hpp` (ItemCatalog, Inventory, EquipmentManager, LootDropSystem)
- `game/src/inventory.cpp`
- `server/include/glory/server/account_database.hpp`
- `server/src/account_database.cpp`
- `server/src/main.cpp`
- `network/include/glory/network/network.hpp`
- `network/src/network.cpp`
- `engine/include/glory/engine/application.hpp`
- `engine/include/glory/engine/debug_panel.hpp`
- `engine/src/application.cpp`
- `engine/src/debug_panel.cpp`
- `game/src/main.cpp`
- `tests/inventory_equipment_tests.cpp`
- `.ai/specs/inventory-equipment-and-loot/{plan.md,test-report.md,review.md}`
- `PLAN.md` (tick đúng mục sau khi Tester PASS và Reviewer không ghi mục bắt buộc sửa)

## 4. Rủi ro tiềm ẩn

- DB account hiện có cần nâng schema không phá dữ liệu; thêm `CREATE TABLE IF NOT EXISTS`, khóa ngoại và unique(character_id, slot_index/slot_type).
- Equip/unequip/pickup có thể làm mất item nếu save thất bại; thao tác trên bản sao runtime, ghi SQLite transaction rồi mới áp dụng/broadcast state.
- Client không được tự quyết định ownership, vị trí pickup, item bonus hoặc loot; server kiểm tra session token, character đã chọn, khoảng cách và catalog.
- Kích thước ENet tối đa 1200 bytes; giới hạn 24 inventory slots, tối đa 3 equipment slots và số loot entries trong mỗi sync.
- Mỗi account có tối đa 16 character từ bước trước; mỗi character inventory đúng 24 slot, stack phải nằm trong `maxStack`.
- Boss death có thể được quan sát nhiều tick; chỉ tạo drop khi HP chuyển từ dương sang không dương. Nút mô phỏng chỉ được server xử lý khi session đã chọn character.
- Renderer/UI chỉ tiêu thụ view model và callback; không đặt game rules trong engine.

## 5. Tiêu chí chấp nhận

- `assets/data/items.json` có đủ trường id/name/type/rarity/bonusAttack/bonusDefense/bonusMaxHp/maxStack; parser từ chối item ID trùng, enum sai và stack không hợp lệ.
- Inventory có 24 slot; add/remove/move xử lý stack, giới hạn slot và không làm mất dữ liệu khi thao tác thất bại.
- Equipment chỉ nhận đúng loại item, hỗ trợ thay/tháo an toàn; bonus Attack/Defense/Max HP được tính chính xác.
- Loot giữ item ID, quantity, vị trí; pickup trong tầm gần vào inventory, ngoài tầm hoặc túi đầy không xóa drop.
- SQLite migration tạo inventories/equipments; save/load giữ nguyên slot, số lượng, slot type và item ID; thay đổi được commit atomic và ràng buộc theo character.
- Server load loadout khi chọn nhân vật; thao tác mạng kiểm tra ownership/session và vị trí pickup; state chỉ phát sau khi persist thành công.
- DebugPanel có lưới 24 ô, slot equipment, base/bonus stats, Equip/Unequip, Simulate Boss Loot Drop và Pick Up.
- Codec các packet mới round-trip và từ chối payload lỗi; build thành công, 27/27 CTest và smoke test server/client PASS.
- Có test report/review; roadmap chỉ tick sau khi đủ vai trò và kiểm thử PASS.
