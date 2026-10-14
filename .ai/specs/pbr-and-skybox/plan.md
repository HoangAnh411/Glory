# PBR cơ bản và skybox

## 1. Mục tiêu

Thay chiếu sáng Blinn-Phong của renderer bằng Cook-Torrance metallic-roughness, giữ shadow mapping hiện có, thêm các yếu tố vật liệu PBR từ glTF và một skybox cubemap procedural có phản xạ môi trường cơ bản. Các công thức PBR có phiên bản CPU/GLM độc lập với OpenGL để kiểm thử.

## 2. Danh sách task

1. Thêm module `pbr_math` với Fresnel Schlick, GGX NDF, Schlick-GGX geometry và hàm F0; thêm test CPU kiểm tra dielectric, metallic và roughness.
2. Mở rộng dữ liệu vật liệu glTF để đọc base color, metallic factor, roughness factor và AO; mặc định theo glTF khi thiếu material.
3. Thay fragment shader của ModelRenderer bằng Cook-Torrance, giữ 3x3 PCF và áp dụng shadow lên ánh sáng trực tiếp; cung cấp cubemap cho phản xạ kim loại.
4. Thêm Skybox RAII: cubemap gradient procedural, cube VAO/VBO, shader loại bỏ translation và đặt depth far plane; vẽ với `GL_LEQUAL` rồi khôi phục `GL_LESS`.
5. Nạp `glDepthFunc` trong GlApi; bảo đảm thứ tự hủy resource còn nằm trong lifetime GL context.
6. Mở rộng DebugPanel với Metallic, Roughness, AO và Enable Skybox.
7. Đăng ký test PBR trong CTest; build, chạy toàn bộ CTest và `--smoke-test`; ghi báo cáo test và review.

## 3. File cần sửa / tạo mới

- Tạo `engine/include/glory/engine/pbr_math.hpp`, `engine/src/pbr_math.cpp`.
- Tạo `engine/include/glory/engine/skybox.hpp`, `engine/src/skybox.cpp`.
- Tạo `tests/pbr_tests.cpp`.
- Sửa `engine/include/glory/engine/gltf_asset.hpp`, `engine/src/gltf_asset.cpp` để giữ thông số metallic-roughness/AO.
- Sửa `engine/include/glory/engine/model.hpp`, `engine/src/model.cpp` để cung cấp thuộc tính vật liệu cho renderer.
- Sửa `engine/include/glory/engine/gl_api.hpp`, `engine/src/gl_api.cpp` để nạp `glDepthFunc`.
- Sửa `engine/include/glory/engine/shader_program.hpp`, `engine/src/shader_program.cpp` để truyền uniform PBR và cubemap.
- Sửa `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp` cho shader PBR, skybox và tham số điều khiển.
- Sửa `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp` cho sliders/checkbox.
- Sửa `CMakeLists.txt` để thêm nguồn và test.
- Sau PASS, cập nhật mục trong `PLAN.md`.
- Tạo `.ai/specs/pbr-and-skybox/test-report.md`, `.ai/specs/pbr-and-skybox/review.md`.

## 4. Rủi ro và giảm thiểu

- Cubemap procedural cần tạo dữ liệu sáu mặt nhất quán; dùng cùng hàm gradient theo hướng chuẩn hóa cho từng texel để tránh đường nối.
- Mở rộng struct asset có thể ảnh hưởng loader; giữ defaults glTF metallic=1 và roughness=1 khi không có material, AO=1.
- Thứ tự hủy tài nguyên GL quan trọng; đặt Skybox trong ModelRenderer và bảo đảm renderer được hủy trước khi application hủy context.
- `glDepthFunc` cần được khôi phục cả sau lượt vẽ skybox; thực hiện theo cặp LEQUAL/LESS trong hàm draw.
- Test PBR phải không tạo context hoặc gọi OpenGL; link chỉ GLM và module toán.

## 5. Tiêu chí chấp nhận

- PBR math chạy CPU độc lập: F0 dielectric bằng 0.04, metallic F0 bằng albedo, GGX hữu hạn và đáp ứng roughness thấp/cao.
- glTF metallic/roughness factors được giữ lại và dùng trong shading; shader áp dụng ambient AO, GGX/Smith/Schlick và shadow chỉ trên direct light.
- Skybox procedural hiển thị phía sau scene, không dịch chuyển theo camera; cubemap tham gia phản xạ metallic.
- Depth function được khôi phục về `GL_LESS`; resource skybox được giải phóng trước GL context.
- DebugPanel có Metallic [0,1], Roughness [0.04,1], AO [0,1], Enable Skybox.
- CTest có 8 test và toàn bộ PASS; build PASS; `glory_client.exe --smoke-test` thoát mã 0 sau 120 tick.
- Reviewer ghi nhận ba mục Phải sửa / Nên sửa / Gợi ý; chỉ tick roadmap sau khi không còn vấn đề bắt buộc.
