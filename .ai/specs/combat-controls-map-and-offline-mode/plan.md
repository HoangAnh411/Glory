# Kế Hoạch Kỹ Thuật: Hoàn Thiện Hệ Thống Điều Khiển Kỹ Năng, Bản Đồ Sàn Đấu & Chế Độ Chơi (Combat Controls, Arena Map & Play Modes)

## 1. Mục Tiêu (Objective)
Giải quyết triệt để 3 vấn đề người chơi vừa phản hồi:
1. **Kích hoạt phím bấm kỹ năng 1, 2, 3, 4 & Chuột trái (LMB):**
   - Bổ sung định nghĩa các phím số `Key1, Key2, Key3, Key4` vào hệ thống Input của engine (`InputSystem`) và bản đồ hành động (`ActionMap`).
   - Kết nối phím `1, 2, 3, 4` để khi người chơi bấm trên bàn phím sẽ tung đúng kỹ năng tương ứng trên Skill Bar (`Quick Slash`, `Heavy Strike`, `Whirlwind`, v.v.).
   - Hỗ trợ click chuột trực tiếp vào icon kỹ năng trên thanh HUD để tung chiêu.
2. **Khắc phục nhầm lẫn phím Né đòn (Dodge) & Nhảy (Jump):**
   - Đổi nhãn trên thanh HUD thành `[Shift] Dodge` để khớp với phím `LeftShift` trong engine, đồng thời hiển thị `[Space] Jump` rõ ràng cho người chơi.
3. **Cơ chế dự phòng Offline / Online thông minh & Khởi động game mượt mà:**
   - Trong `main.cpp`: Khi client không có kết nối server (server chưa bật hoặc rớt mạng), client tự động chạy vòng lặp mô phỏng cục bộ (Offline Sandbox) thay vì bị khóa input và đóng băng.
   - Trong `play_game.bat`: Cung cấp menu lựa chọn nhanh (1: Chơi Offline ngay lập tức; 2: Tự động khởi động Dedicated Server chạy ngầm rồi kết nối).
   - Thêm nút "Play Offline (Chơi Ngay)" trên cửa sổ đăng nhập để bỏ qua kết nối server nếu muốn test ngay.
4. **Mở rộng mặt sàn đấu (Arena Ground Mesh):**
   - Mở rộng kích thước mặt sàn `GroundPlane` trong engine thành đấu trường 60x60 mét tại cao độ `Y = 0.0F` (trùng khớp với mặt phẳng va chạm của nhân vật), với màu sàn đá đấu trường tương phản rõ nét để người chơi nhìn thấy rõ không gian di chuyển.

---

## 2. Danh Sách Task Nhỏ (Tasks)

- [ ] **Task 1 (Input System & Action Map Key1..Key4):**
  - Cập nhật `shared/include/glory/input/input_frame.hpp`: thêm `Key1, Key2, Key3, Key4` vào enum `Control`.
  - Cập nhật `engine/src/input_system.cpp`: ánh xạ các scancode `SDL_SCANCODE_1`, `SDL_SCANCODE_2`, `SDL_SCANCODE_3`, `SDL_SCANCODE_4` thành `Control::Key1..Key4`.
  - Cập nhật `game/include/glory/game/action_map.hpp` và `game/src/action_map.cpp`:
    - Thêm các action `Skill1, Skill2, Skill3, Skill4` vào enum `Action`.
    - Gán mặc định `Skill1 -> Key1`, `Skill2 -> Key2`, `Skill3 -> Key3`, `Skill4 -> Key4`.

- [ ] **Task 2 (Main Loop Skill Execution & Offline Fallback):**
  - Cập nhật `game/src/main.cpp`:
    - Kiểm tra các action `Skill1, Skill2, Skill3, Skill4`. Khi được bấm (pressed), lấy skill ID tương ứng từ `combat.skillStatuses()` hoặc class skill registry và gán vào `requestedSkillId`.
    - Sửa nhánh mạng: Chỉ chuyển sang authoritative server khi `networkClient->state() == ConnectionState::Connected && accountPanel.authenticated`. Nếu đang ở trạng thái `Connecting` hoặc `Disconnected`, tiếp tục chạy simulation cục bộ mượt mà (người chơi di chuyển, chém, né, đánh Boss thoải mái).

- [ ] **Task 3 (HUD Interaction & Labels Polish):**
  - Cập nhật `engine/src/debug_panel.cpp`:
    - Cho phép click chuột vào từng ô skill trên action bar để gán `requestedSkillId = skill.id`.
    - Sửa nhãn hiển thị nút né đòn: `[Shift] Dodge` thay vì `[Space]`.
    - Thêm nút "Play Offline / Sandbox" trong hộp thoại đăng nhập.

- [ ] **Task 4 (Arena Ground Mesh Polish):**
  - Cập nhật `engine/src/ground_plane.cpp` và `engine/src/model_renderer.cpp`:
    - Điều chỉnh `floorY = 0.0F` và mở rộng bán kính sàn đấu từ 10m lên 60m (`-60.0F` đến `60.0F`).
    - Đặt màu sàn đá đấu trường sáng rõ và có texture UV lặp lại đều đặn.

- [ ] **Task 5 (Launcher & Unit Tests):**
  - Nâng cấp `play_game.bat` hỗ trợ cả 2 chế độ: Chơi Offline ngay và Chơi Online tự khởi động server.
  - Viết test trong `tests/test_game_hud.cpp` kiểm tra các action `Skill1..Skill4` và input mapping.
  - Chạy `ctest` và smoke test đảm bảo 100% tests PASS.

---

## 3. Danh Sách File Cần Sửa (Files to touch)
- `shared/include/glory/input/input_frame.hpp`
- `engine/src/input_system.cpp`
- `game/include/glory/game/action_map.hpp`
- `game/src/action_map.cpp`
- `game/src/main.cpp`
- `engine/src/debug_panel.cpp`
- `engine/src/ground_plane.cpp`
- `engine/src/model_renderer.cpp`
- `play_game.bat`
- `tests/test_game_hud.cpp`

---

## 4. Tiêu Chí Chấp Nhận (Acceptance Criteria)
1. **Bấm phím 1, 2, 3, 4 trên bàn phím:** Nhân vật tung ra đúng các chiêu thức tương ứng trên Skill Bar, thanh hồi chiêu bắt đầu đếm ngược.
2. **Click chuột vào ô skill trên màn hình:** Tung chiêu tương ứng.
3. **Chuột trái (LMB):** Thực hiện đòn đánh cơ bản / combo slash chém trúng Boss/Dummy.
4. **Bấm Shift:** Thực hiện động tác né đòn (Dodge) với thanh cooldown đếm ngược, hiển thị huy hiệu `[I-FRAME]`.
5. **Bấm Space:** Thực hiện động tác nhảy (Jump).
6. **Mặt sàn đấu:** Nhìn thấy rõ ràng mặt sàn đấu đá rộng 60x60m ngay dưới chân nhân vật và boss, không còn cảm giác lơ lửng trong không gian rỗng.
7. **Chạy không cần server:** Bật game lên là chơi được ngay lập tức ở chế độ Offline Sandbox, không bị kẹt vì không có server.
8. **Toàn bộ test suite PASS 100%.**
