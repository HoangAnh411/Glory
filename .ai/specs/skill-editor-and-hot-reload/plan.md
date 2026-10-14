# Skill Editor and Hot-Reload Plan

## 1. Mục tiêu

Thêm công cụ ImGui để biên tập năm skill hiện có, dữ liệu animation event và timeline; lưu JSON có validation; reload skill vào `CombatSystem` khi game đang chạy mà không reset nhân vật hoặc encounter. Phần registry/serialization thuộc `glory_game`, còn UI và DTO thuộc `glory_engine`.

## 2. Tasks

1. Hoàn thiện `SkillRegistry` với validate dùng chung, serialize định dạng chuẩn, lưu file an toàn qua file tạm và nạp lại từ file. Hỗ trợ các event có `time`, `type`, `name` trong skill JSON.
2. Thêm phương thức thay registry vào `CombatSystem`; chỉ thay registry/cooldown map sau khi registry mới hợp lệ và giữ nguyên cooldown còn tương ứng, active combat, vị trí, máu và state encounter.
3. Tạo DTO editor trong engine không phụ thuộc `glory_game`; thêm cửa sổ `Skill & Event Editor` trong DebugPanel với chọn skill, các trường gameplay/hitbox/effect/event, scrubber và các dấu active/cancel/event, cùng Save/Reload và F5.
4. Nối UI với game từ `game/src/main.cpp`: chuyển đổi DTO, ghi/nạp qua `SkillRegistry`, áp dụng registry mới cho combat và báo lỗi trong panel mà không làm crash.
5. Thêm unit tests cho round-trip serialize, validation (số âm, NaN, activeEnd < activeStart) và hot-reload an toàn; đăng ký CTest.
6. Configure/build, chạy CTest và smoke test client; ghi log thật vào `test-report.md`; review độc lập, xử lý mọi lỗi bắt buộc; sau cùng tick hai mục editor và hot-reload trong PLAN.md.

## 3. Files cần sửa / tạo

- `game/include/glory/game/combat.hpp`
- `game/src/combat.cpp`
- `engine/include/glory/engine/application.hpp`
- `engine/include/glory/engine/debug_panel.hpp`
- `engine/src/debug_panel.cpp`
- `engine/src/application.cpp`
- `game/src/main.cpp`
- `assets/data/skills.json`
- `tests/skill_registry_tests.cpp` (mới)
- `CMakeLists.txt`
- `.ai/specs/skill-editor-and-hot-reload/test-report.md` (vai Tester)
- `.ai/specs/skill-editor-and-hot-reload/review.md` (vai Reviewer)
- `PLAN.md` (chỉ hai mục yêu cầu, sau PASS)

## 4. Rủi ro & Giảm thiểu

- Engine/game phụ thuộc vòng: DTO và callback nằm ở engine API, adapter gọi game API chỉ trong client.
- JSON sai hoặc thiếu trường có thể làm hỏng runtime: parse và validate vào registry tạm trước commit; lỗi trả về UI, registry đang chạy không đổi.
- Ghi file có thể bị gián đoạn: serialize/validate trước, ghi file tạm cùng thư mục rồi thay thế file đích.
- Chỉnh skill đang chạy có thể vô hiệu hóa con trỏ: registry đổi bằng move sau khi parse; CombatSystem kết thúc tick hiện tại an toàn bằng cách chỉ lookup skill theo ID trong từng bước, reload UI xử lý ngay trước tick kế tiếp.
- Dear ImGui thao tác dữ liệu động: giữ lựa chọn theo ID, tái đồng bộ danh sách từ registry sau reload.
- Cửa sổ đồ họa không chạy được trong môi trường headless: kiểm chứng compile và smoke test theo khả năng môi trường; ghi rõ nếu backend đồ họa của máy không hỗ trợ smoke.

## 5. Tiêu chí chấp nhận

- UI chọn và chỉnh đủ `slash`, `heavy_strike`, `combo_1`, `combo_2`, `combo_3`, tên, damage, cooldown, cast time, animation state, combo, buffer/cancel, hit-stop, knockback, hitbox, status effects và animation events.
- Timeline hiển thị thời lượng, khoảng hitbox active, cancel marker và event marker; F5 và nút Reload nạp lại.
- JSON round-trip giữ toàn bộ trường skill/effect/event; file không hợp lệ hoặc metadata âm/NaN/thời gian hitbox sai bị từ chối.
- Reload hợp lệ cập nhật skill mới trong CombatSystem mà không thay đổi state người chơi/encounter; reload lỗi không làm thay đổi registry đang chạy.
- Test mới được đăng ký trong CTest; toàn bộ build, CTest và smoke test client PASS trước khi tick hai mục trong PLAN.md.
- Logic game không phụ thuộc render; không thêm game engine hoặc nội dung/tên từ Vinh Diệu; giải thích tiếng Việt, định danh và comment kỹ thuật bằng tiếng Anh.
