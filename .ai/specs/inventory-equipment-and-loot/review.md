# Review: Nhân vật, inventory, trang bị, rơi đồ

## Phạm vi đã rà soát

Đối chiếu `plan.md` và quy tắc kiến trúc trong `PLAN.md`; rà soát game logic, schema/migration SQLite, xử lý request phía server, codec packet, DebugPanel/client và test mới.

### 1. Phải sửa

- Không phát hiện vấn đề chặn nghiệm thu.

### 2. Nên sửa

- `AccountDatabase::saveCharacterState` xác thực chỉ số slot và quantity dương, còn tính hợp lệ theo catalog (item ID tồn tại, quantity không vượt `maxStack`, loại item khớp equipment slot) hiện được kiểm tra ở lớp game khi restore. Có thể gom thêm validation catalog ở tầng persistence nếu sau này có đường ghi DB khác ngoài runtime đã kiểm tra.

### 3. Gợi ý

- Bổ sung integration test qua ENet cho vòng đời chọn character → equip/pickup → reconnect → xác nhận state đã lưu. Hiện codec và persistence được test riêng, còn hai smoke test xác nhận startup/tick.
- UI DebugPanel hiển thị loot sync và các thao tác chính; có thể bổ sung hiển thị chi tiết kết quả lỗi theo từng mã action khi UI được mở rộng.

## Kết luận

Đạt plan và đủ điều kiện tick roadmap. Server giữ quyền quyết định state, kiểm tra session cùng character ownership; pickup kiểm tra vị trí; thay đổi inventory/equipment chỉ được áp dụng vào runtime sau khi ghi SQLite thành công. Kết quả build, 27/27 CTest và smoke test server/client đều PASS.
