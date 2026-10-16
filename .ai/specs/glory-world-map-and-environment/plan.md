# Kế Hoạch Kỹ Thuật (Technical Plan): Xây Dựng Bản Đồ Thế Giới Game Vinh Diệu (Glory World Map & Environment)

- **Mã tính năng:** `glory-world-map-and-environment`
- **Thư mục tài liệu:** `.ai/specs/glory-world-map-and-environment/`
- **Chế độ:** Auto-Pilot (PLANNER ➔ CODER ➔ TESTER ➔ REVIEWER ➔ SUMMARY)
- **Mục tiêu:**
  Xây dựng bản đồ hoàn chỉnh theo phong cách thế giới MMORPG *Vinh Diệu* (*Toàn Chức Cao Thủ* / *The King's Avatar*):
  1. Phân chia rõ ràng 3 khu vực thế giới:
     - **Trong Thành (Sanctuary Citadel / Safe Zone):** Phố xá thời trung cổ, nhà cửa mái ngói vách gỗ, quảng trường lát đá, đài phun nước trung tâm, cột đèn lồng ma thuật, cổng thành đá và tường thành kiên cố.
     - **Ngoài Thành (Outer Wilds / Forest Frontier):** Đồng cỏ xanh tươi, đường mòn sỏi đá, hàng rào gỗ, cây sồi cổ thụ, thông tuyết, tảng đá tự nhiên, bãi tập luyện.
     - **Khu Vực Nguy Hiểm (Ruins of Desolation / Boss Lair):** Phế tích cổ đổ nát, cột đá gãy đổ, đài đuốc lửa chiến trận bốc cháy ngùn ngụt, chông gai phòng thủ, sào huyệt Boss hắc ám.
  2. Tạo bộ sưu tập model 3D glTF 2.0 đầy đủ chi tiết với bảng màu PBR:
     - Nhà cửa trung cổ (`house_medieval.gltf`)
     - Cổng thành & tường luỹ đá (`city_wall_gate.gltf`)
     - Cột đèn đường thắp sáng (`lantern_post.gltf`)
     - Đài phun nước quảng trường (`stone_fountain.gltf`)
     - Cây sồi tán rộng (`tree_oak.gltf`) & Cây thông gai (`tree_pine.gltf`)
     - Tảng đá thiên nhiên (`rock_boulder.gltf`)
     - Hàng rào gỗ dã ngoại (`fence_wood.gltf`)
     - Cột phế tích cổ (`ruin_pillar.gltf`)
     - Đài đuốc lửa rực cháy (`brazier_fire.gltf`)
     - Thùng gỗ / hòm tiếp tế (`crate_wooden.gltf`)
  3. Mở rộng mặt đất lên 240m x 240m với texture đa phân vùng (`world_ground.png`): Lát đá quảng trường thành phố, đường đất sỏi dẫn lối qua cổng thành, đồng cỏ dã ngoại và nền đá cháy của phế tích nguy hiểm.
  4. Tích hợp sương mù khoảng cách (Distance Fog) trong PBR Shader: Hòa quyện đường chân trời mượt mà vào bầu trời, xóa sổ vĩnh viễn cảnh tượng "đen thui".
  5. Nạp cấu hình vật thể từ `world_map.json` và đăng ký vật cản AABB vào `CollisionWorld` để nhân vật va chạm vật lý chân thực, không đi xuyên tường hay nhà cửa.

---

## 1. Thiết Kế Bản Đồ & Bố Cục Không Gian

```
                              [PHƯƠNG BẮC - NORTH]
                     +---------------------------------------+
                     |    KHU VỰC NGUY HIỂM / PHẾ TÍCH CỔ    |
                     |  (Ruins of Desolation - Z: -30 to -80)|
                     |  - Cột đá phế tích, đài đuốc lửa      |
                     |  - Rào chắn cọc gai, đất cháy         |
                     |  - Sào huyệt Boss Gorgon              |
                     +---------------------------------------+
                                         ▲
                                 [Hàng rào chông]
                                         ▲
                     +---------------------------------------+
                     |       NGOÀI THÀNH / VÙNG DÃ NGOẠI     |
                     |   (Outer Wilds - Z: -30 to +15)       |
                     |  - Đường mòn sỏi đá, hàng rào gỗ      |
                     |  - Rừng cây sồi, cây thông, tảng đá   |
                     |  - Đèn dẫn đường, bãi bù nhìn gỗ      |
                     +---------------------------------------+
                                         ▲
                           [CỔNG THÀNH ĐỒ SỘ - Z: +15]
                                         ▼
                     +---------------------------------------+
                     |       TRONG THÀNH (SAFE ZONE)         |
                     |   (Sanctuary Citadel - Z: +15 to +80) |
                     |  - Quảng trường đá, đài phun nước     |
                     |  - Nhà cửa trung cổ, quán trọ, thùng  |
                     |  - Cột đèn lồng thắp sáng ấm áp       |
                     |  - Tường thành bao bọc an toàn        |
                     +---------------------------------------+
                              [PHƯƠNG NAM - SOUTH]
```

---

## 2. Danh Sách Tệp Tin Triển Khai (File Inventory)

### 2.1. Assets & Tools
- `tools/generate_environment_models.py`: Kịch bản Python tự động tạo texture palette `environment_palette.png`, `world_ground.png` và 10 model 3D glTF 2.0 (mesh, uv, materials).
- `assets/textures/environment_palette.png`: Bảng màu PBR (đá thành cổ, gỗ mộc, ngói nung, kính đèn phát sáng, lá cây, đồng cổ).
- `assets/textures/world_ground.png`: Bản đồ kết cấu mặt đất 512x512 phân vùng (đá phiến thành phố, sỏi đường mòn, cỏ xanh dã ngoại, đá đen phế tích).
- `assets/data/world_map.json`: Tọa độ, góc xoay và tỉ lệ cho từng công trình, cây cối, đèn đóm trong toàn bộ bản đồ.

### 2.2. Engine & Shader
- `engine/src/ground_plane.cpp`: Mở rộng kích thước từ `[-60, +60]` lên `[-120, +120]` (240m x 240m) và điều chỉnh UV tiling phù hợp.
- `engine/src/model_renderer.cpp` & `engine/include/glory/engine/model_renderer.hpp`:
  - Nạp `world_ground.png` thay thế cho `arena_floor.png`.
  - Tích hợp công thức Distance Fog vào Fragment Shader (hòa trộn màu vật thể xa > 45m với màu chân trời của Skybox).
  - Thêm phương thức nạp và vẽ các static environment props.

### 2.3. Gameplay & Collision
- `game/src/main.cpp`:
  - Đọc `world_map.json`, tự động sinh các hộp va chạm `Aabb` tương ứng các tòa nhà, tường thành, cổng thành, cột đá và nạp vào `CollisionWorld`.
  - Đặt vị trí xuất phát của Player ở trong thành (Quảng trường trung tâm `[0, 0, 40]`, quay mặt hướng ra cổng thành phương Bắc).
  - Đặt vị trí Boss ở trung tâm Phế tích nguy hiểm (`[0, 0, -50]`).
  - Thêm cơ chế nhận diện khu vực: Hiển thị thông báo trên màn hình khi người chơi bước qua Cổng thành ra Dã ngoại, hoặc bước vào Phế tích Nguy hiểm.

---

## 3. Tiêu Chí Nghiệm Thu (Acceptance Criteria)

1. [x] Toàn bộ 10 model 3D môi trường (nhà, cổng thành, đèn, cây, đài phun, đuốc, cột, đá, rào) được tạo thành công dạng glTF 2.0 chuẩn.
2. [x] Mặt đất mở rộng 240m với texture đa vùng chi tiết.
3. [x] Không còn hiện tượng mép map bị cắt cụt đen thui (nhờ Distance Fog hòa trộn vào bầu trời).
4. [x] Bản đồ có đầy đủ 3 phân khu Trong Thành, Ngoài Thành và Phế Tích Nguy Hiểm.
5. [x] Nhân vật xuất phát trong thành, có thể đi bộ xuyên qua cổng thành ra khu dã ngoại và tiến vào sào huyệt Boss.
6. [x] Hệ thống va chạm vật lý hoạt động chuẩn: Người chơi không đi xuyên tường nhà hay tường thành.
7. [x] Toàn bộ 37/37 unit test và Smoke test tiếp tục PASS 100%.
