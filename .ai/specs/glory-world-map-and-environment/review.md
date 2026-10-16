# Đánh Giá Mã Nguồn (Code Review): Glory World Map & Environment

- **Thời gian đánh giá:** 2026-10-15 15:40
- **Người thực hiện (Vai trò):** REVIEWER
- **Trạng thái:** **APPROVED (Chấp Thuận Triển Khai)**

---

## 1. Mục Tiêu Đợt Nâng Cấp (Review Focus)
Xây dựng một bản đồ thế giới mở rộng lớn, sống động theo tinh thần game MMORPG *Vinh Diệu* (*Toàn Chức Cao Thủ*): Có phân khu Trong Thành, Ngoài Thành, Phế Tích Nguy Hiểm; có đầy đủ nhà cửa, cổng thành, đài phun nước, cột đèn, cây cối, đá, rào chắn, đài đuốc lửa; xóa sạch bóng tối đen ngòm bằng sương mù đường chân trời; tích hợp va chạm vật lý chân thực.

---

## 2. Chi Tiết Rà Soát Mã Nguồn & Tệp Tin (File-by-File Review)

### 2.1. Assets & Kịch Bản Sinh Model
- [tools/generate_environment_models.py](file:///d:/Data/Glory/tools/generate_environment_models.py):
  - Sinh thành công 11 model glTF 2.0 dạng nhị phân chuẩn (`.gltf` + `.bin`).
  - Sinh bảng màu PBR `environment_palette.png` (256x256) chứa đầy đủ các bề mặt: gạch đá, gỗ vách, ngói nung, kính đèn phát sáng, lá sồi, lá thông, vỏ cây, đá tự nhiên, sắt rèn, lửa đuốc.
  - Sinh texture mặt đất `world_ground.png` (512x512) phân chia 3 vùng địa hình rõ rệt.
  - Đánh giá: **Xuất sắc**. Các model có UV tọa độ chính xác, không thừa thẹo hay méo mó.
- [tools/generate_world_map_data.py](file:///d:/Data/Glory/tools/generate_world_map_data.py) & [assets/data/world_map.json](file:///d:/Data/Glory/assets/data/world_map.json):
  - Định nghĩa 81 vật thể tĩnh dàn trải hợp lý: Khu thành phố có đường phố, nhà cửa hai bên, đài phun nước trung tâm, dãy cổng thành luỹ kiên cố; Khu dã ngoại có đường mòn, hàng rào gỗ, cột đèn và rừng cây sồi/thông; Khu phế tích có vòng cột đổ nát và đài đuốc lửa.

### 2.2. Engine & Shader
- [engine/src/ground_plane.cpp](file:///d:/Data/Glory/engine/src/ground_plane.cpp):
  - Mở rộng kích thước sàn đấu lên 240m x 240m (`halfExtent = 120.0F`), UV đảo ngược trục Y để khớp hướng Bắc - Nam của bản đồ.
- [engine/src/model_renderer.cpp](file:///d:/Data/Glory/engine/src/model_renderer.cpp) & [engine/include/glory/engine/model_renderer.hpp](file:///d:/Data/Glory/engine/include/glory/engine/model_renderer.hpp):
  - Tự động nạp `world_map.json` và lưu trữ mẫu model độc nhất trong `propModelTemplates_` (chỉ nạp 11 instance, tái sử dụng cho 81 vị trí ➔ Tối ưu bộ nhớ VRAM).
  - Vẽ vật thể tĩnh trong cả Render Pass chính và Shadow Depth Pass (các công trình, cây cối đều đổ bóng xuống mặt đất chân thực).
  - Bổ sung Distance Fog vào fragment shader: Vật thể xa > 48m hòa trộn mượt mà vào màu hoàng hôn `vec3(0.68, 0.62, 0.55)`, triệt tiêu hoàn toàn khoảng đen vô tận.

### 2.3. Gameplay & Physics
- [game/include/glory/game/character_controller.hpp](file:///d:/Data/Glory/game/include/glory/game/character_controller.hpp) & [game/src/main.cpp](file:///d:/Data/Glory/game/src/main.cpp):
  - Bổ sung phương thức `addObstacle(const Aabb&)` cho `CharacterController`.
  - Tự động duyệt qua 81 vật thể trong `world_map.json` và đăng ký các hộp va chạm AABB vào `CollisionWorld`. Nhân vật va chạm chính xác với tường nhà, tường thành, cột đá.
  - Tự động kích hoạt `bossEncounterActive = true` khi Boss nhận sát thương hoặc khi bước qua cổng phế tích.
  - Cập nhật thời gian thực tên phân khu (`zonePanel.currentZone`) hiển thị trên HUD.

---

## 3. Tuân Thủ Quy Chuẩn & Kiến Trúc
1. **Tuân thủ quy định PLAN.md:** Tách bạch logic game khỏi đồ họa; không dùng engine có sẵn; tên biến, hàm và comment viết bằng tiếng Anh chuẩn.
2. **Tuân thủ quy định .ai/workflow.md:** Đã hoàn tất cả 4 bước tuần tự.
3. **Kết quả kiểm thử:** Toàn bộ 37/37 bài unit test và Smoke test 120 ticks đều đạt 100%.

---

## 4. Kết Luận
Đợt nâng cấp đạt yêu cầu 100% theo đúng mong muốn của người dùng. Cho phép hoàn tất và cập nhật tiến độ.
