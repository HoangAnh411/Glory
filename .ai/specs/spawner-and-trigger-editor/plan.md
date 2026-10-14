# World Spawner and Trigger Editor Plan

## 1. Mục tiêu

Thêm dữ liệu spawn/trigger JSON, logic quản lý thuần C++ trong `glory_game`, và cửa sổ ImGui trong `glory_engine` để chỉnh vị trí/quay, vùng trigger, snap spawn theo player, save và hot-reload. Trigger phát trạng thái Enter/Inside/Exit; client tiêu thụ sự kiện để kích hoạt boss và HealArea.

## 2. Tasks

1. Tạo `WorldEncounter`, `SpawnPoint`, `TriggerZone` và enum entity/shape/type/transition trong game. Cung cấp load/save, validate dữ liệu hữu hạn/kích thước/id duy nhất/tham chiếu, truy vấn spawn, cập nhật trạng thái trigger và danh sách vùng đang hoạt động.
2. Tạo `assets/data/world_spawns.json` với spawn Player/PracticeDummy/Boss cùng Box/Sphere zones. Ghi an toàn bằng file tạm và thay thế nguyên tử.
3. Bổ sung DTO world editor, callbacks save/reload và trigger-state view cho engine API. Thêm cửa sổ `World & Spawner Editor`, chọn/sửa spawn và zone, đổi enum, thêm/xóa trigger zone, Snap to Player, Save/Reload, trạng thái hoạt động.
4. Nối client: nạp spawn ban đầu; áp dụng player/boss spawn, giữ tọa độ PracticeDummy trong encounter data; cập nhật trigger mỗi fixed tick; BossAggro kích hoạt boss, HealArea hồi player; phản ánh trigger active vào DebugPanel. Reload dữ liệu hợp lệ áp dụng spawn mới, dữ liệu lỗi giữ nguyên state hiện tại.
5. Thêm unit test CTest cho JSON round-trip, validate shape/dữ liệu lỗi, enter/inside/exit cho Box/Sphere, và áp dụng lại spawn transform.
6. Build, chạy CTest và smoke test client; ghi `test-report.md`, review độc lập và xử lý mục Phải sửa; chỉ sau PASS tick mục `Editor đặt quái, spawn point, trigger` trong PLAN.md.

## 3. Files cần sửa / tạo

- `game/include/glory/game/world_encounter.hpp` (mới)
- `game/src/world_encounter.cpp` (mới)
- `game/include/glory/game/character_controller.hpp`
- `game/src/character_controller.cpp`
- `game/src/main.cpp`
- `assets/data/world_spawns.json` (mới)
- `engine/include/glory/engine/application.hpp`
- `engine/include/glory/engine/debug_panel.hpp`
- `engine/src/debug_panel.cpp`
- `engine/src/application.cpp`
- `tests/world_encounter_tests.cpp` (mới)
- `CMakeLists.txt`
- `.ai/specs/spawner-and-trigger-editor/test-report.md` (Tester)
- `.ai/specs/spawner-and-trigger-editor/review.md` (Reviewer)
- `PLAN.md` (chỉ checkbox của mục này sau PASS)

## 4. Rủi ro & Giảm thiểu

- Không tạo phụ thuộc game -> engine: DTO và callbacks đặt ở engine; chuyển đổi/logic chỉ ở client.
- JSON lỗi có thể làm hỏng encounter đang chạy: parse và validate dữ liệu ứng viên trước khi thay; callback commit chỉ sau khi tạo được bản chuyển đổi.
- Teleport có thể giữ vận tốc/dodge cũ: API teleport reset transient movement state và đặt vị trí/yaw trực tiếp.
- Trigger Inside lặp mỗi tick có thể lặp tác dụng: client chỉ kích hoạt hành động một lần ở Enter; Inside dùng để hiển thị trạng thái.
- Project hiện có Boss là combat target chính, chưa có actor practice dummy độc lập; spawn PracticeDummy vẫn được lưu, chọn và hiển thị vị trí qua logic world, không tạo renderer/AI/combat actor mới trong bước editor này.
- Game loop render chạy sau fixed update: editor commands áp dụng sau frame và có hiệu lực ở tick kế tiếp; reload lỗi không commit.

## 5. Tiêu chí chấp nhận

- JSON có các spawn Player, PracticeDummy, Boss và trigger Box/Sphere với đúng trường đã yêu cầu.
- Load/save round-trip giữ loại entity, vị trí/yaw và kiểu/kích thước trigger; từ chối id trùng, vector không hữu hạn, kích thước không dương, shape/type không hợp lệ.
- Logic giao cắt Box/Sphere chính xác và phát Enter, Inside, Exit theo từng lần cập nhật; trạng thái active hiện được trong editor.
- Client lấy Player/Boss từ spawn point; BossAggro kích hoạt encounter, HealArea hồi máu khi Enter; Snap to Player, Save và Reload hoạt động; reload lỗi giữ nguyên cấu hình/state runtime.
- Teleport/áp dụng spawn reset transient movement state và đặt lại vị trí/yaw.
- Unit test mới được đăng ký CTest; build, toàn bộ CTest và smoke test client PASS.
- Không include SDL/OpenGL trong `glory_game`; không thêm game engine; giải thích tiếng Việt, identifier/comment kỹ thuật bằng tiếng Anh.
