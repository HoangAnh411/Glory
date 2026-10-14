# Review: Character Controller và Third-Person Camera

## 1. Phải sửa

- Không phát hiện vấn đề bắt buộc phải sửa. `CharacterController` ở `glory_game`, không phụ thuộc SDL, renderer hay OpenGL; kết quả build và toàn bộ test đều PASS.

## 2. Nên sửa

- Tính toán shadow light-space hiện vẫn đặt quanh world origin. Khi nhân vật đi xa hơn vùng shadow hiện có, bóng của nhân vật có thể bị cắt; nên chuyển tâm shadow theo vùng camera/nhân vật ở bước cải tiến shadow tiếp theo.
- Camera chưa có xử lý va chạm với địa hình/vật cản. Đây chưa thuộc bước hiện tại vì kế hoạch va chạm capsule với địa hình nằm ở mục kế tiếp của roadmap.

## 3. Gợi ý

- Thêm test tích hợp input/camera-mode với application nếu sau này tách vòng lặp ứng dụng khỏi OpenGL context để test được headless.
- Cân nhắc thêm gia tốc/giảm tốc nhân vật và smoothing camera sau khi có yêu cầu điều khiển cụ thể; hiện tốc độ ngang đổi tức thời theo input như phạm vi controller cơ bản.
- Cập nhật `PLAN.md` chỉ sau khi đã lưu báo cáo kiểm thử và review; trạng thái cuối cùng của mục này đã được cập nhật sau khi hai vai trò hoàn tất.
