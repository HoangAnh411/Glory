# Review: Dodge và Capsule Collision

## 1. Phải sửa

- Không phát hiện lỗi bắt buộc phải sửa. State Dodge/I-frame và capsule resolver nằm trong `glory_game`; không phụ thuộc SDL/OpenGL. Cả 12 CTest cùng smoke-test PASS.

## 2. Nên sửa

- Resolver hiện duyệt tuần tự toàn bộ AABB và ramp mỗi substep. Khi số collider tăng, nên thêm broadphase spatial partition để giảm chi phí.
- Collision count là số lần tiếp xúc được xử lý trong các substep, có thể lớn hơn số vật cản duy nhất trong một tick; nên đổi tên/hiển thị thành “resolution contacts” hoặc bổ sung bộ đếm collider duy nhất nếu UI cần ý nghĩa đó.

## 3. Gợi ý

- Khi gameplay có sát thương thực, nối `isInvulnerable` với combat damage gate; hiện controller phát trạng thái I-frame và DebugPanel hiển thị, chưa có hệ thống nhận sát thương ở roadmap.
- Bổ sung terrain mesh/heightfield sau này; geometry hiện tại cố ý dùng mặt phẳng, AABB và ramp tuyến tính để giữ resolver độc lập, dễ test CPU.
