# Báo cáo kiểm thử: Behavior Tree và Boss Encounter

## Lệnh đã chạy
- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build` — PASS, exit code 0.
- `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure` — PASS, 16/16 test.
- `.\build\glory_client.exe --smoke-test` — PASS, SDL3/OpenGL khởi tạo và đủ 120 fixed tick.

## Tiêu chí đã kiểm tra
- Selector, Sequence, Condition, Action và Inverter trả Success/Failure/Running đúng — PASS.
- Boss pha 1 đuổi người chơi ngoài tầm, chém khi ở gần và tuân thủ cooldown — PASS.
- Dưới 50% HP, Boss kích hoạt Enrage một lần, nhận AttackBoost/SpeedBoost và hit-stop — PASS.
- Pha 2 thực hiện Charge/Slam; đòn đánh có hitbox, sát thương và knockback — PASS.
- Player I-frame vô hiệu hóa sát thương/knockback; HP bằng 0 đưa Boss vào Dead — PASS.
- Combat tích hợp damage, Bleed, hit-stop và knockback giữa Player/Boss — PASS.
- Reset encounter và hai test CTest mới — PASS.

## Kết quả
Build, toàn bộ 16 CTest và smoke test đều PASS. Cấu hình CMake có các cảnh báo deprecation từ dependency hiện có, không có lỗi biên dịch.
