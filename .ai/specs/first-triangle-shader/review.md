# Review: First Shader Triangle

## 1. Phải sửa

- Không có. Toàn bộ GL entry points dùng trong render được nạp từ `SDL_GL_GetProcAddress`; shader/program và VAO/VBO chỉ nằm trong `glory_engine`. Không tìm thấy lời gọi OpenGL trực tiếp trong `game/` hoặc lời gọi hàm `gl*()` trực tiếp ở source engine.
- `TriangleRenderer` được tạo sau khi context hiện hành và nằm trong scope kết thúc trước `SDL_GL_DestroyContext`. Cả đường thoát bình thường lẫn ngoại lệ khởi tạo renderer đều giải phóng renderer trước context; shader compile/link failure cũng dọn đối tượng đã tạo.
- GLSL dùng `#version 450 core`, buffer có ba vertex interleaved position/color và phát lệnh `glDrawArrays(GL_TRIANGLES, 0, 3)`. Input, fixed timestep, CTest registrations và cấu hình copy DLL hiện có được giữ nguyên.

## 2. Nên sửa

- Không có mục chặn nghiệm thu. Smoke test cùng kiểm tra tĩnh xác nhận đường render và cleanup theo code; không có regression trong test suite hiện tại.

## 3. Gợi ý

- Bổ sung kiểm thử lỗi biên dịch/link shader bằng context test riêng hoặc injectable GL function table để xác minh driver log và cleanup trên các nhánh lỗi.
- Khi có quy trình visual regression, chụp framebuffer trong smoke test để xác nhận màu/vị trí tam giác, thay vì chỉ xác nhận pipeline chạy hết vòng lặp.

## Kết luận

Đạt các tiêu chí kỹ thuật của bước. Không còn lỗi bắt buộc phải sửa; đề nghị tick mục “Vẽ tam giác đầu tiên bằng shader” trong `PLAN.md`.
