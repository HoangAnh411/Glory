# Review: Combo, Cancel Window, Hit-stop và Knockback

## 1. Phải sửa

- Không phát hiện lỗi bắt buộc. Combo/cancel/hit-stop/knockback nằm trong `glory_game`; controller và animation freeze nhận trạng thái qua callback data; `glory_game` không gọi SDL/OpenGL. 13/13 CTest và smoke-test PASS.

## 2. Nên sửa

- `CombatSystem` hiện quản lý một practice dummy. Khi combat chuyển sang nhiều entity, chuyển target/hit set sang danh sách entity và tiếp tục dùng collision broadphase.
- Input buffering và hit-stop đang được cập nhật theo fixed tick; các giá trị sát biên nên tiếp tục được kiểm thử khi thay fixed timestep hoặc thêm animation rate scaling.

## 3. Gợi ý

- Tách cấu hình combat tuning sang editor/hot reload khi roadmap đến công cụ nội bộ; hiện các khoảng combo/cancel/hit-stop/knockback đã data-driven từ skills JSON.
- Thêm animation state riêng cho từng combo step và Dodge khi asset animation tương ứng sẵn sàng; hiện ba nhịp restart state `Attack` qua animation request pulse.
- Bổ sung bài test integration cho `CharacterController::allowDodge` cùng input Left Shift và test va chạm knockback ở nhiều góc/tường liên tiếp.
