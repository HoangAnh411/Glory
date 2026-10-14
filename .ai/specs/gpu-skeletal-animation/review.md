# Code Review: GPU Skeletal Animation

### 1. Phải sửa

- Không còn vấn đề bắt buộc sửa. Loader trích xuất node hierarchy, skin joint mapping, inverse-bind matrices, JOINTS/WEIGHTS và TRS channels; dữ liệu JOINTS được ánh xạ theo thứ tự skin, weights được validate/normalize.
- Animator thuần C++/GLM xử lý LINEAR/STEP, slerp quaternion và tính global pose/palette; unit test không cần GL context.
- Renderer gửi tối đa 64 ma trận lên GPU; shader deform vị trí/pháp tuyến, còn `u_HasSkin=false` giữ đường mesh tĩnh. DebugPanel có chọn clip (khi nhiều clip), Play/Pause và scrub.
- Giữ nguyên fixed timestep, camera, Blinn-Phong, input mapping, DLL copy và smoke. Sáu CTest cùng smoke 120 tick PASS.

### 2. Nên sửa

- Parser hiện xử lý primitive đầu tiên của mesh và một skin gắn với mesh node đầu tiên; các model nhiều primitive/skin cần lặp và hợp nhất draw submissions.
- `CUBICSPLINE` và morph-weight animation bị từ chối rõ ràng; nên bổ sung khi asset pipeline cần các định dạng này.
- Skin palette/vector pose được cấp phát và tính lại mỗi render frame; có thể cache/cập nhật khi thời gian animation đổi để giảm allocation.
- Có thể bổ sung kiểm thử GL context/mock để khẳng định giới hạn uniform GPU trên nhiều driver; smoke hiện xác nhận trên máy build này.

### 3. Gợi ý

- Thêm animation blending, cross-fade và lựa chọn loop/clamp mode.
- Thêm test fixture nhiều clip để kiểm chứng Combo chọn clip trong DebugPanel.
- Dùng texture nhân vật/mesh khớp với asset gameplay khi có model nhân vật thật; fixture hiện tại là crate có joint chuyển động để kiểm chứng pipeline.

## Kết luận

Đạt mục “Skeletal animation: skinning trên GPU”; không có lỗi chặn. Có thể cập nhật checkbox roadmap.
