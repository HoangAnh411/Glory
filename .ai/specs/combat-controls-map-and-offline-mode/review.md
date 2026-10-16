# Báo Cáo Đánh Giá (Review Report): Combat Controls, Arena Map & Play Modes

- **Thời gian đánh giá:** 2026-10-14 20:47
- **Người thực hiện (Vai trò):** REVIEWER
- **Trạng thái:** **APPROVED (ĐÃ DUYỆT)**

---

## 1. Phải sửa (Critical / Required)
*Không có lỗi nghiêm trọng.* Toàn bộ các tiêu chí đề ra trong `plan.md` đã được triển khai chính xác, an toàn bộ nhớ, cấu trúc luồng rõ ràng, không crash, build sạch và 37/37 unit tests đều PASS.

---

## 2. Nên sửa (Recommended)
1. **Kiểm tra tương tác chuột với thanh Skill trên HUD (`captureMouse_`):**
   - Khi người chơi click chuột trái vào ô kỹ năng trên HUD, hành động click đồng thời kích hoạt `requestedSkillId = skill.id` và gửi tín hiệu chuột trái cho input frame. Hiện tại logic trong `main.cpp` ưu tiên `effectiveSkillId` nên skill vẫn tung ra chuẩn xác. Tuy nhiên, về lâu dài nên tách biệt rõ giữa "Click UI" và "Click tấn công thường vào không gian 3D" để tránh xoay camera ngoài ý muốn khi click chuột vào thanh action bar.
2. **Khung cấu hình phím linh hoạt (Action-to-Slot Mapping):**
   - Hiện tại `Skill1..Skill4` ánh xạ trực tiếp theo chỉ mục `availableSkills[0..3]`. Trong tương lai khi có tính năng kéo thả kỹ năng vào thanh phím tắt (Hotbar customization), nên đưa thêm một bảng ánh xạ `Slot -> SkillId` độc lập.

---

## 3. Gợi ý (Suggestions)
1. **Giải thích trực quan cho người chơi về Model nhân vật (Khối hộp 3D `starter_crate.gltf`):**
   - Người chơi thắc mắc tại sao nhân vật là "cục hình vuông": Hiện tại asset mẫu trong thư mục `assets/models/` của engine chỉ có `starter_crate.gltf` (khối hộp lập phương có gắn xương rig để kiểm thử shader skinning, hitbox và animation transform trong các phase đầu).
   - *Đề xuất:* Khi triển khai bước nâng cấp đồ họa nhân vật tiếp theo, ta có thể import mô hình nhân vật Humanoid 3D (ví dụ: Chiến binh cầm kiếm định dạng GLTF/GLB có xương hoàn chỉnh) để thay thế khối hộp này.
2. **Hiệu ứng âm thanh khi tung chiêu (Skill SFX):**
   - Hiện tại engine đã có `AudioSystem`, có thể bổ sung các âm thanh chém gió, va chạm kim loại đặc thù khi bấm phím 1, 2, 3, 4 để tăng cảm giác chặt chém hành động chân thực.
3. **Menu `play_game.bat`:**
   - Việc tách rõ 2 tùy chọn: `[1] Chơi Offline Sandbox` và `[2] Chơi Online Multiplayer (tự động bật Server)` là giải pháp rất trực quan, giúp người chơi vào trải nghiệm gameplay ngay lập tức mà không phải mở nhiều cửa sổ terminal.

---

## 4. Bảng Đối Chiếu Chi Tiết Với Kế Hoạch (Plan vs Code)

| Hạng mục kiểm tra | Yêu cầu theo `plan.md` | Kết quả rà soát mã nguồn | Đánh giá |
|:---|:---|:---|:---:|
| **Phím bấm kỹ năng 1..4** | Thêm `Key1..Key4` vào InputFrame, ActionMap và xử lý trong `main.cpp` | Đã khai báo trong `input_frame.hpp`, map SDL scancode trong `input_system.cpp`, bind `Action::Skill1..4` trong `action_map.cpp`, gán `effectiveSkillId` và gọi `combat.requestSkill()` an toàn với kiểm tra biên độ dài vector. | **ĐẠT** |
| **Sửa phím Né đòn / Nhảy** | Đổi nhãn HUD thành `[Shift] Dodge`, `[Space] Jump` | Đã sửa trong `debug_panel.cpp` dòng 257 và dòng 277 (quick hint). | **ĐẠT** |
| **Click icon chiêu thức** | Click chuột vào slot trên Action Bar để tung chiêu | Đã thêm kiểm tra hover và left click trong `debug_panel.cpp` dòng 228 gán `requestedSkillId = skill.id`. | **ĐẠT** |
| **Mở rộng sàn đấu Arena** | Mở rộng sàn đá 60x60m tại `Y = 0.0F` | Sửa `floorY = 0.0F` trong `ground_plane.cpp`, mở rộng từ `[-60, 60]`, điều chỉnh shader base color và PBR roughness trong `model_renderer.cpp`. | **ĐẠT** |
| **Offline Sandbox Mode** | Tự động chạy local simulation khi không có kết nối server | Điều kiện `networkClient->state() == Connected && accountPanel.authenticated` giúp client không bị block vòng lặp game khi server offline. Thêm nút "Play Offline" trong modal Login. | **ĐẠT** |
| **Launcher `play_game.bat`** | Menu chọn Offline Sandbox hoặc Online tự bật Server | Menu 2 tùy chọn gọn gàng, mặc định chọn 1 vào thẳng game. | **ĐẠT** |
| **Kiểm thử tự động** | Viết test cho `Skill1..4` và pass toàn bộ test suite | Thêm `testSkillActionBindings()` vào `tests/test_game_hud.cpp`, 37/37 tests pass 100%. | **ĐẠT** |

---

## 5. Kết Luận Của Reviewer
Mã nguồn đáp ứng toàn diện các tiêu chí chất lượng và hoàn thành tốt yêu cầu của người dùng. Cho phép chuyển sang bước **TỔNG KẾT** để bàn giao cho người dùng trải nghiệm.
