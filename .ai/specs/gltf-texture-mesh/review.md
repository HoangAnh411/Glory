# Code Review: Textured glTF Mesh

### 1. Phải sửa

- Không còn mục bắt buộc sửa. Cấu trúc engine/game được giữ tách biệt; các lời gọi OpenGL nằm trong engine. CPU loaders có thể được kiểm thử không cần GL context. Các tài nguyên stb/cgltf và GPU đều có đường giải phóng an toàn, và build/CTest/smoke test đều PASS.

### 2. Nên sửa

- `GltfAsset` hiện chọn mesh primitive đầu tiên. Khi scene cần nhiều primitive/material, nên mở rộng thành danh sách mesh thay vì âm thầm bỏ qua phần còn lại.
- Bổ sung test GLB có ảnh nhúng để kiểm thử nhánh buffer-view, ngoài glTF external buffer/texture của asset mẫu hiện tại.
- Có thể chuyển một số kiểm tra cấu trúc GLTF từ exception thành kiểu kết quả lỗi có ngữ cảnh chi tiết hơn.

### 3. Gợi ý

- Thêm cache texture/model và lựa chọn sampler/filter/wrap theo metadata glTF khi hệ thống asset phát triển.
- Phát triển shader thành Blinn-Phong ở roadmap kế tiếp; hiện normal được nạp và đưa vào layout nhưng shader mẫu chỉ hiển thị base-color texture.

## Kết luận

Đạt tiêu chí của nhóm hai mục hiện tại. Không có lỗi cản trở; có thể tick cả hai mục trong `PLAN.md`.
