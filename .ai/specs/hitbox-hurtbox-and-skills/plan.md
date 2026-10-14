# Kế hoạch: Hitbox, Hurtbox và Skill Data-Driven

## 1. Mục tiêu

Triển khai skill registry nạp JSON, combat resolution chạy CPU độc lập trong `glory_game`, hitbox gắn theo vị trí/hướng/animation state của người chơi, hurtbox capsule, cooldown/cast-time, chống multi-hit trong một lần thi triển, và practice dummy có thể hồi máu. Application chỉ chuyển input, snapshot và event hình ảnh/âm thanh giữa game logic với engine.

## 2. Thiết kế và tasks

1. Thêm `nlohmann/json` header-only qua CMake FetchContent; `glory_game` được link với target JSON nhưng không link SDL/OpenGL.
2. Tạo `combat.hpp/.cpp` với `SkillDefinition`, `SkillRegistry`, `Hurtbox`, `Hitbox`, `Combatant`, `CombatHitEvent`, và `CombatSystem`. Loader kiểm tra kiểu dữ liệu, giá trị hữu hạn/phạm vi, trường bắt buộc và báo lỗi có ngữ cảnh.
3. Dùng hurtbox capsule theo trục Y và hitbox hình cầu. Tính tâm hitbox bằng offset local X/Y/Z được xoay quanh Y theo `facingYaw`; chỉ xét hit trong animation state và active time window của skill. Mỗi activation lưu tập entity ID đã trúng để chặn multi-hit.
4. Skill activation kiểm tra cast time và cooldown; `PrimaryAttack` yêu cầu skill `slash`. Đưa HP, số hit, skill cooldowns, hit event và yêu cầu reset dummy qua snapshot/callback; gameplay resolution vẫn do `glory_game` sở hữu.
5. Tạo `assets/data/skills.json` với `slash` và `heavy_strike`. Practice dummy có ID ổn định, hurtbox capsule và HP 100; hit event cho engine biết vị trí để phát particle burst và `hit.wav`.
6. Mở rộng DebugPanel để hiển thị skills/cooldowns, HP và hit count của dummy, cùng nút yêu cầu hồi máu; application chuyển yêu cầu này về game callback và xử lý hit events bằng renderer/audio.
7. Thêm CPU test cho JSON, giao cắt/trượt, active window, cooldown/cast-time và multi-hit; đăng ký CTest.
8. Chạy build, toàn bộ CTest và 120-tick smoke-test; ghi output thực tế vào `test-report.md`. Reviewer đối chiếu plan, kiến trúc và roadmap; xử lý mọi mục Phải sửa, sau đó cập nhật đúng hai checkbox PLAN.md.

## 3. Files to touch

- Tạo `game/include/glory/game/combat.hpp`, `game/src/combat.cpp`.
- Tạo `assets/data/skills.json`.
- Sửa `CMakeLists.txt` để khai báo JSON, thêm source và test `glory_combat`.
- Sửa `game/src/main.cpp` để nạp skill data và điều phối combat trong fixed update.
- Sửa `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp` để mở rộng snapshot và chuyển reset request/hit event.
- Sửa `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp` để hiển thị skill/dummy state và reset control.
- Tạo `tests/combat_tests.cpp`.
- Tạo `.ai/specs/hitbox-hurtbox-and-skills/test-report.md` và `review.md`.
- Cập nhật hai mục liên quan trong `PLAN.md` sau khi build/test PASS và review không còn mục bắt buộc.

## 4. Rủi ro và giảm thiểu

- JSON sai hoặc thiếu trường có thể làm app crash lúc khởi tạo: validate đầy đủ, ném `runtime_error` có đường dẫn/trường; test file thật và dữ liệu không hợp lệ.
- Capsule/sphere intersection có thể sai khi mục tiêu ở cao hoặc lệch trục: tính khoảng cách bình phương từ tâm sphere đến đoạn thẳng capsule, kiểm thử biên trong/ngoài.
- Update frame không đều có thể bỏ lỡ active window: combat chạy theo fixed timestep; kiểm tra cửa sổ theo thời điểm activation và elapsed time.
- Thay đổi engine snapshot có thể phá smoke-test: thêm trường mặc định tương thích và chạy build/CTest/smoke sau tích hợp.
- Hit event cần hiệu ứng nhưng combat không được gọi renderer/audio: phát event có vị trí để application tiêu thụ.
- Các thay đổi sẵn có ở working tree là ngoài phạm vi; chỉ chỉnh file thuộc feature và hai checkbox được yêu cầu.

## 5. Tiêu chí nghiệm thu

- Hai skill mẫu được đọc từ JSON với damage/cooldown/cast-time và hitbox data đúng; lỗi JSON hoặc skill không hợp lệ được báo rõ.
- Hitbox/hurtbox kiểm tra giao cắt đúng, chỉ gây damage trong active window và đúng animation state, cooldown/cast-time được tôn trọng.
- Mỗi target chỉ bị tính damage một lần trong một activation; activation tiếp theo được gây damage lại.
- Dummy bắt đầu 100 HP, nhận đúng damage, đếm số lần bị đánh trúng, và có thể reset.
- Hit event cung cấp điểm va chạm; engine phát particle burst và `hit.wav`, không đưa SDL/OpenGL vào `glory_game`.
- DebugPanel hiển thị danh sách skill/cooldown, HP/hit count, và nút hồi máu.
- Build thành công, mọi CTest cũ và mới PASS, smoke-test kết thúc sau 120 fixed ticks; có test report và review ba nhóm.
- Chỉ tick hai checkbox mục tiêu trong PLAN.md sau khi đủ các điều kiện trên.
