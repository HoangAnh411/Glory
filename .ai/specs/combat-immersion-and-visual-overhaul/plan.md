# Kế Hoạch Kỹ Thuật: Đại Tu Trải Nghiệm Chiến Đấu, Hình Ảnh Nhân Vật & Môi Trường Đấu Trường (Combat Immersion & Visual Overhaul)

## 1. Mục Tiêu (Objective)
Nâng cấp toàn diện trải nghiệm chơi của Glory từ một bản kỹ thuật prototype thành một tựa game 3D Action MMORPG thực thụ với 4 trụ cột chính:
1. **Mô hình 3D Nhân vật Chiến binh & Boss (Humanoid Characters):**
   - Thay thế khối hộp gỗ (`starter_crate.gltf`) bằng mô hình Chiến binh 3D (Humanoid Skeletal Mesh) hoàn chỉnh với đầy đủ đầu, thân, tay cầm Đại Kiếm (Broadsword), chân và hoạt ảnh chuyển động xương mượt mà: `Idle` (thế thủ kiếm), `Run` (chạy nước rút), `Attack` (chém vung đại kiếm).
   - Boss Iron Warden to lớn, uy dũng với tông màu thiết giáp kim loại đen huyền.
2. **Cảm giác Chiến đấu Đã Tay (Combat Game Feel, Floating Numbers & Audio VFX):**
   - **Số sát thương nảy lên (Floating Damage Numbers):** Khi chém trúng quái/boss, các con số sát thương màu vàng kim rực rỡ (`-125`) và số bạo kích màu đỏ nổi bật (`CRIT! -280`) nảy lên từ vị trí va chạm trong không gian 3D và bay lượn trên màn hình.
   - **Âm thanh & Hiệu ứng va chạm (SFX & Hit Sparks):** Âm thanh chém gió `swing.wav` phát ra ngay khi vung kiếm / tung chiêu `1..4`, âm thanh kim loại va chạm `hit.wav` cùng hiệu ứng tia lửa tóe ra rực rỡ khi trúng đòn.
3. **Hệ thống Khóa Mục Tiêu (Target Lock-On) & Camera Hành Động:**
   - Phím `[Tab]` khóa mục tiêu vào Boss.
   - Khi Lock-on: Camera tự động xoay bám theo Boss, nhân vật tự động hướng mặt về đối thủ và di chuyển dạt vòng (strafe orbit) xung quanh Boss.
   - Hiển thị tâm ngắm khóa mục tiêu (Diamond Lock Reticle `[ < ◈ > ]`) rực sáng trên đỉnh đầu Boss.
4. **Cảnh Báo Chiêu Thức Boss & Đấu Trường Sống Động (Telegraph Danger Zone & Arena Visuals):**
   - **Vòng tròn đỏ cảnh báo chiêu Boss (Red Danger Zone / Telegraph Indicator):** Khi Boss chuẩn bị tung đòn dậm đất hoặc đòn quét mạnh, một vòng tròn cảnh báo nguy hiểm màu đỏ phát sáng xuất hiện trên mặt đất dưới chân Boss, báo hiệu cho người chơi bấm `[Shift] Dodge` để né đòn bất tử (I-frame).
   - **Ranh giới đấu trường (Arena Perimeter):** Hiển thị vòng ranh giới đấu trường La Mã rõ nét trên mặt sàn đá để người chơi định vị không gian.

---

## 2. Danh Sách Task Nhỏ (Tasks)

- [ ] **Task 1: Tạo Mô hình 3D Chiến binh & Boss dạng Humanoid Skeletal glTF 2.0**
  - Viết generator script tạo mô hình chuẩn glTF 2.0 [`assets/models/hero_warrior.gltf`](file:///d:/Data/Glory/assets/models/hero_warrior.gltf) và binary data:
    - Khung xương Humanoid: Root, Pelvis, Spine, Chest, Head, Shoulders, Arms, Hands, Weapon, Thighs, Calves, Feet.
    - Mesh chiến binh đội mũ giáp, giáp ngực, tay nắm chặt Đại Kiếm thép sáng bạc.
    - 3 Hoạt ảnh xương GPU Skinning hoàn chỉnh: `Idle`, `Run`, `Attack` (chém đại kiếm có nhịp vung và giật đòn uy lực).
  - Cập nhật cấu hình mô hình mặc định trong [`engine/include/glory/engine/application.hpp`](file:///d:/Data/Glory/engine/include/glory/engine/application.hpp) và [`game/src/main.cpp`](file:///d:/Data/Glory/game/src/main.cpp).

- [ ] **Task 2: Hệ thống Hiển thị Số Sát Thương Bay (Floating Damage Numbers) & Âm thanh Chiến đấu**
  - Trong [`engine/include/glory/engine/application.hpp`](file:///d:/Data/Glory/engine/include/glory/engine/application.hpp) và [`engine/src/debug_panel.cpp`](file:///d:/Data/Glory/engine/src/debug_panel.cpp):
    - Quản lý danh sách `FloatingCombatText` có tọa độ 3D, lượng sát thương, loại đòn (Normal vs Crit vs PlayerHurt), vận tốc bay lên và thời gian mờ dần.
    - Sử dụng ma trận `viewProjectionMatrix` chiếu tọa độ 3D sang 2D màn hình, vẽ chữ nổi MMORPG viền đen bóng mờ.
  - Trong [`engine/src/application.cpp`](file:///d:/Data/Glory/engine/src/application.cpp) & [`game/src/main.cpp`](file:///d:/Data/Glory/game/src/main.cpp):
    - Kích hoạt `audio.playSwing()` khi vung đòn/tung chiêu và `audio.playHit()` khi đòn đánh kết nối trúng mục tiêu.

- [ ] **Task 3: Hệ thống Khóa Mục Tiêu (Target Lock-On System)**
  - Cập nhật [`shared/include/glory/input/input_frame.hpp`](file:///d:/Data/Glory/shared/include/glory/input/input_frame.hpp) và [`engine/src/input_system.cpp`](file:///d:/Data/Glory/engine/src/input_system.cpp): Bổ sung `Control::KeyTab` (map `SDL_SCANCODE_TAB`).
  - Cập nhật [`game/include/glory/game/action_map.hpp`](file:///d:/Data/Glory/game/include/glory/game/action_map.hpp) và [`game/src/action_map.cpp`](file:///d:/Data/Glory/game/src/action_map.cpp): Thêm `Action::TargetLock` gắn với `KeyTab`.
  - Cập nhật [`engine/include/glory/engine/third_person_camera.hpp`](file:///d:/Data/Glory/engine/include/glory/engine/third_person_camera.hpp): Thêm cơ chế bám góc nhìn mục tiêu khi Lock-on (`setLockTarget`).
  - Trong [`engine/src/debug_panel.cpp`](file:///d:/Data/Glory/engine/src/debug_panel.cpp): Vẽ Diamond Reticle `[ ◈ ]` màu hổ phách/đỏ trên đỉnh đầu Boss khi đang bị khóa mục tiêu.

- [ ] **Task 4: Vòng Cảnh Báo Nguy Hiểm Của Boss (Telegraph Danger Zone) & Ranh Giới Đấu Trường**
  - Trong [`engine/include/glory/engine/application.hpp`](file:///d:/Data/Glory/engine/include/glory/engine/application.hpp) và [`engine/src/debug_panel.cpp`](file:///d:/Data/Glory/engine/src/debug_panel.cpp):
    - Đọc trạng thái đòn đánh của Boss từ `bossAttackName` và `bossBehaviorNode`.
    - Khi Boss chuẩn bị nạp đòn công kích diện rộng, chiếu vẽ vòng tròn cảnh báo nguy hiểm màu đỏ rực (Danger Zone Telegraph Indicator) trên mặt đất xung quanh mục tiêu/boss với hiệu ứng đập nhịp (pulsing).
    - Vẽ đường viền ranh giới đấu trường bán kính an toàn trên sàn đấu.

- [ ] **Task 5: Kiểm Thử Tự Động & Thẩm Định Kỹ Thuật**
  - Viết unit test cho `TargetLock` và `FloatingCombatTextCalculations` trong [`tests/test_game_hud.cpp`](file:///d:/Data/Glory/tests/test_game_hud.cpp).
  - Chạy `cmake --build build`, `ctest` và smoke test đảm bảo 100% tests PASS sạch sẽ.

---

## 3. Danh Sách File Cần Chỉnh Sửa / Tạo Mới (Files to touch)
- `shared/include/glory/input/input_frame.hpp`
- `engine/src/input_system.cpp`
- `game/include/glory/game/action_map.hpp`
- `game/src/action_map.cpp`
- `engine/include/glory/engine/third_person_camera.hpp`
- `engine/include/glory/engine/application.hpp`
- `engine/src/application.cpp`
- `engine/src/debug_panel.cpp`
- `game/src/main.cpp`
- `tests/test_game_hud.cpp`
- `assets/models/hero_warrior.gltf` (và binary kèm theo)

---

## 4. Rủi Ro Tiềm Ẩn & Giải Pháp Xử Lý (Risks & Mitigations)
- **Rủi ro 1 (Độ tương thích glTF 2.0 với parser cgltf hiện tại):**
  - *Giải pháp:* Generator script sẽ tuân thủ nghiêm ngặt chuẩn glTF 2.0 giống hệt cấu trúc của `starter_crate.gltf` (chỉ dùng các accessor kiểu `VEC3`, `VEC4`, `MAT4`, các joint node chuẩn, inverse bind matrices đúng kích thước 16 floats/joint).
- **Rủi ro 2 (Chiếu tọa độ 3D sang 2D màn hình cho Damage Numbers):**
  - *Giải pháp:* Kiểm tra tọa độ clip space $w > 0.001$ để loại bỏ các điểm nằm phía sau camera, tránh việc vẽ số sát thương bị lật ngược khi kẻ địch ở sau lưng người chơi.
- **Rủi ro 3 (Xung đột camera giữa chuột tự do và Lock-On):**
  - *Giải pháp:* Khi đang Lock-On, camera mượt mà nội suy góc Yaw hướng về Boss nhưng vẫn cho phép chuột tác động nhẹ góc Pitch (lên/xuống), tạo cảm giác điều khiển tự nhiên như các game Souls-like.

---

## 5. Tiêu Chí Chấp Nhận (Acceptance Criteria)
1. **Nhân vật người 3D:** Khối hộp gỗ được thay thế bằng hình hài Chiến binh 3D cầm Đại Kiếm với cử động chân tay nhịp nhàng khi đứng (`Idle`), chạy (`Run`) và chém kiếm (`Attack`).
2. **Số sát thương bay (Floating Combat Numbers):** Đòn đánh trúng hiện rõ số sát thương nảy lên (`-125`, `CRIT! -280`) có hiệu ứng bay bổng và mờ dần trong không gian 3D.
3. **Âm thanh chiến đấu SFX:** Tiếng vung kiếm xé gió phát ra khi vung đòn, tiếng kim loại nện trúng phát ra khi đánh trúng mục tiêu.
4. **Khóa mục tiêu (Target Lock-On):** Bấm `[Tab]` khóa/mở khóa góc nhìn bám Boss, xuất hiện hồng tâm ngắm `[ ◈ ]` trên đầu Boss.
5. **Cảnh báo chiêu thức (Boss Telegraph):** Xuất hiện vòng đỏ cảnh báo nguy hiểm dưới chân khi Boss chuẩn bị ra đòn, giúp người chơi canh nhịp né đòn.
6. **Bộ test suite:** Tất cả các bài kiểm thử tự động và smoke test đều **PASS 100%**.
