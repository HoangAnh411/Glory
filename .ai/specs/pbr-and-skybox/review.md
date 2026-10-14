# Review: PBR cơ bản và skybox

## 1. Phải sửa

- Không có. Build, 8 bài CTest và smoke test đều PASS. Thứ tự thành viên bảo đảm Skybox, model, shader và shadow resources được giải phóng khi OpenGL context còn tồn tại; Skybox khôi phục `GL_LESS` sau khi vẽ.

## 2. Nên sửa

- glTF material metallic/roughness/AO đang lấy các factor và occlusion strength; chưa nạp các texture metallic-roughness/occlusion chuẩn glTF. Thêm texture maps sẽ giúp asset PBR thực tế có độ chi tiết theo texel.
- Bộ kiểm thử CPU kiểm tra F0 và GGX trực tiếp, nhưng có thể bổ sung kiểm thử riêng cho Fresnel Schlick và biên roughness 0.04/1.0.
- Skybox gradient procedural tạo ảnh riêng cho sáu mặt. Có thể tăng tính liên tục ở seam bằng cách đánh giá toàn bộ texel từ hướng cubemap chuẩn và kiểm tra hình ảnh trên nhiều GPU.

## 3. Gợi ý

- Thêm mip chain/prefiltered environment cubemap và BRDF integration LUT để phản xạ IBL metallic thay đổi theo roughness chính xác hơn.
- Cho phép chọn cubemap asset ngoài procedural gradient trong công cụ debug.
