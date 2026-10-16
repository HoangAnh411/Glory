# Kế Hoạch Kỹ Thuật: Đại Tu Đồ Họa, Nhân Vật, Đấu Sĩ Boss & Không Gian Đấu Trường (Visual Atmosphere, Characters & World Overhaul)

## 1. Mục Tiêu (Objective)
Giải quyết triệt để phản hồi của người chơi về mặt hình ảnh ("trông đang chán lắm bạn ạ"):
1. **Nhân vật Chiến Binh (Hero Warrior):**
   - Loại bỏ hoàn toàn lớp texture hộp gỗ màu nâu bùn đất (`starter_crate.png`).
   - Tạo bộ texture bảng màu chi tiết phong cách hiệp sĩ kỳ ảo (`warrior_palette.png`): giáp thép sáng bóng bạc, áo choàng/tabard xanh thẫm hoàng gia (Royal Blue) sau lưng, viền vàng kim loại, kính ngắm visor phát sáng, lưỡi kiếm sáng ánh thép.
   - Bổ sung ánh sáng viền (Rim Light / Fill Light) để lưng nhân vật không bị tối sầm khi quay lưng về camera.
2. **Đấu Sĩ Boss Hoành Tráng ("Gorgon, the Iron Warden"):**
   - Thay thế hoàn toàn khối gạch hộp nâu đơn điệu bằng mô hình Boss 3D đích thực (`boss_gorgon.gltf`): Đấu sĩ khổng lồ cao 3.5m khoác giáp đen Dark Iron, vai giáp gai góc, nón sắt có sừng quỷ với đôi mắt rực lửa đỏ, tay vung cây Chiến Búa Khổng Lồ (Great Warhammer / Cleaver).
3. **Đấu Trường La Mã Kỳ Ảo (Colosseum Arena Environment):**
   - Thay mặt sàn xám đơn điệu bằng sàn đá lát vân đấu trường La Mã có hoa văn trung tâm.
   - Dựng các trụ đá đấu trường (Stone Pillars) và đuốc lửa cháy rực (Flaming Braziers) bao quanh ranh giới đấu trường, tạo chiều sâu không gian 3D sống động.
   - Bầu trời hoàng hôn chiến trận (Dramatic Sky & Sun) tương phản rõ nét với bóng đổ nhân vật.
4. **Chuẩn Hóa Giao Diện (UI Polish & Zero Overlap):**
   - Sửa lỗi dòng phím tắt `[WASD] Di chuyen...` đè lên thanh phím kỹ năng `Skill Action Bar`.
   - Ẩn hộp thoại `Glory Account Login` thừa thãi khi đang ở chế độ chơi Offline Sandbox.
   - Phối màu từng ô kỹ năng trên HUD (Đỏ bạo kích, Vàng chém nhanh, Xanh bão kiếm) giúp thanh Action Bar bắt mắt.

---

## 2. Danh Sách Task Nhỏ (Tasks)

- [ ] **Task 1: Tạo Bộ Texture & Nâng Cấp Model Chiến Binh (Hero Warrior Polish)**
  - Tạo texture độ phân giải cao `assets/textures/warrior_palette.png` với các vùng màu chuyên biệt: Giáp thép bạc bóng, vải hoàng gia xanh thẫm, viền vàng, lưỡi kiếm sắc bén, da tay và bốt chiến.
  - Cập nhật script `tools/generate_hero_warrior.py` để map UV chính xác từng bộ phận (đầu, ngực, lưng, kiếm, vai, chân) vào texture mới.
  - Tăng độ sáng môi trường `ambientStrength` từ 0.35 lên 0.55 và chỉnh góc nguồn sáng mặt trời để lưng nhân vật luôn nổi bật khối 3D.

- [ ] **Task 2: Tạo Mô Hình 3D Đấu Sĩ Boss Gorgon (The Iron Warden Boss)**
  - Viết generator script `tools/generate_boss_model.py` tạo `assets/models/boss_gorgon.gltf` và `.bin`:
    - Thân hình hộ pháp khổng lồ, giáp sắt đen gai góc, tay cầm Chiến Búa Khổng Lồ, mắt phát sáng.
    - Bộ xương và animation chiến đấu: `Idle` (thở uy hiếp), `Attack` (bổ búa sấm sét).
  - Cập nhật `engine/src/application.cpp` để nạp `boss_gorgon.gltf` cho `bossRenderer` thay vì nạp chung crate.

- [ ] **Task 3: Sàn Đấu Đá Cổ Phong & Kiến Trúc Đấu Trường (Arena Pillars & Architecture)**
  - Nâng cấp `GroundPlane` và shader trong `engine/src/ground_plane.cpp` và `engine/src/model_renderer.cpp`:
    - Vẽ sàn đá đấu trường có texture vân đá lát và vòng tròn hoa văn cổ phong.
    - Dựng 8 trụ đá đấu trường bao quanh ranh giới vòng cung 30m, mỗi trụ có đài lửa bốc cháy.
  - Nâng cấp bầu trời `skyColor` trong `engine/src/skybox.cpp` với dải mây hoàng hôn hùng vĩ.

- [ ] **Task 4: Tinh Chỉnh Bố Cục UI (Fix Overlapping & Clean Layout)**
  - Trong `engine/src/debug_panel.cpp`:
    - Tách biệt vị trí dòng hướng dẫn nhanh `Quick Hint` lên phía trên Skill Bar hoặc sang góc phải gọn gàng, loại bỏ hoàn toàn hiện tượng chữ đè lên nút skill.
    - Chỉ hiển thị modal đăng nhập khi người chơi chủ động bấm đăng nhập hoặc khi kết nối online; ẩn gọn gàng khi chơi Sandbox.
    - Đổi màu slot icon kỹ năng rực rỡ tương ứng với từng chiêu thức.

- [ ] **Task 5: Kiểm Thử Tự Động & Thẩm Định**
  - Chạy `cmake --build build`, `ctest` và smoke test đảm bảo 100% tests PASS sạch sẽ.

---

## 3. Danh Sách File Cần Chỉnh Sửa / Tạo Mới (Files to touch)
- `tools/generate_hero_warrior.py`
- `tools/generate_boss_model.py`
- `assets/textures/warrior_palette.png`
- `assets/textures/boss_palette.png`
- `assets/models/hero_warrior.gltf`
- `assets/models/boss_gorgon.gltf`
- `engine/include/glory/engine/directional_light.hpp`
- `engine/src/application.cpp`
- `engine/src/model_renderer.cpp`
- `engine/src/ground_plane.cpp`
- `engine/src/skybox.cpp`
- `engine/src/debug_panel.cpp`
- `game/src/main.cpp`
- `tests/test_game_hud.cpp`

---

## 4. Tiêu Chí Chấp Nhận (Acceptance Criteria)
1. **Chiến binh đẹp mắt:** Nhân vật có màu sắc giáp thép bạc sáng rõ, áo vải xanh thẫm, cầm đại kiếm thép bóng, không còn là khối màu nâu đen bùn đất.
2. **Boss Gorgon khổng lồ:** Boss đứng trên sân đấu là đấu sĩ thiết giáp đen to lớn cầm Chiến Búa, không còn là khối hộp gạch tĩnh.
3. **Không gian đấu trường có kiến trúc:** Nhìn thấy sàn đá đấu trường, các trụ đá cổ và ngọn lửa bao quanh, bầu trời tương phản đẹp mắt.
4. **UI không bị đè chữ:** Dòng hướng dẫn phím bấm nằm tách bạch, không đè lên thanh phím kỹ năng 1..4.
5. **100% Tests PASS.**
