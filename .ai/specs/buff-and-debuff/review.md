# Review: Buff and Debuff

## 1. Phải sửa
- Không phát hiện lỗi nghiêm trọng qua đối chiếu kế hoạch, kiểm tra mã và kết quả build/test.
- Status manager và skill parser nằm trong `glory_game`, không phụ thuộc SDL/OpenGL.

## 2. Nên sửa
- `StatusEffectManager` giữ stack limit theo lần áp dụng đầu tiên; nếu cần thay đổi giới hạn trong lúc hiệu ứng đang tồn tại, nên quy định rõ chính sách cập nhật dữ liệu.
- Máu người chơi hiện là giá trị runtime phục vụ hiệu ứng HoT/DoT, chưa có giao diện HP riêng vì phạm vi milestone tập trung vào buff/debuff.

## 3. Gợi ý
- Mở rộng các preset DebugPanel thành bảng chỉnh sửa magnitude/duration/tick interval để tuning trực tiếp.
- Khi thêm multiplayer, serialize trạng thái hiệu ứng và tick timer trong snapshot authoritative.

## Kết luận
Đạt kế hoạch: hiệu ứng định kỳ, modifier, stun, stacking/expiration, on-hit JSON, giao diện debug và unit tests đều được tích hợp. Build, 14/14 CTest và smoke test 120 tick PASS.
