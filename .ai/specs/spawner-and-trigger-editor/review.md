# Review: World Spawner and Trigger Editor

## 1. Phải sửa

- Không phát hiện lỗi bắt buộc sau khi đối chiếu code với plan, build/test và kiến trúc `PLAN.md`.

## 2. Nên sửa

- Thêm integration test cho callback hot-reload lỗi để xác nhận các vector editor và trạng thái encounter giữ nguyên khi file JSON hỏng. Hiện callback parse, chuyển đổi và validate dữ liệu tạm trước khi commit.
- Mở cửa sổ client để kiểm tra trực tiếp bố cục cửa sổ World Editor, thao tác Snap, Save và Reload; smoke test xác nhận OpenGL/client loop chạy nhưng không mô phỏng thao tác chuột.

## 3. Gợi ý

- Spawn point `PracticeDummy` hiện được lưu, chỉnh, snap và hiển thị vị trí/yaw; dự án chưa có actor practice dummy riêng để spawn/render/combat. Có thể thêm entity runtime ở một bước gameplay sau.
- `EncounterBoundary` hiện phát transition và hiển thị trạng thái; hành vi khóa/rời arena có thể được xác định khi thiết kế encounter cụ thể.

## Kết luận

Đạt kế hoạch. `WorldEncounter` ở `glory_game` xử lý JSON, validation, geometry và Enter/Inside/Exit không phụ thuộc SDL/OpenGL. DebugPanel ở engine dùng DTO/callback; client áp dụng Player/Boss spawn, bật boss bằng BossAggro, hồi máu bằng HealArea và trừ máu khi vào HazardArea. Build, 18 CTest và smoke test đều PASS.
