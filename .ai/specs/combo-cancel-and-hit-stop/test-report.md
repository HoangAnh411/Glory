# Báo cáo kiểm thử: Combo, Cancel Window, Hit-stop và Knockback

## Lệnh đã chạy

- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build` — exit code 0.
- `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure` — exit code 0; 13/13 test PASS.
- `.\build\glory_client.exe --smoke-test` — exit code 0; `SDL3/OpenGL 4.5 context and 120 fixed ticks passed`.
- `git diff --check` — exit code 0.

## Kết quả

**PASS** — toàn bộ 13 CTest và smoke-test 120 fixed ticks thành công.

## Các trường hợp đã kiểm tra

- Skill JSON đọc đủ 5 cấu hình; combo damage tăng từ 20 → 30 → 50 — PASS.
- Combo khởi chạy nhịp 1, input buffer nối sang nhịp 2/3, input hết hạn không tự nối, timeout đóng cửa sổ và reset combo — PASS.
- Dodge bị khóa trước recovery; cancel window mở sau active frames; dodge trong window hủy đòn và reset chain — PASS.
- Hit-stop light strike giữ đúng 0.06s; thời gian đòn và vị trí Dummy đứng yên trong freeze — PASS.
- Knockback light đẩy Dummy theo hướng ra xa attacker, giảm tốc tuyến tính tới 0 và giữ grounded — PASS.
- Collision resolver chặn knockback đi xuyên AABB wall — PASS.
- `glory_fixed_timestep`, `glory_input_mapping`, `glory_camera`, `glory_asset_loader`, `glory_directional_light`, `glory_skeletal_animation`, `glory_shadow`, `glory_pbr`, `glory_particle`, `glory_character_controller`, `glory_third_person_camera`, `glory_collision` — PASS.

## Vòng sửa

- Build đầu tiên phát hiện cách truyền `{0.0F}` không tương thích constructor explicit của `glm::vec3`; test được đổi sang `glm::vec3(0.0F)`.
- CTest đầu tiên phát hiện resolver nhận vận tốc cũ, khiến knockback không giảm tốc; đã truyền vận tốc sau deceleration vào resolver. Build và toàn bộ test cuối cùng PASS.
