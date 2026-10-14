# Review: GLM and Free-Fly Camera

## 1. Phải sửa

- Không có. `FreeCamera` chỉ phụ thuộc GLM và input frame chung; camera tests chạy mà không tạo SDL window hoặc OpenGL context.
- Cấu hình camera được kiểm tra (FOV, aspect, near/far, speed, sensitivity); pitch được clamp, yaw được giới hạn chu kỳ, chuyển động dùng fixed delta và chuẩn hóa hướng chéo.
- Các hàm GL cần cho uniform và depth test được nạp qua `SDL_GL_GetProcAddress`; shader upload ma trận GLM theo cột với `GL_FALSE`; renderer bật depth test. Không thấy lời gọi hàm OpenGL trực tiếp trong tầng game hoặc trong camera.
- Build, ba CTest, smoke 120 tick và kiểm tra whitespace đều đạt. Input mapping/fixed timestep hiện có và CMake runtime DLL copy vẫn còn nguyên.

## 2. Nên sửa

- Không có lỗi hoặc thiếu sót cần chặn nghiệm thu bước này.

## 3. Gợi ý

- Khi muốn trải nghiệm FPS liên tục qua biên cửa sổ, có thể bổ sung chế độ relative mouse/cursor capture và cơ chế bật/tắt để tránh giữ chuột khi không điều khiển camera.
- Có thể mở rộng unit test để kiểm tra hướng camera sau nhiều vòng yaw và các tổ hợp đồng thời Space+Control/W+S.

## Kết luận

Đạt tiêu chí của plan và ràng buộc kiến trúc. Không có mục “Phải sửa”; đề nghị đánh dấu hoàn thành trong `PLAN.md`.
