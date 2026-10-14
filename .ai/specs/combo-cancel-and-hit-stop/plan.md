# Kế hoạch: Combo, Cancel Window, Hit-stop và Knockback

## 1. Mục tiêu

Mở rộng `CombatSystem` thuần game logic để hỗ trợ combo ba nhịp có input buffer/window, dodge cancel chỉ trong recovery, hit-stop cho attacker/target/animation, và knockback của Dummy được giải quyết qua `CollisionWorld`. Không đưa API SDL/OpenGL vào `glory_game`.

## 2. Thiết kế và tasks

1. Mở rộng schema skill JSON với `comboNext`, `comboWindow`, `inputBuffer`, `cancelWindowStart`, `hitStopDuration`, `knockbackSpeed`, `knockbackDuration`; thêm `combo_1` → `combo_2` → `combo_3` với sát thương và hit-stop/knockback tăng dần. Giữ các skill thử nghiệm hiện có.
2. Bổ sung metadata validated vào `SkillDefinition`; duy trì combo step, combo timer/window, buffer timer, cancel-window availability, animation restart/cancel signals và hit-stop remaining trong `CombatSystem`.
3. Chuyển input attack theo state: attack khi rảnh khởi động `combo_1`; lệnh trong buffer/window được nối chuỗi; timeout reset step. Dodge trong active frames bị chặn khi đang đánh; dodge trong cancel window được CharacterController chấp nhận và CombatSystem hủy activation.
4. Khi hit xảy ra, kích hoạt hit-stop lấy từ skill, đóng băng tick điều khiển/animation và Dummy knockback. Sau hit-stop, giảm vận tốc knockback tuyến tính trong cấu hình thời lượng; resolve capsule qua `CollisionWorld` để xử lý tường/sàn.
5. Mở rộng snapshot DebugPanel hiển thị combo step/window, cancel availability, hit-stop state và vector vận tốc knockback Dummy.
6. Mở rộng `combat_tests.cpp` cho chuỗi 1→2→3, reset sau timeout, input buffer, dodge-cancel trước/sau active window, hit-stop duration và knockback hướng/giảm tốc/collision.
7. Build, chạy toàn bộ CTest và 120-tick smoke test; ghi kết quả thật. Review theo ba nhóm, xử lý mục Phải sửa, sau đó mới tick hai checkbox roadmap.

## 3. Files to touch

- Sửa `game/include/glory/game/combat.hpp`, `game/src/combat.cpp`.
- Sửa `game/include/glory/game/character_controller.hpp`, `game/src/character_controller.cpp` để cho phép combat khóa dodge trong active phase.
- Sửa `assets/data/skills.json` cho combo metadata và cấu hình 3 nhịp.
- Sửa `game/src/main.cpp` để điều phối trạng thái dodge/attack, đóng băng game updates theo hit-stop và truyền telemetry.
- Sửa `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp` để chuyển telemetry, cancel và freeze animation.
- Sửa `engine/src/debug_panel.cpp` để hiển thị combo/cancel/hit-stop/knockback.
- Sửa `tests/combat_tests.cpp`; target CTest `glory_combat` đã đăng ký trong `CMakeLists.txt`, nên không cần thay CMake trừ khi phát sinh file test riêng.
- Tạo `.ai/specs/combo-cancel-and-hit-stop/test-report.md`, `review.md`; cập nhật hai dòng trong `PLAN.md` sau PASS.

## 4. Rủi ro và giảm thiểu

- Input buffer/window lệch một fixed tick: định nghĩa elapsed rõ ràng, tính timer bằng fixed delta và kiểm thử sát biên mở/đóng.
- Dodge trước khi hết active frame có thể cancel nhầm: CharacterController nhận cờ `allowDodge` từ `CombatSystem`; chỉ active phase khóa dodge, recovery mở cancel.
- Hit-stop làm input hoặc physics tiến ngoài ý muốn: dừng controller và animation khi timer còn dương; CombatSystem chỉ giảm timer trong frozen ticks.
- Knockback xuyên vật cản hoặc mất grounding: cập nhật Capsule qua resolver sẵn có; test va chạm môi trường cùng deceleration.
- Combo damage/hit-stop/knockback sai skill: xác nhận metadata JSON và hiệu ứng theo skill trong unit tests.

## 5. Tiêu chí chấp nhận

- Ba đòn combo chuyển đúng 1→2→3 khi attack được buffer hoặc bấm trong window; hết window thì chain quay lại combo_1.
- Buffer tối đa 0.15s được giữ đến lúc window mở; input hết hạn không tự nối đòn.
- Dodge không thể hủy chiêu trước khi active window kết thúc; trong cancel window dodge bắt đầu và attack bị hủy.
- Hit tạo hit-stop theo skill; controller, Dummy knockback và animation không tiến trong thời gian freeze, rồi tiếp tục sau khi timer hết.
- Dummy bị đẩy đúng hướng, giảm vận tốc tới 0 trong thời lượng cấu hình và không xuyên tường/sàn nhờ `CollisionWorld`.
- DebugPanel có combo step/window, cancel status, hit-stop status và knockback velocity.
- `glory_game` không phụ thuộc SDL/OpenGL; toàn bộ CTest cũ và mới PASS, smoke-test đủ 120 fixed ticks; có test report/review ba nhóm.
