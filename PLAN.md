# Dự án: Game 3D hành động kiểu MMORPG, tự viết engine

## Hướng dẫn cho AI (đọc kỹ trước khi làm)

Bạn là lập trình viên hỗ trợ tôi xây dựng dự án này. Hãy làm theo các quy tắc sau:

1. Đọc toàn bộ file này trước. Mỗi lần làm việc, tìm **việc đầu tiên chưa tick `[x]`** trong danh sách bên dưới và chỉ làm việc đó (hoặc nhóm nhỏ liền kề), không nhảy cách giai đoạn.
2. Làm xong thì đổi `[ ]` thành `[x]` trong file này và tóm tắt ngắn: đã làm gì, file nào thay đổi, cách chạy thử.
3. Mỗi bước phải **build và chạy được**. Không viết một lượng lớn code chưa kiểm chứng.
4. Giải thích ngắn gọn bằng tiếng Việt; đặt tên biến, hàm, comment kỹ thuật bằng tiếng Anh.
5. Nếu yêu cầu chưa rõ hoặc có nhiều hướng thiết kế, hãy hỏi tôi một câu trước khi viết code.
6. Tách logic game khỏi phần render ngay từ đầu, vì sau này server phải chạy logic mà không cần đồ họa.
7. Dùng thư viện cho phần phụ trợ, nhưng **các hệ thống lõi tự viết**: renderer, animation, combat, netcode.
8. Không dùng game engine (Unity, Unreal, Godot...). Không dùng tên hay nhân vật của game Vinh Diệu.

## Bối cảnh

- Mục tiêu: game 3D hành động, chiến đấu thao tác thủ công (không khóa mục tiêu), có combo, hủy chiêu, nhiều class, đấu trường PvP, sau cùng là thế giới online (MMO).
- Chiến lược: chia thành 3 tầng, mỗi tầng đều chơi được: (1) single-player, (2) đấu trường online 1v1 / 3v3, (3) MMO đầy đủ. Có thể dừng ở tầng 2.
- Người làm: một mình, bán thời gian.

## Công nghệ (giả định, có thể đổi)

- Ngôn ngữ: C++ (chuẩn C++20), build bằng CMake
- Cửa sổ và input: SDL3
- Đồ họa: OpenGL 4.5 (có thể chuyển Vulkan sau)
- Thư viện: GLM (toán), cgltf (nạp glTF), stb_image (ảnh), Dear ImGui (debug/editor), EnTT (ECS), miniaudio (âm thanh), ENet (mạng), SQLite hoặc PostgreSQL (dữ liệu)
- Dữ liệu skill, quái, vật phẩm: JSON

## Kiến trúc module

```
Platform (window, input, thời gian)
  ├─ Renderer (mesh, shader, shadow, skinning, particle, post-FX)
  ├─ Asset pipeline (glTF -> định dạng nội bộ)
  ├─ ECS / Scene
  ├─ Physics & Collision (capsule, hitbox/hurtbox, raycast)
  ├─ Animation (state machine, blending, root motion, animation events)
  ├─ Combat (skill data-driven, combo, cooldown, buff/debuff, hit-stop)
  ├─ AI (behavior tree)
  ├─ UI (HUD, inventory, skill bar)
  ├─ Audio
  └─ Network (client/server, snapshot, prediction)
```

## Danh sách việc cần làm

### Giai đoạn 0: Thiết kế (2-4 tuần)
- [x] Chốt công nghệ: C++, SDL3, OpenGL 4.5, GLM, Dear ImGui
- [x] Cài môi trường: compiler, CMake, Git, RenderDoc
- [x] Tạo repo Git và cấu trúc thư mục (engine, game, server, assets)
- [x] Viết GDD 1-2 trang: bối cảnh, vòng lặp chơi, điểm khác biệt
- [x] Chọn class đầu tiên và liệt kê 5 skill
- [x] Chốt phạm vi vertical slice: 1 map, 1 class, 1 boss, 1 đấu trường
- [x] Đặt tên và thế giới riêng, không dùng tên hay nhân vật của Vinh Diệu

### Giai đoạn 1: Nền tảng (1-2 tháng)
- [x] Mở cửa sổ SDL3 với OpenGL context
- [x] Viết game loop với fixed timestep
- [x] Xử lý input bàn phím và chuột, cho phép đổi phím
- [x] Vẽ tam giác đầu tiên bằng shader
- [x] Thêm GLM và camera tự do
- [x] Nạp texture bằng stb_image và vẽ mesh có texture
- [x] Nạp model glTF bằng cgltf
- [x] Thêm ánh sáng Blinn-Phong
- [x] Gắn Dear ImGui làm panel debug (FPS, thông số)
- [x] **Cột mốc:** nhân vật 3D có ánh sáng, xoay camera quanh được

### Giai đoạn 2: Renderer và animation (2-3 tháng)
- [x] Skeletal animation: skinning trên GPU
- [x] Blend giữa các animation (đứng, chạy, đánh)
- [x] Animation event: đánh dấu khung gây sát thương, phát âm thanh
- [x] Shadow mapping
- [x] PBR cơ bản và skybox
- [x] Particle system cho hiệu ứng skill
- [x] Tích hợp âm thanh bằng miniaudio
- [x] **Cột mốc:** nhân vật chạy, đánh, có bóng đổ và hiệu ứng

### Nâng cấp đồ họa: Giai đoạn A (Camera, ánh sáng, bóng)
- [x] A1: Camera vai thứ ba 65 độ, tay đòn 5 m, lệch vai và spring arm chống vật cản
- [x] A2: HDR, ACES tonemapping, xử lý màu sRGB/linear và GPU timer query
- [ ] A3: Ánh sáng môi trường, cascaded shadows, fog và sky gradient
- [ ] A4: Anti-aliasing, preset chất lượng và đo hiệu năng theo pass

### Giai đoạn 3: Gameplay lõi, single-player (3-4 tháng)
- [x] Character controller: chạy, nhảy, camera third-person
- [x] Né đòn (dodge) với khung bất tử ngắn
- [x] Va chạm capsule với địa hình
- [x] Hitbox và hurtbox gắn theo animation
- [x] Skill data-driven: đọc từ JSON (sát thương, cooldown, cast time)
- [x] Combo và cửa sổ hủy chiêu (cancel window)
- [x] Hit-stop và knockback
- [x] Buff và debuff
- [x] AI quái bằng behavior tree
- [x] Boss đầu tiên với 2-3 pha
- [x] **Cột mốc:** đánh bại boss một mình, cảm giác combat đã tay

### Giai đoạn 4: Công cụ nội bộ (1-2 tháng, làm song song)
- [x] Editor skill và animation event bằng ImGui
- [x] Editor đặt quái, spawn point, trigger
- [x] Hot-reload dữ liệu để chỉnh cân bằng nhanh

### Giai đoạn 5: Netcode và đấu trường (3-5 tháng)
- [x] Tách logic game khỏi render để chạy server không cần đồ họa
- [x] Client và server qua UDP (ENet)
- [x] Server authoritative với tick 30-60 Hz
- [x] Snapshot và nội suy cho nhân vật khác
- [x] Client-side prediction và reconciliation
- [x] Lag compensation cho đòn đánh
- [x] Đấu 1v1, sau đó 3v3, matchmaking đơn giản
- [x] **Cột mốc:** 2 máy đánh nhau mượt với ping khoảng 100ms

### Giai đoạn 6: Lớp MMO (6-12 tháng trở lên)
- [x] Tài khoản, đăng nhập và database
- [x] Nhân vật, inventory, trang bị, rơi đồ
- [x] Chia zone và interest management
- [x] Chat, party, guild
- [x] Dungeon instance
- [x] Chợ giao dịch
- [x] Thêm class mới và cân bằng

### Giai đoạn 7: Hoàn thiện và vận hành
- [x] Chống gian lận: kiểm tra vị trí, tốc độ, cooldown ở server
- [x] Load test bằng bot
- [x] Logging và backup database
- [x] Closed beta và thu phản hồi

## Tài liệu tham khảo

- LearnOpenGL.com (đồ họa)
- Game Programming Patterns (Robert Nystrom)
- Game Engine Architecture (Jason Gregory)
- Real-Time Collision Detection (Christer Ericson)
- Gaffer On Games và Source Multiplayer Networking của Valve (netcode)
