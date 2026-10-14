# Review: Hitbox, Hurtbox và Skills JSON

## 1. Phải sửa

- Không phát hiện vấn đề bắt buộc phải sửa. JSON loader, skill registry, geometry intersection, cooldown/cast state và damage resolution nằm trong `glory_game`; combat không gọi SDL/OpenGL. 13/13 CTest và smoke-test 120 tick PASS.

## 2. Nên sửa

- Combat hiện có một practice dummy và một hurtbox trong sample. Khi thêm nhiều target, chuyển danh sách entity sang collection và duyệt theo broadphase để mở rộng mà không đổi API hitbox.
- Hit event dùng tâm hitbox làm vị trí hiệu ứng; có thể tính điểm gần nhất trên đoạn capsule để particle xuất hiện chính xác hơn trên bề mặt.

## 3. Gợi ý

- Thêm animation socket/bone metadata khi asset nhân vật thật được tích hợp. Hiện tại hitbox theo vị trí nhân vật, `facingYaw` và active window của animation state theo đúng dữ liệu skill.
- Bổ sung kiểm thử JSON lỗi/duplicate ID và tương tác UI thủ công; smoke-test không giả lập thao tác DebugPanel.
