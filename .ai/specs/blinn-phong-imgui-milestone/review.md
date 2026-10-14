# Code Review: Blinn-Phong and Dear ImGui Milestone

### 1. Phải sửa

- Không còn lỗi bắt buộc sửa. Dear ImGui được đóng gói trong engine, header ImGui không lộ sang game; renderer nhận `DirectionalLight`; shader dùng world-space position/normal cùng view position; camera input được lọc theo capture flags, còn fixed update của game vẫn chạy bình thường.
- Vòng đời tài nguyên hợp lệ: renderer/model hủy trước DebugPanel; backend/context ImGui hủy khi GL context còn sống; sau đó application mới hủy context SDL. Constructor DebugPanel cũng dọn các phần đã khởi tạo nếu init bước sau thất bại.
- Build, 5 CTest và smoke test 120 tick đều PASS.

### 2. Nên sửa

- `FetchContent_Populate(imgui)` phát cảnh báo deprecated ở CMake 4.4. Nên đổi sang cơ chế populate không deprecated khi cấu trúc upstream ImGui hỗ trợ phù hợp, hoặc cô lập policy tương ứng.
- FPS hiện là nghịch đảo thời gian frame tức thời; có thể bổ sung trung bình trượt để số hiển thị ổn định hơn.
- Bổ sung test tích hợp/snapshot hoặc công cụ kiểm tra shader và điều khiển capture UI; CTest hiện kiểm thử math light, còn smoke chỉ bao phủ init/render/teardown.

### 3. Gợi ý

- Thêm orbit camera quanh target khi gameplay có nhân vật/scene entity, trong khi giữ free-fly camera cho debug.
- Mở rộng light panel với preset và nút reset thông số mặc định.
- Tách shader thành file asset để dễ reload/chẩn đoán khi nội dung render tăng.

## Kết luận

Đạt yêu cầu kỹ thuật của hai mục và kiểm chứng milestone hiện tại. Chỉ còn các cải tiến không chặn; có thể tick hai mục và cột mốc Giai đoạn 1 theo yêu cầu.
