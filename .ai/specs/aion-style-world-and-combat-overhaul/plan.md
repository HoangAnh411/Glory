# Kế Hoạch Kỹ Thuật (Technical Plan): Nâng Cấp Đồ Họa Kiểu Aion 2, Môi Trường Thế Giới & Hệ Thống Động Tác Kỹ Năng Đa Dạng

- **Mã tính năng:** `aion-style-world-and-combat-overhaul`
- **Thư mục tài liệu:** `.ai/specs/aion-style-world-and-combat-overhaul/`
- **Vai trò hiện tại:** **PLANNER (Dừng lại ở bước này theo yêu cầu của người dùng để phê duyệt)**
- **Mục tiêu:**
  1. Nâng cấp đồ họa engine lên chuẩn High-Fantasy MMORPG phong cách **Aion 2** (HDR Bloom phát sáng, Tone Mapping điện ảnh, sương mù khí quyển, vệt chém kiếm rực rỡ).
  2. Xóa bỏ hoàn toàn "thế giới đen thui" bằng việc xây dựng **Đấu trường Thánh Địa 3D (Sanctuary Colosseum)** với tường thành, cột đá La Mã, đài lửa và chân trời hùng vĩ.
  3. Xây dựng bộ **Animation chuyên biệt cho từng kỹ năng** (chém thường, chém nặng bổ đất, xoay lốc xoáy 360 độ, húc khiên choáng, lộn nhào né đòn).
  4. Sửa lỗi **Boss đứng im vô tri**: Tự động kích hoạt trận đánh khi lại gần hoặc khi nhận sát thương, bổ sung hành vi truy đuổi, dậm đất và lao húc sống động.

---

## 1. Phân Tích Thực Trạng Hiện Tại (Root Cause Analysis)

### 1.1. Vì sao không gian xung quanh "đen thui"?
- File `engine/src/ground_plane.cpp` chỉ vẽ duy nhất một mặt phẳng vuông nhỏ `[-60, +60]`.
- Ngoài phạm vi 60m không có bất kỳ thực thể 3D nào (không tường, không cột, không địa hình phụ).
- Chưa có hệ thống sương mù khoảng cách (Distance Fog) để hòa trộn biên mặt đất vào bầu trời, khiến mép sàn đấu bị cắt đứt gột rửa rơi thẳng vào nền đen vô tận của OpenGL Clear Color.

### 1.2. Vì sao Boss chỉ "đứng im một chỗ"?
- Trong `game/src/main.cpp` (dòng 1108 & 1143), biến `bossEncounterActive` chỉ được kích hoạt nếu nhân vật bước vào ô trigger `boss_aggro_gate` nằm tít phía sau tại `[0.0, 0.0, -6.0]`.
- Người chơi xuất phát ở `[0, 0, 0]` và đánh thẳng vào Boss ở `[-3.8, 0, 0]` thì trigger này **không bao giờ được chạm tới**! Do đó `boss.update(...)` không bao giờ được gọi và Boss hóa thành bù nhìn vô tri.
- Boss chưa được tích hợp skeletal animation riêng nên chỉ hiển thị ở tư thế tĩnh.

### 1.3. Vì sao các kỹ năng đều có động tác giống hệt nhau?
- Trong `tools/generate_hero_warrior.py`, model chỉ có đúng 3 clip hoạt ảnh: `Idle`, `Run`, và `Attack`.
- Trong `assets/data/skills.json`, tất cả các chiêu (`slash`, `heavy_strike`, `shield_bash`, `combo_1`, `combo_2`, `combo_3`,...) đều đang gán cứng `"animationState": "Attack"`. Người chơi bấm phím nào cũng chỉ thấy vung tay phải chém một nhát y hệt nhau.

---

## 2. Giải Pháp Chi Tiết Theo Từng Trụ Cột

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       AION 2 STYLE GRAPHICS & WORLD                         │
├─────────────────────┬───────────────────────┬───────────────────────────────┤
│   1. ĐỒ HỌA & PBR   │   2. MAP ĐẤU TRƯỜNG   │   3. ANIMATION & BOSS COMBAT  │
│ - HDR Framebuffer   │ - Colosseum Stone Wall│ - Multi-Skill Animations      │
│ - Multi-Pass Bloom  │ - Corinthian Pillars  │   (Slash, Slam, Spin, Bash)   │
│ - ACES Tonemapping  │ - Fire Braziers (FX)  │ - Weapon Trail Ribbons (Vệt)  │
│ - Atmospheric Fog   │ - Horizon Mountains   │ - Auto-Aggro Boss AI          │
│ - Soft Shadows PCF  │ - Skybox blending     │ - Boss Attack & Movement      │
└─────────────────────┴───────────────────────┴───────────────────────────────┘
```

### Trụ Cột 1: Nâng Cấp Đồ Họa Như Aion 2 (Graphics & Post-Processing)
Đặc trưng hình ảnh của Aion 2 (Unreal Engine 5) là ánh sáng thần tiên rực rỡ, kim loại phản quang lấp lánh và các hiệu ứng phát quang (Emissive Bloom) cực kỳ bắt mắt:
1. **HDR Off-screen Render Pipeline**:
   - Thay vì vẽ thẳng ra LDR Backbuffer, engine vẽ toàn bộ thế giới vào một Framebuffer nổi 16-bit (`GL_RGBA16F`).
2. **Post-Processing Pass: Multi-Pass Bloom**:
   - Trích xuất các vùng phát sáng vượt ngưỡng (`threshold > 1.0`): Lưỡi kiếm ma thuật, mắt boss rực lửa dung nham, hoa văn giáp trụ, lửa đuốc.
   - Thực hiện Gaussian Blur 2 pass (ngang/dọc) và hòa trộn cộng dồn (Additive Blending) lại cảnh gốc. Điều này mang lại hiệu ứng ánh hào quang tỏa sáng thần thánh đặc trưng của game Aion.
3. **ACES Filmic Tone Mapping**:
   - Nén dải sáng HDR về dải hiển thị chuẩn của màn hình, giữ được độ tương phản sâu ở vùng tối mà không bị cháy trắng ở vùng phát sáng.
4. **Exponential Distance & Height Fog**:
   - Thêm sương mù khoảng cách vào PBR Shader: Những vật thể ở xa hơn 35m sẽ từ từ hòa quyện vào tông màu hoàng hôn ấm áp của Skybox, xóa sổ vĩnh viễn cảm giác "đen thui".
5. **Weapon Slash Ribbon Trails**:
   - Sinh dải đa giác mỏng dọc theo lưỡi kiếm khi nhân vật tung chiêu để tạo vệt kiếm ánh sáng xé gió (Slash Arc) chuẩn MMORPG hành động.

### Trụ Cột 2: Xây Dựng Bản Đồ "Đấu Trường Thánh Địa" (Sanctuary Colosseum Map)
Xóa bỏ tấm sàn phẳng trơ trọi 60x60m:
1. **Vòng Tường Đá Cổ Đại (Circular Colosseum Wall)**:
   - Dựng một vòng cung tường đá La Mã cao 7m, bao quanh võ đài bán kính 30m. Tường có các cột trụ gờ nổi và hoa văn sa thạch cổ.
2. **Cột Đá Thần Thoại (Ancient Pillars)**:
   - Bố trí 8 cột đá Corinthian uy nghiêm quanh rìa đấu trường với đỉnh cột chạm khắc hoa văn.
3. **Đài Đuốc Lửa Thánh (Grand Braziers)**:
   - Đặt 4 đài lửa bằng đồng cổ ở 4 góc đấu trường với hiệu ứng hạt lửa bập bùng và khói bốc lên cao.
4. **Hậu Cảnh Kỳ Vĩ (Floating Sanctuary Background)**:
   - Thêm các khối đảo đá lơ lửng và dải núi xa ở đường chân trời trong skybox, mô phỏng đúng bối cảnh thế giới thần thoại của Aion.

### Trụ Cột 3: Hệ Thống Animation Từng Kỹ Năng Riêng Biệt (Unique Skill Animations)
Nâng cấp bộ xương nhân vật (Rigging) và sinh các chuyển động chuyên biệt:
1. **`Skill_Slash` (Đòn đánh thường - LMB)**:
   - Nhát 1: Chém lướt ngang từ phải sang trái.
   - Nhát 2: Chém hất ngược từ dưới lên trên.
2. **`Skill_Heavy` (Kỹ năng [1] - Trảm Kích Bổ Đất)**:
   - Nhân vật lấy đà nhảy vọt lên không, hai tay nắm chặt chuôi kiếm bổ thẳng xuống đất tạo sóng chấn động.
3. **`Skill_Whirlwind` (Kỹ năng [2] - Phong Ma Toàn Vũ)**:
   - Thân mình xoay tròn 360 độ 2 vòng liên tiếp, lưỡi kiếm quét ngang tạo thành vòng tròn ánh sáng bảo vệ và sát thương toàn bộ kẻ địch xung quanh.
4. **`Skill_ShieldBash` (Kỹ năng [3] - Hộ Vệ & Húc Khiên)**:
   - Dậm chân trụ vững, giơ khiên chắn trước ngực sau đó lao mạnh về phía trước húc văng đối thủ gây choáng.
5. **`Skill_Dodge` (Phím Shift - Lộn Nhào / Lướt Gió)**:
   - Nhân vật hạ thấp trọng tâm, thực hiện cú lộn nhào nhanh kèm vệt mờ né tránh mọi đòn tấn công.
6. **Cập nhật `skills.json`**:
   - Ánh xạ chính xác từng skill ID vào đúng `animationState` thay vì dùng chung chữ "Attack".

### Trụ Cột 4: Boss Gorgon Thức Tỉnh & Tác Chiến Sống Động
1. **Cơ chế Auto-Aggro thông minh**:
   - Bỏ phụ thuộc vào trigger box lùi; Boss tự động kích hoạt chế độ chiến đấu khi:
     - Người chơi bước vào phạm vi 12 mét.
     - HOẶC Boss trúng bất kỳ đòn đánh nào của người chơi.
2. **Bộ chuyển động riêng cho Boss Gorgon**:
   - `Boss_Idle`: Thở dốc, búa tạ đặt nghiêng trên vai.
   - `Boss_Walk`: Bước đi nặng nề, mặt đất rung nhẹ khi tiếp đất.
   - `Boss_HammerSlam`: Giơ búa chiến lên cao bằng 2 tay và giáng sấm sét xuống mặt đất.
   - `Boss_Charge`: Cúi thấp người gầm lên và lao thẳng về phía người chơi.
   - `Boss_EnrageRoar`: Gầm rú nộ khí chuyển sang Phase 2 khi máu dưới 50%.

---

## 3. Các Tệp Tin Sẽ Được Tạo / Chỉnh Sửa Trong Giai Đoạn Tiếp Theo

| Nhóm | Tệp Tin | Mô Tả Thay Đổi Dự Kiến |
|:---|:---|:---|
| **Graphics & PostFX** | `engine/include/glory/engine/post_process.hpp`<br>`engine/src/post_process.cpp` | Tạo HDR Framebuffer, Bloom Kawase pass, ACES Tonemapping shader. |
| **PBR Shader** | `engine/src/model_renderer.cpp` | Bổ sung Emissive sampling, Distance Height Fog, Soft Shadow PCF. |
| **Map Architecture** | `tools/generate_colosseum_arena.py`<br>`assets/models/arena_colosseum.gltf` | Sinh model 3D đấu trường La Mã: tường thành, cột đá, đài đuốc và nạp vào game. |
| **Hero Rigging & Anim** | `tools/generate_hero_warrior.py` | Bổ sung 5 clip animation: Slash combo, Heavy leap slam, Whirlwind spin, Shield bash, Dodge roll. |
| **Boss Rigging & Anim** | `tools/generate_boss_model.py` | Bổ sung skeletal rig và animation clips cho Boss Gorgon. |
| **Skill Data Mapping** | `assets/data/skills.json` | Cập nhật `animationState` chuẩn xác cho từng skill. |
| **Game Loop & Aggro** | `game/src/main.cpp`<br>`game/src/boss.cpp` | Sửa logic tự động kích hoạt Boss Aggro khi bị đánh hoặc lại gần. |

---

## 4. Lộ Trình Triển Khai (Dự Kiến Khi Được Phê Duyệt)

- **Bước 1 (World & Map):** Dựng Đấu Trường Colosseum 3D + Distance Fog ➔ Giải quyết triệt để cảnh tượng "đen thui", mang lại không gian hùng vĩ.
- **Bước 2 (Post-FX & Aion Visuals):** Bổ sung HDR Framebuffer, Bloom phát sáng cho vũ khí và chiêu thức, ACES Tonemapping ➔ Đạt chuẩn đồ họa lung linh như Aion 2.
- **Bước 3 (Skill Animations):** Nạp bộ animation riêng cho từng skill của Hero + vệt chém kiếm ➔ Bấm chiêu nào tung động tác đẹp mắt tương ứng chiêu đó.
- **Bước 4 (Boss Awakening):** Bật Auto-Aggro, tích hợp bước chạy và chiêu đập búa cho Boss ➔ Trận chiến sống động, gay cấn và thử thách.

---

> [!NOTE]
> Kế hoạch này hiện **đang dừng lại ở bước PLANNER**. Chưa có dòng code thực thi nào được viết theo đúng chỉ dẫn của người dùng. Hãy xem xét các nội dung trên và cho biết ý kiến của bạn!
