# Review: Skill Editor and Hot-Reload

## 1. Phải sửa

- Không phát hiện lỗi bắt buộc sau khi đối chiếu implementation với plan và quy tắc kiến trúc trong `PLAN.md`.

## 2. Nên sửa

- Mở cửa sổ game để kiểm tra thủ công thao tác nhập chuỗi, kéo timeline và nhấn F5 trên máy có desktop; build và smoke test hiện xác nhận biên dịch/khởi tạo renderer nhưng không tự động xác nhận tương tác ImGui.
- Bổ sung test cho trường hợp file JSON reload lỗi để khẳng định registry đang chạy không đổi; callback hiện parse/copy trước khi commit và chỉ thay dữ liệu sau khi nạp thành công.

## 3. Gợi ý

- Animation event metadata hiện được lưu, chỉnh và hiển thị trên timeline. Có thể nối `sound`/`particle` với hệ thống phát audio/particle theo thời gian animation ở bước tiếp theo.
- Có thể trích DTO conversion từ `game/src/main.cpp` sang adapter riêng nếu editor được dùng ở nhiều executable.

## Kết luận

Đạt kế hoạch: UI editor ở engine dùng DTO/callback, còn parsing, validation, serialize và hot-reload nằm trong game. Reload tạo registry tạm trước khi thay, giữ vị trí/HP và cooldown theo skill ID. Build, 17 CTest và smoke test đều PASS.
