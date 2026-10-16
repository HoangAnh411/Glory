# Báo Cáo Đánh Giá (Review Report): Combat Immersion & Visual Overhaul

- **Thời gian đánh giá:** 2026-10-14 21:04
- **Người thực hiện (Vai trò):** REVIEWER
- **Trạng thái:** **APPROVED (ĐÃ DUYỆT)**

---

## 1. Phải sửa (Critical / Required)
*Không có lỗi nghiêm trọng.* Tất cả các yêu cầu trong kế hoạch kỹ thuật `plan.md` đã được triển khai chính xác, kiểm tra biên độ an toàn, không có hiện tượng tràn bộ nhớ hay rò rỉ con trỏ, toàn bộ 37 bài test và smoke test đều đạt PASS 100%.

---

## 2. Nên sửa (Recommended)
1. **Khoảng cách khóa mục tiêu tối đa (Max Lock Distance):**
   - Hiện tại phím `[Tab]` khóa vào tọa độ của Boss không phụ thuộc vào khoảng cách. Nếu người chơi di chuyển quá xa (ví dụ > 50m), camera vẫn cố gắng xoay góc nhìn bám theo Boss ở tít chân trời.
   - *Khuyến nghị:* Nên thêm điều kiện tự động nhả khóa (Auto-unlock) khi khoảng cách giữa người chơi và Boss vượt quá 35 mét hoặc khi Boss bị che khuất hoàn toàn.
2. **Kích thước số sát thương bay (Font Scaling):**
   - Hiện tại các số sát thương bay (`FloatingCombatText`) đang sử dụng font chữ mặc định của ImGui kèm hiệu ứng bóng mờ đen. Trong các bản cập nhật UI tiếp theo, có thể nạp thêm một font số to đậm phong cách MMORPG (như Impact hoặc Trajan Pro) để các con số bạo kích `CRIT!` nhìn hoành tráng hơn nữa.

---

## 3. Gợi ý (Suggestions)
1. **Đa dạng hóa Model vũ khí theo Class:**
   - Script tạo nhân vật `tools/generate_hero_warrior.py` hiện tại đã tạo ra một bộ khung xương Humanoid chuẩn 7 joints cùng thanh Đại Kiếm (Broadsword). Trong tương lai, ta có thể mở rộng generator này để sinh thêm vũ khí cho các hệ class khác: Pháp trượng cho Mage, Song đao cho Assassin, Búa tạ cho Paladin.
2. **Chuyển động Camera Shake khi ăn đòn chí mạng:**
   - Khi Boss tung đòn dậm đất `GroundSlam` hoặc khi người chơi kích hoạt đòn đánh bạo kích lớn, có thể thêm một biên độ rung nhẹ camera (Camera Shake) trong 0.15 giây để tạo cảm giác chấn động mạnh mẽ.

---

## 4. Bảng Đối Chiếu Chi Tiết Với Kế Hoạch (Plan vs Code)

| Hạng mục kế hoạch | Yêu cầu trong `plan.md` | Kết quả triển khai thực tế | Đánh giá |
|:---|:---|:---|:---:|
| **Model 3D Chiến binh (Hero Warrior)** | Thay khối hộp bằng mô hình người 3D có Đại Kiếm và 3 animations | Tạo thành công `assets/models/hero_warrior.gltf` và `.bin`, nạp tự động trong `main.cpp`, chuyển động xương GPU Skinning chân thực. | **ĐẠT** |
| **Số sát thương nảy lên (Floating Text)** | Hiện số sát thương bay lên từ vị trí va chạm 3D | Cài đặt `FloatingCombatText` trong `debug_panel.cpp`, chiếu 3D sang 2D qua ma trận View-Projection, hiển thị số vàng thường, số đỏ bạo kích `CRIT!` và số sát thương nhận từ Boss. | **ĐẠT** |
| **Âm thanh chiến đấu SFX** | Vung kiếm phát tiếng xé gió, chém trúng phát tiếng kim loại | Kích hoạt `audio.playSwing()` khi vung đòn trong `application.cpp` và `audio.playHit()` kèm tia lửa khi trúng mục tiêu. | **ĐẠT** |
| **Hệ thống Khóa mục tiêu (Target Lock)** | Phím Tab khóa góc nhìn bám Boss, hiện hồng tâm trên đầu Boss | Bổ sung `Control::KeyTab` và `Action::TargetLock`, `ThirdPersonCamera` tự động xoay yaw bám Boss, hiển thị hồng tâm ngắm kim cương `[ ◈ ]` và badge trạng thái trên HUD. | **ĐẠT** |
| **Vùng cảnh báo nguy hiểm (Telegraph Area)** | Vòng đỏ phát sáng dưới đất khi Boss sắp ra chiêu | Chiếu vẽ vòng tròn đa giác đỏ rực bán kính 4.8m đập nhịp dưới chân Boss kèm cảnh báo `! DANGER ZONE - [SHIFT] DODGE !`. | **ĐẠT** |
| **Ranh giới đấu trường (Arena Boundary)** | Vòng tròn viền đá đấu trường phân định không gian | Vẽ đường viền bán kính 35m trên mặt sàn đá đấu trường. | **ĐẠT** |
| **Kiểm thử tự động** | Bổ sung test case và pass 100% test suite | Thêm `testTargetLockAndCombatVisuals()` trong `tests/test_game_hud.cpp`, toàn bộ 37/37 bài test đạt 100% PASS. | **ĐẠT** |

---

## 5. Kết Luận Của Reviewer
Giai đoạn triển khai hoàn thành xuất sắc và đáp ứng trọn vẹn 4 hạng mục người dùng yêu cầu. Cho phép chuyển sang bước **TỔNG KẾT** để bàn giao sản phẩm.
