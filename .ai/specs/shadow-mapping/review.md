# Code Review: Shadow Mapping

### 1. Phải sửa (Critical / Required)

- Không còn lỗi bắt buộc sửa. CPU light-space math nằm riêng khỏi OpenGL; test target chỉ biên dịch GLM math. FBO kiểm tra completeness, depth/main shader khởi tạo thành công trong smoke, pass depth dùng skin palette hiện tại, và pass chính phục hồi framebuffer/viewport trước khi vẽ floor/model.
- Thứ tự thành viên bảo đảm `GroundPlane`, `Model`, shader và `ShadowMap` được hủy trước `GlApi`; renderer sống trong scope có GL context nên các tài nguyên GPU được giải phóng trước khi context bị hủy.
- Bảy CTest cùng smoke 120 tick đã PASS sau lần build cuối.

### 2. Nên sửa (Recommended)

- Light-space camera hiện dùng center `(0,0,0)` và ortho half-extent cố định `16`. Cách này phù hợp sample hiện tại; khi camera/scene có thể di chuyển, nên tính bounds từ frustum hoặc vùng caster/receiver để tránh bóng bị cắt và tăng độ phân giải hữu dụng.
- Smoke xác nhận FBO completeness và shader compile/link nhưng không đo chất lượng ảnh. Nên thêm kiểm tra hình ảnh hoặc lượt QA trực quan khi có harness render capture.

### 3. Gợi ý (Suggestions)

- Có thể thêm texel snapping vào light-space center để giảm bóng rung khi camera di chuyển.
- Khi scene có nhiều mesh, gom caster thành danh sách renderable dùng chung giữa depth pass và lighting pass để tránh bỏ sót vật thể.

## Kết luận

Đạt mục Shadow mapping cho sample hiện tại; không có vấn đề chặn. Có thể tick mục tương ứng trong roadmap.
