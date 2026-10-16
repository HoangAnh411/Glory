# Kế Hoạch Kỹ Thuật (Technical Plan): Làng Tân Thủ Sống Động & Ẩn Chuột Khi Chơi (Novice Village & Mouse Capture)

- **Mã tính năng:** `novice-village-world-and-mouse-capture`
- **Thư mục tài liệu:** `.ai/specs/novice-village-world-and-mouse-capture/`
- **Chế độ:** Auto-Pilot (PLANNER ➔ CODER ➔ TESTER ➔ REVIEWER ➔ SUMMARY)
- **Mục tiêu cốt lõi:**
  1. **Khắc phục lỗi con trỏ chuột:** Tự động ẩn con trỏ chuột hệ điều hành và khóa chuột tương đối (`SDL_SetWindowRelativeMouseMode`) khi đang chơi để xoay camera mượt mà, chỉ nhả chuột khi mở bảng Dev Tools (F3).
  2. **Xây dựng Thành Tân Thủ (Novice Town / Tân Thủ Thôn):** Một ngôi làng fantasy tuyệt đẹp lấy cảm hứng từ Tân Thủ Thôn trong *Toàn Chức Cao Thủ*, đầy đủ màu sắc, chi tiết, cho phép người chơi đi dạo khám phá:
     - **Địa hình & Mặt đất:** Tinh chỉnh shader mặt đất khử hoàn toàn ánh xanh ngắt; phủ gạch đá cuội quảng trường làng, đường mòn đất cát, thảm cỏ xanh mướt điểm hoa dã quỳ, các triền đồi thoải xanh tươi.
     - **Đồi núi bao bọc:** Các dãy đồi xanh (`terrain_hill_green`) và vách đá sa thạch (`terrain_cliff_rock`) bao bọc quanh làng tạo thành thung lũng sơn thủy hữu tình, xóa bỏ cảm giác sàn đấu phẳng lì trơ trọi.
     - **Công trình kiến trúc Tân Thủ Thôn:** Quán trọ 2 tầng có ban công (`house_town_inn`), nhà dân nông thôn mái ngói đỏ vách vôi trắng khung gỗ (`house_cottage`), lò rèn than hồng (`house_blacksmith`), cối xay gió trên triền đồi (`village_windmill`), cổng làng tháp canh gỗ (`village_gate_tower`), giếng nước đá cuội trung tâm (`village_well`).
     - **Thảm thực vật & Tự nhiên:** Cây sồi đại thụ ngàn năm (`tree_village_oak`), cây phong lá vàng mùa thu (`tree_birch_gold`), cây thông triền núi (`tree_conifer_pine`), các bồn hoa dại rực rỡ (`bush_flower_cluster`).
     - **Đồ trang trí đời sống:** Cột đèn lồng gỗ thắp nến ấm áp (`lamp_wooden_lantern`), xe kéo chở rơm vàng (`cart_wooden_hay`), cụm thùng rượu sồi (`barrel_stack`), ghế nghỉ chân (`bench_wooden`), hàng rào gỗ nông trang (`fence_farm_wood`), biển chỉ đường gỗ (`signpost_wooden`).
  3. **Vật lý va chạm & Trải nghiệm đi dạo:**
     - Đặt vị trí xuất phát người chơi ngay tại trung tâm Quảng trường Làng, trước Giếng Nước và Cây Đại Thụ.
     - Đăng ký hộp va chạm AABB cho tất cả nhà cửa, cối xay, giếng nước, hàng rào, cây cối để người chơi không đi xuyên vật thể.

---

## 1. Phân Tích & Giải Pháp Kỹ Thuật

### 1.1. Ẩn Con Trỏ Chuột (Mouse Cursor Capture)
- **Vấn đề:** Hiện tại SDL3 chưa gọi `SDL_SetWindowRelativeMouseMode(window, true)`, dẫn tới con trỏ chuột Windows luôn hiển thị chình ình đè lên tâm ngắm nhân vật.
- **Giải pháp:**
  - Trong game loop của `application.cpp`:
    ```cpp
    bool wantsUiMouse = debugPanel.wantsCaptureMouse() || feedbackPanelOpen;
    SDL_SetWindowRelativeMouseMode(window, !wantsUiMouse);
    ```
  - Khi chơi: Chuột bị ẩn hoàn toàn, mọi di chuyển chuột dùng để xoay camera mượt mà tự do chuẩn TPS.
  - Khi mở F3 Dev Tools: Chuột tự động hiện lại để bấm giao diện.

### 1.2. Khử Màu Xanh Của Mặt Đất (Ground Ambient Lighting & Texture Tiling)
- **Vấn đề:** Shader PBR hiện tại lấy `environmentDiffuse = skybox(0, 1, 0) * baseColor * 0.55`. Vì đỉnh vòm skybox có màu xanh dương thẳm `(0.12, 0.38, 0.75)`, mọi mặt phẳng hướng lên trời (đặc biệt là mặt đất) bị nhuộm một lớp màu xanh nước biển trông như mặt nước.
- **Giải pháp:**
  - Đối với mặt đất và vật thể ngoài trời, pha trộn ánh sáng môi trường hướng lên với ánh sáng phản xạ đất ấm `vec3(0.55, 0.50, 0.42)` thay vì thuần xanh skybox.
  - Tăng độ phân giải và chi tiết cho texture mặt đất, lặp vân đá/cỏ/đất sắc nét.

### 1.3. Hệ Thống Model & Bảng Màu Thủ Công Phong Phú
- Tạo script `tools/generate_novice_village.py`:
  - `assets/textures/novice_village_palette.png` (512x512) chứa 16 vùng chất liệu tươi sáng, giàu chi tiết: Vách vôi trắng, khung gỗ sồi già, ngói nung đỏ cam tươi tắn, kính đèn vàng rực, lá xanh biếc, lá vàng kim, rơm vàng, đá cuội, sắt rèn, nước trong vắt.
  - Sinh 18 model 3D glTF 2.0 phân loại rõ ràng theo đúng yêu cầu người dùng:
    - Nhóm Kiến trúc: Nhà trọ, Nhà dân, Lò rèn, Cối xay gió, Cổng tháp canh, Giếng nước.
    - Nhóm Tự nhiên: Cây đại thụ, Cây phong vàng, Cây thông, Bụi hoa cỏ dại, Đồi cỏ xanh, Vách đá núi.
    - Nhóm Đời sống: Cột đèn gỗ, Xe chở rơm, Thùng gỗ, Ghế gỗ, Hàng rào, Biển chỉ đường.

---

## 2. Tiêu Chí Nghiệm Thu (Acceptance Criteria)

1. [x] Con trỏ chuột Windows được ẩn hoàn toàn khi đang chơi game; xoay camera góc nhìn thứ 3 mượt mà không vướng chuột. Nhấn F3 chuột tự động xuất hiện để bấm Dev Tools.
2. [x] Toàn bộ 18 model 3D Làng Tân Thủ được sinh ra với màu sắc PBR rực rỡ, chuẩn phong cách fantasy MMORPG.
3. [x] Mặt đất hiển thị đúng màu cỏ xanh, đường đất sỏi, đá cuội quảng trường, không còn bị ám màu xanh nước biển.
4. [x] Bản đồ có đồi núi bao bọc quanh làng tạo thành thung lũng sơn thủy hữu tình.
5. [x] Người chơi xuất phát tại Quảng trường Làng, tự do đi dạo ngắm nhìn phố xá, giếng nước, cối xay gió, quán trọ, lò rèn, xe rơm.
6. [x] Va chạm vật lý AABB chuẩn xác: Người chơi không đi xuyên tường nhà, tường giếng hay thân cây.
7. [x] Toàn bộ 37/37 unit test và Smoke test tiếp tục PASS 100%.
