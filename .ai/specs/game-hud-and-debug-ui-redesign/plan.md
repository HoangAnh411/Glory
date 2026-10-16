# Kế Hoạch Kỹ Thuật: Nâng Cấp Giao Diện Game (Game HUD & Debug Tools Redesign)

## 1. Mục Tiêu (Objective)
- Giải quyết triệt để tình trạng giao diện bị đè chữ, hỗn loạn và choán toàn bộ màn hình khi người chơi vào game.
- Thiết kế và tích hợp **Game HUD (Heads-Up Display) MMORPG thực thụ**:
  - Khung thông tin nhân vật (Player Frame: HP bar, Class, Tên) ở góc trên bên trái.
  - Thanh kỹ năng chiến đấu (Skill Action Bar) ở đáy màn hình với hiển thị phím tắt, thời gian hồi chiêu (cooldown sweep/timer) trực quan.
  - Thanh máu Boss (Boss Health Bar) uy lực ở giữa phía trên màn hình (tự động kích hoạt khi Boss vào trạng thái chiến đấu hoặc nhận sát thương).
  - Tâm ngắm chiến đấu (Action Combat Reticle) ở chính giữa màn hình.
  - Hiển thị hiệu ứng trạng thái (Buff/Debuff, Dodge Cooldown, Combo Step, I-Frame indicator).
- Đưa toàn bộ các công cụ phát triển vào **Chế độ Dev Tools ẩn/hiện bằng phím F3**:
  - Mặc định khi vào game: Debug Tools được ẩn hoàn toàn (`devToolsVisible_ = false`), input chuột và phím 100% dành cho gameplay.
  - Khi nhấn phím `F3`: Kích hoạt một cửa sổ **Glory Dev Tools Hub** duy nhất với hệ thống 6 Tabs phân loại khoa học (Diagnostics, Skills & Combat, World & Spawners, Multiplayer, Economy & Social, Anti-Cheat), chấm dứt hoàn toàn việc các panel bị sinh ra đè chồng chéo tại tọa độ `(0, 0)`.
- Tinh chỉnh ánh sáng môi trường (`ambientStrength`) trong `DirectionalLight` từ `0.18F` lên `0.35F` để mặt khuất của vật thể 3D trong game không bị tối đen thui (pitch black).

---

## 2. Danh Sách Task Nhỏ (Tasks)

- [ ] **Task 1 (Game HUD System):** Xây dựng hệ thống Game HUD trong `engine/include/glory/engine/debug_panel.hpp` và `engine/src/debug_panel.cpp`:
  - Thêm phương thức `renderGameHud(...)`.
  - Thiết kế Player HP Bar dạng bo góc với màu gradient đỏ/xanh lá, hiển thị số máu `current / max` và phần trăm.
  - Thiết kế Skill Action Bar tại đáy màn hình với các ô kỹ năng (phím tắt `1, 2, 3, 4` hoặc `Q, E, R`), hiển thị tên skill, trạng thái sẵn sàng hoặc lớp phủ xám + thanh đo thời gian hồi chiêu (cooldown).
  - Thiết kế Boss Health Bar lớn ở giữa trên màn hình với tên Boss, thanh HP đỏ và phase hiện tại (`Phase 1` / `Phase 2: ENRAGED`).
  - Thiết kế Combat Reticle (dấu thập tâm ngắm mờ) và trạng thái bất tử (I-Frame) / Dodge Cooldown indicator.
  - Đảm bảo Game HUD dùng cờ `ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground` để không cản trở thao tác điều khiển nhân vật.

- [ ] **Task 2 (Debug Toggle & Input Clean-up):**
  - Bổ sung biến trạng thái `devToolsVisible_` trong `DebugPanel` (mặc định = `false`).
  - Trong `DebugPanel::processEvent`, bắt sự kiện phím `SDLK_F3` để đảo trạng thái `devToolsVisible_`.
  - Đảm bảo khi `devToolsVisible_ == false`, `wantsCaptureMouse()` và `wantsCaptureKeyboard()` không nuốt chuột/phím của người chơi (trừ khi cửa sổ F8 Feedback đang mở).

- [ ] **Task 3 (Dev Tools Hub & Fix Stacking Bug):**
  - Tái cấu trúc toàn bộ các cửa sổ debug rời rạc (`Glory Debug`, `Skill & Event Editor`, `World & Spawner Editor`, `Anti-Cheat Monitor`, `Arena Matchmaking`, `Social`, `Marketplace`) vào một cửa sổ trung tâm có kích thước cố định hoặc dockable mang tên `Glory Dev Tools (F3)`.
  - Phân thành 6 tab rõ ràng:
    1. `Diagnostics & Engine`: FPS, Camera position/yaw/pitch, PBR sliders, Skybox toggle, Directional Light controls, Audio, Particles.
    2. `Combat & Skills`: Skill Editor (Name, damage, cooldown, combo feel, hitbox offset), Timeline scrubber, Animation event markers, Dummy reset.
    3. `World & Spawners`: Spawn points, Trigger zones, Snap to Player, Save/Reload JSON.
    4. `Multiplayer & Arena`: Server ticks, Snapshot interpolation diagnostics, Reconciliation stats, Arena 1v1 / 3v3 Matchmaking controls, Zone AOI, Dungeon.
    5. `Account & Economy`: Account login/register, Character selection & creation, Inventory & Equipment slots, Marketplace browser.
    6. `Anti-Cheat Monitor`: Speed/teleport/cooldown violation stats, probe injection testing.
  - Khắc phục triệt để lỗi không đóng thẻ `ImGui::Begin("Glory Debug")` trước khi mở `Anti-Cheat Monitor` trong code cũ.

- [ ] **Task 4 (Lighting Polish):**
  - Cập nhật `DirectionalLight` trong `engine/include/glory/engine/directional_light.hpp`:
    - Tăng `ambientStrength` từ `0.18F` lên `0.35F` giúp các bề mặt khuất sáng vẫn nhìn thấy rõ chi tiết chất liệu thay vì bị đen hoàn toàn.

- [ ] **Task 5 (Unit Testing & Acceptance Verification):**
  - Tạo file `tests/test_game_hud.cpp`: Kiểm tra logic toggle DevTools, kiểm tra tính toán tỷ lệ thanh máu/cooldown của HUD, kiểm tra tính năng bắt sự kiện F3.
  - Đăng ký test mới vào `CMakeLists.txt`.
  - Biên dịch dự án bằng Clang++ / Ninja và chạy toàn bộ test suite `ctest`.

---

## 3. Danh Sách File Cần Sửa / Tạo Mới (Files to touch)
- `engine/include/glory/engine/debug_panel.hpp` (Thêm API toggle dev tools, getters trạng thái)
- `engine/src/debug_panel.cpp` (Triển khai Game HUD, cấu trúc lại Dev Tools TabBar, xử lý F3)
- `engine/include/glory/engine/directional_light.hpp` (Nâng ambient strength)
- `tests/test_game_hud.cpp` (File test mới cho logic HUD & toggle)
- `CMakeLists.txt` (Khai báo test `glory_game_hud`)

---

## 4. Rủi Ro Tiềm Ẩn & Giải Pháp (Risks & Mitigations)
- **Rủi ro 1:** ImGui Game HUD có thể vô tình chặn chuột khi người chơi muốn xoay camera.
  - *Giải pháp:* Sử dụng flag `ImGuiWindowFlags_NoInputs` cho tất cả các window thuộc Game HUD, chỉ mở nhận input khi người dùng bật `F3` hoặc `F8`.
- **Rủi ro 2:** Một số tính năng callback (Save skills, World editor, Account action) có thể bị gãy khi gộp vào TabBar.
  - *Giải pháp:* Giữ nguyên 100% chữ ký hàm callback và biến state hiện có, chỉ thay đổi layout chứa các widget từ các cửa sổ độc lập sang các TabItem bên trong cùng một cửa sổ `Glory Dev Tools`.

---

## 5. Tiêu Chí Chấp Nhận (Acceptance Criteria)
1. **Khởi động game:** Không còn cảnh tượng các cửa sổ debug đè chữ lên nhau tại góc trái. Màn hình game trong trẻo, không bị che khuất tầm nhìn.
2. **Game HUD:**
   - Góc trên trái hiển thị thanh HP người chơi kèm class name và cấp độ.
   - Đáy màn hình hiển thị thanh Skill Bar với các kỹ năng và vòng quét hồi chiêu (cooldown).
   - Khi đánh Boss hoặc Boss xuất hiện, thanh Boss HP hiển thị ở giữa phía trên màn hình.
   - Có tâm ngắm chiến đấu ở chính giữa màn hình.
3. **Phím tắt F3:**
   - Nhấn `F3`: Cửa sổ `Glory Dev Tools` bật lên với 6 Tab phân loại rõ ràng, không bị đè chữ.
   - Nhấn lại `F3`: Cửa sổ biến mất, trả lại không gian chơi game sạch sẽ.
4. **Ánh sáng 3D:** Khối hộp mẫu và môi trường có ánh sáng môi trường mềm mại hơn, không còn bị đen hoàn toàn ở mặt khuất.
5. **Kiểm thử:** Biên dịch thành công 0 warning/error, toàn bộ 37/37 tests PASS 100%.
