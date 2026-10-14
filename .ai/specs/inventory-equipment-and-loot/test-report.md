# Báo cáo kiểm thử: Nhân vật, inventory, trang bị, rơi đồ

## Lệnh đã thực thi

- `cmake --build build` — exit code 0; build đủ các target liên quan, gồm `glory_server`, `glory_client` và `glory_inventory_equipment_tests`.
- `ctest --test-dir build --output-on-failure` — exit code 0; 27/27 test passed.
- `.\build\glory_server.exe --smoke-test --port 17780 --database :memory:` — exit code 0.
- `.\build\glory_client.exe --smoke-test` — exit code 0.

## Kết quả tổng quan

PASS. Không có test nào thất bại.

## Chi tiết

- Inventory: thêm stack theo `maxStack`, xóa, di chuyển/merge, giới hạn 24 ô và từ chối thao tác không hợp lệ — Passed.
- Equipment: chỉ mặc đúng loại, tháo an toàn và tính Attack/Defense/Max HP — Passed.
- SQLite: lưu/load giữ slot, item, quantity, equipment slot; từ chối token sai và slot trùng — Passed.
- Loot: spawn, giữ drop khi ở ngoài tầm hoặc inventory đầy, nhặt trong tầm — Passed.
- Network codec: round-trip inventory/equipment/equip/unequip/world loot/pickup/action response; từ chối payload bị cắt — Passed.
- Regression: toàn bộ 27 CTest, bao gồm account database và inventory/equipment — Passed.
- Server smoke: `Headless server smoke test passed: 120 fixed ticks at 60 Hz` — Passed.
- Client smoke: `SDL3/OpenGL 4.5 context and 120 fixed ticks passed` — Passed.

## Lỗi

Không có.
