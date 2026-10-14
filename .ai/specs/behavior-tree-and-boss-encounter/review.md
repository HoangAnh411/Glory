# Review: Behavior Tree và Boss Encounter

## 1. Phải sửa
- Không phát hiện lỗi bắt buộc phải sửa sau khi đối chiếu plan, kiểm tra luồng combat và chạy test.
- Behavior Tree và Boss simulation chỉ phụ thuộc thư viện toán/game; SDL/OpenGL chỉ được dùng ở lớp application/render để nhận telemetry và vẽ hiệu ứng.
- Boss dừng AI khi chết; phase transition, cooldown, I-frame, damage và knockback có kiểm thử CPU.

## 2. Nên sửa
- Boss hitbox hiện giải quyết theo capsule/sphere và ngưỡng khoảng cách cố định; có thể chuyển các trị số cân bằng sang dữ liệu để tinh chỉnh không cần build lại.
- Bộ test kiểm tra mô phỏng từng node/pha, chưa chạy một trận chiến end-to-end qua đầu vào SDL.

## 3. Gợi ý
- Mô hình hiện tại dùng `static_crate.gltf` làm hình đại diện Boss; thay bằng model giáp riêng và animation cho Slash, Charge, Slam, Enrage sẽ tăng độ rõ ràng và cảm giác trận đấu.
- Bổ sung telegraph/khung wind-up cho các đòn Slam và Charge để người chơi đọc đòn và căn Dodge tốt hơn.

## Kết luận
Đạt kế hoạch và các tiêu chí nghiệm thu. Build, 16/16 CTest và smoke test 120 tick PASS.
