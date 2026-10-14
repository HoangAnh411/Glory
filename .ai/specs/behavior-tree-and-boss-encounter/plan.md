# Kế hoạch Behavior Tree và Boss Encounter

## 1. Mục tiêu
Hoàn thiện gameplay single-player Giai đoạn 3 bằng framework Behavior Tree thuần C++ và một Boss hai pha có thể chiến đấu hai chiều với người chơi. AI, pha Boss, sát thương, né tránh, hit-stop và knockback không phụ thuộc SDL/OpenGL.

## 2. Công việc
1. Tạo `NodeStatus`, `AiContext`, node cơ sở, Selector, Sequence, ActionNode, ConditionNode và Inverter; kiểm thử các trạng thái chạy.
2. Tạo Boss hai pha với HP 300, hurtbox, BT chọn Chase/Slash ở pha 1 và Charge/Slam/Chase ở pha 2; xử lý Enrage một lần, buffs, hit-stop và trạng thái Dead.
3. Tích hợp Boss với player combat, HP, dodge I-frame, sát thương/knockback hai chiều, reset encounter và victory state trong game loop.
4. Truyền telemetry về DebugPanel: HP người chơi/Boss, phase, BT node, victory, enrage; thêm nút reset trận.
5. Thêm hai CTest CPU cho Behavior Tree và Boss; build, chạy toàn bộ CTest và smoke test; ghi báo cáo và review.

## 3. File cần sửa hoặc tạo
- Tạo `game/include/glory/game/behavior_tree.hpp`, `game/include/glory/game/boss.hpp`, `game/src/boss.cpp`.
- Sửa `game/src/main.cpp`, `game/include/glory/game/character_controller.hpp`, `game/src/character_controller.cpp`, `game/include/glory/game/combat.hpp`, `game/src/combat.cpp`, `CMakeLists.txt`, `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp`, `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp`, `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp`.
- Tạo `tests/behavior_tree_tests.cpp`, `tests/boss_tests.cpp`.
- Tạo `.ai/specs/behavior-tree-and-boss-encounter/plan.md`, `test-report.md`, `review.md`; cập nhật ba mục cuối của Giai đoạn 3 trong `PLAN.md` sau khi PASS.

## 4. Rủi ro và cách giảm thiểu
- Trạng thái Running trong composite cần nhớ chỉ số con để Sequence không khởi động lại action đang chạy; Selector phải dọn trạng thái khi nhánh kết thúc.
- Boss không được gây sát thương mỗi tick trong một đòn; cooldown và cờ tấn công đang diễn ra giới hạn mỗi lần trúng một hit.
- Phase transition và enrage chỉ chạy một lần ngay dưới ngưỡng 50%; test cả lần chuyển pha và HP bằng 0.
- Input Dodge/I-frame được đánh giá cùng tick nhận đòn để người chơi né được đòn quét.
- Engine chỉ nhận telemetry/event đơn giản; behavior tree và boss simulation không gọi renderer hoặc SDL.

## 5. Tiêu chí nghiệm thu
- Các node Behavior Tree trả Success/Failure/Running đúng; composite và inverter có kiểm thử CPU.
- Pha 1 đuổi khi xa, chém khi trong tầm; pha 2 mở Charge/Slam và Enrage buff/hit-stop khi HP dưới 50%; Boss dừng AI khi chết.
- Player có thể gây sát thương và knockback Boss; Boss có thể gây sát thương/knockback, nhưng không làm sát thương khi player đang I-frame.
- DebugPanel hiển thị HP hai bên, phase, node đang chạy và trạng thái chiến thắng; Reset Boss Fight khôi phục encounter.
- Cả 14 CTest hiện tại và hai test mới PASS; executable hoàn thành smoke test 120 tick.
