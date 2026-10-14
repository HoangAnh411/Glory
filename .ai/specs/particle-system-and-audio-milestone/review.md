# Review: Particle System và Audio milestone

## 1. Phải sửa

- Không có. Particle physics và CTest chạy độc lập với GL; smoke khởi tạo audio engine, chạy 120 tick và thoát 0. Particle renderer khôi phục depth mask và tắt blending sau khi vẽ. ModelRenderer và AudioSystem được hủy trước GL context/SDL shutdown.

## 2. Nên sửa

- `ParticleRenderer::draw` tạo lại vector vertex mỗi frame đang có particle. Có thể giữ vector scratch trong renderer để giảm allocation khi nhiều burst liên tục.
- AudioSystem bỏ qua mã lỗi trả về từ `ma_engine_play_sound`; nên bổ sung log hoặc bộ đếm lỗi asset khi tích hợp pipeline âm thanh lớn hơn.
- Điểm spawn `hit_frame` hiện dùng vị trí tấn công cố định trong world; character controller/hitbox ở Giai đoạn 3 có thể cung cấp vị trí theo socket và mục tiêu va chạm.

## 3. Gợi ý

- Thêm pooling/instance rendering và giới hạn theo ngân sách GPU khi hiệu ứng có hàng nghìn hạt.
- Đưa tên file âm thanh, màu, lực và số lượng hạt vào cấu hình skill data-driven thay vì ánh xạ trực tiếp trong callback.
- Thêm kiểm thử AudioSystem bằng backend null hoặc fake để kiểm tra mute/master-volume mà không cần thiết bị vật lý.
