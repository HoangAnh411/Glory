# Quyết Định Kỹ Thuật

## Stack đã chốt

| Lĩnh vực | Lựa chọn | Lý do / ranh giới |
|---|---|---|
| Ngôn ngữ | C++20 | Chuẩn thống nhất cho engine, client và server.
| Build | CMake 3.25+ | Cấu hình đa nền tảng; mỗi module được đưa vào khi có mã nguồn.
| Cửa sổ và input | SDL3 | Lớp platform của client; không phụ thuộc game rules.
| Đồ họa | OpenGL 4.5 Core | Renderer tự viết; chỉ client khởi tạo graphics context.
| Toán | GLM | Vector/matrix phía engine và renderer; game rules chỉ dùng phần cần thiết.
| Debug UI | Dear ImGui | Công cụ debug/editor, không phải UI gameplay cuối cùng.

## Thư viện theo giai đoạn

- cgltf: nạp glTF ở pipeline asset khi renderer/model cần.
- stb_image: nạp ảnh/texture khi triển khai asset pipeline.
- EnTT: ECS khi có entity/system gameplay; giữ game state không phụ thuộc renderer.
- miniaudio: âm thanh ở giai đoạn animation/gameplay.
- ENet: vận chuyển UDP đáng tin cậy cho đấu trường ở giai đoạn netcode.
- SQLite: lưu dữ liệu local trong tầng chơi đơn nếu cần; PostgreSQL là lựa chọn cho dịch vụ tài khoản/MMO. Chỉ chọn một backend khi thiết kế persistence cụ thể được duyệt.

Các dependency trên chưa được kéo vào bootstrap Giai đoạn 0. Không sử dụng Unity, Unreal, Godot hay engine có sẵn.

## Ranh giới module

- `engine/`: platform, renderer, asset, âm thanh và các dịch vụ nền. Renderer chỉ nhận dữ liệu cần vẽ.
- `game/`: luật chơi, combat, AI và cấu hình nội dung; không gọi API đồ họa.
- `server/`: host simulation authoritative, tái sử dụng luật chơi mà không tạo cửa sổ hay graphics context.
- `assets/`: dữ liệu nguồn và nội dung game, không chứa mã thực thi.
- `docs/`: thiết kế sản phẩm và kỹ thuật.

