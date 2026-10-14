# Review: Input Capture and Remapping

## 1. Phải sửa

- Không có. Code đáp ứng plan; public shared/game input data không phụ thuộc SDL/OpenGL, rebinding từ chối action/control không hợp lệ mà không thay đổi mapping trước đó, và test/build/smoke đều PASS.

## 2. Nên sửa

- Không có mục bắt buộc hoặc cần sửa trước khi đánh dấu hoàn thành.

## 3. Gợi ý

- `Control` hiện hỗ trợ một tập điều khiển được liệt kê rõ (W/A/S/D, Q/E, Space, Escape, phím mũi tên và ba nút chuột). Khi xây dựng màn hình cấu hình phím, có thể mở rộng danh sách hoặc biểu diễn phím vật lý tổng quát để người chơi gán các phím khác.
- Bộ test bao phủ mapping/rebinding thuần game; có thể bổ sung test SDL event translator cho key repeat, focus loss và mouse delta khi cần tăng coverage tầng engine.

## Kết luận

Đạt yêu cầu của bước hiện tại. Không phát hiện lỗi cần sửa; đề nghị tick mục input/remapping trong `PLAN.md`.
