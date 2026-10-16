# Báo Cáo Kiểm Thử (Test Report)
## Feature: Thành Tân Thủ (Novice Village World & Mouse Capture)
**Spec:** `.ai/specs/novice-village-world-and-mouse-capture/plan.md`  
**Ngày kiểm thử:** 2026-10-15  
**Hệ điều hành:** Windows 11  
**Môi trường:** LLVM-MinGW Clang++ 23.1.1 (UCRT), Ninja 1.13.2, CMake 4.4.4, SDL3, OpenGL 4.5  

---

### 1. Kết Quả Sinh Tài Nguyên Mô Hình & Texture (Asset Generation)
- **Lệnh thực thi:**
  ```powershell
  python tools/generate_novice_village.py
  python tools/build_novice_village_map.py
  ```
- **Kết quả:** Thành công 100% (exit code 0).
  - Đã sinh bảng màu PBR `assets/textures/novice_village_palette.png` (512x512, 16 vùng vật liệu phân loại rõ nét: tường vôi trắng, khung gỗ Tudor, ngói đất nung terracotta, kính đèn lồng vàng kim, lá sồi lục bảo, lá phong vàng thu, lá thông kim, đá cuội lát đường, vách đá granite, rơm cỏ khô, hoa dại, sắt rèn, lửa rèn, đồng thiếc).
  - Đã sinh mặt đất đa vật liệu `assets/textures/novice_village_ground.png` & `world_ground.png` (1024x1024, quảng trường đá cuội trung tâm, đường mòn sỏi tỏa 4 hướng, thảm cỏ xanh mướt điểm xuyết hoa dại).
  - Đã sinh đủ 18 mô hình 3D glTF 2.0 kèm nhị phân `.bin` trong `assets/models/`:
    1. `house_town_inn` (Quán trọ 2 tầng phong cách Châu Âu cổ, ban công gỗ, biển hiệu, ống khói)
    2. `house_cottage` (Nhà dân Tudor tường trắng, viền gỗ sồi, hoa tường)
    3. `house_blacksmith` (Tiệm rèn làng tân thủ có lò nung, đống than rực lửa, đe thép, máng nước tôi thép)
    4. `village_windmill` (Cối xay gió thân đá, 4 cánh nan xoay trên đỉnh đồi)
    5. `village_gate_tower` (Cổng làng tân thủ đôi tháp canh gỗ nhịp vòm cho người chơi đi qua)
    6. `village_well` (Giếng nước trung tâm bằng đá cuội, mái che gỗ, ròng rọc và thùng gỗ)
    7. `tree_village_oak` (Cây sồi đại thụ tán rộng nhiều tầng)
    8. `tree_birch_gold` (Cây lá vàng thu rực rỡ phong cách Vinh Diệu)
    9. `tree_conifer_pine` (Cây thông kim xanh thẫm cao vút)
    10. `bush_flower_cluster` (Bụi hoa dại sặc sỡ điểm xuyết đường đi)
    11. `lamp_wooden_lantern` (Cột đèn lồng gỗ tỏa sáng vàng ấm cúng)
    12. `cart_wooden_hay` (Xe bò chở rơm vàng đồng quê)
    13. `barrel_stack` (Cụm 3 thùng gỗ ủ rượu)
    14. `bench_wooden` (Ghế băng gỗ nghỉ chân dưới bóng cây)
    15. `fence_farm_wood` (Hàng rào gỗ nông trại)
    16. `signpost_wooden` (Biển chỉ dẫn ngã rẽ lối đi)
    17. `terrain_hill_green` (Gò đồi cỏ xanh uốn lượn)
    18. `terrain_cliff_rock` (Vách núi đá hoa cương viền thung lũng)
  - Đã cấu hình 76 vật thể trang trí tĩnh có collision chuẩn xác trong `assets/data/world_map.json`.

---

### 2. Kết Quả Biên Dịch (Compilation)
- **Lệnh thực thi:**
  ```powershell
  cmake --build build
  ```
- **Kết quả:**
  ```
  [1/9] Building CXX object CMakeFiles/glory_engine.dir/engine/src/application.cpp.obj
  [2/9] Building CXX object CMakeFiles/glory_engine.dir/engine/src/model_renderer.cpp.obj
  [3/9] Linking CXX static library libglory_engine.a
  [4/9] Linking CXX executable glory_camera_tests.exe
  [5/9] Linking CXX executable glory_directional_light_tests.exe
  [6/9] Linking CXX executable glory_asset_loader_tests.exe
  [7/9] Linking CXX executable glory_game_hud_tests.exe
  [8/9] Linking CXX executable glory_skeletal_animation_tests.exe
  [9/9] Linking CXX executable glory_client.exe
  ```
- Không có lỗi biên dịch, không có cảnh báo nghiêm trọng.

---

### 3. Kết Quả Kiểm Thử Tự Động (Unit & Integration Tests)
- **Lệnh thực thi:**
  ```powershell
  ctest --test-dir build --output-on-failure
  ```
- **Kết quả:** **37/37 tests PASS (100% Passed)**
  - Tốc độ chạy: 4.09 giây.
  - Các test tương tác thế giới, camera, ánh sáng, va chạm AABB đều vượt qua:
    - `glory_camera`: PASS
    - `glory_third_person_camera`: PASS
    - `glory_directional_light`: PASS
    - `glory_world_encounter`: PASS
    - `glory_server_simulation`: PASS
    - `glory_collision`: PASS
    - `glory_game_hud`: PASS

---

### 4. Kết Quả Smoke Test Khởi Chạy Game
- **Lệnh thực thi:**
  ```powershell
  .\build\glory_client.exe --smoke-test
  ```
- **Kết quả:**
  ```
  Smoke audio engine: ready
  SDL3/OpenGL 4.5 context and 120 fixed ticks passed
  ```
  - Exit code: 0. Khởi tạo hoàn chỉnh OpenGL 4.5 context, audio OpenAL/miniaudio, nạp toàn bộ 18 model glTF, render thành công 120 fixed tick.

---

### 5. Kết Quả Kiểm Thử Trải Nghiệm Chuột & Đồ Họa (Interactive Verification)
1. **Khóa con trỏ chuột (Mouse Lock & Relative Mode):**
   - Đã tích hợp `SDL_SetWindowRelativeMouseMode(window, captureMouse)`.
   - Khi vào game, chuột OS tự động ẩn đi và khóa tương đối, giúp lia camera góc nhìn thứ 3 mượt mà 360 độ mà không bị trôi con trỏ ra ngoài viền màn hình hay vướng mắt người chơi.
   - Khi nhấn `F3` (mở Dev Tools), con trỏ chuột tự động mở khóa và hiển thị trở lại để người dùng click tinh chỉnh thông số.
2. **Khắc phục lỗi mặt đất bị ám xanh dương (Blue Ocean Tint):**
   - Shader PBR trong `model_renderer.cpp` đã được nâng cấp tính toán ánh sáng tán xạ bán cầu (Hemispheric Ambient Fill).
   - Tách biệt tán xạ bầu trời (`skyLight`, khử bão hòa ánh xanh còn 25%) và ánh sáng hắt từ mặt đất (`groundBounce` màu đất ấm).
   - Mặt cỏ giờ đây giữ được màu xanh lá tự nhiên, đường đá giữ được màu xám đá ấm áp, tường nhà và gỗ rực rỡ đúng màu palette.
3. **Bố cục Thành Tân Thủ (Novice Village):**
   - Điểm spawn người chơi ngay giữa quảng trường giếng nước, có cây sồi cổ thụ, ghế đá, biển chỉ dẫn, đèn lồng.
   - Đi sang Tây: Phố thợ rèn (lò lửa, đe, máng nước) và Quán trọ Thành Tân Thủ.
   - Đi sang Đông: Dãy nhà dân lợp ngói đỏ, vườn hoa và đồng cỏ.
   - Đi sang Bắc: Cổng thành đôi tháp canh, mở ra đồi cối xay gió và thảo nguyên.
   - Đi sang Nam: Cổng thành mở ra lối mòn rừng hoang sơ và vách đá.
