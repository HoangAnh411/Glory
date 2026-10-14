# Báo cáo kiểm thử: Hitbox, Hurtbox và Skills JSON

## Lệnh đã thực thi

- `& 'C:\Program Files\CMake\bin\cmake.exe' -S . -B build` — exit code 0; cấu hình CMake và fetch `nlohmann/json` thành công.
- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build` — exit code 0.
- `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure` — exit code 0; 13/13 test PASS.
- `.\build\glory_client.exe --smoke-test` — exit code 0; `SDL3/OpenGL 4.5 context and 120 fixed ticks passed`.
- `git diff --check` — exit code 0; không có whitespace error.

## Kết quả

**PASS** — build hoàn tất; 12 test trước đó và test combat mới đều PASS; smoke-test thoát sạch sau 120 fixed ticks.

## Test cases

- `glory_combat`: nạp đúng 2 skill; damage/cooldown/cast-time khớp JSON — PASS.
- Hitbox sphere giao cắt hurtbox capsule khi nằm trong tầm và bỏ qua mục tiêu ngoài tầm — PASS.
- Slash không gây damage trước `activeStart`, trúng trong cửa sổ, không multi-hit trong cùng activation, và chặn cast khi cooldown chưa hết — PASS.
- Animation state ngoài skill state ngăn hit resolution — PASS.
- Heavy Strike đợi cast-time và active window, sau đó gây đúng 60 damage — PASS.
- Reset Dummy khôi phục HP và hit count — PASS.
- `glory_fixed_timestep`, `glory_input_mapping`, `glory_camera`, `glory_asset_loader`, `glory_directional_light`, `glory_skeletal_animation`, `glory_shadow`, `glory_pbr`, `glory_particle`, `glory_character_controller`, `glory_third_person_camera`, `glory_collision` — PASS.
- Smoke-test khởi tạo audio/renderer và kết thúc đúng 120 fixed ticks — PASS.

## Ghi chú

- Lần build đầu báo `glm::radians` chưa được khai báo trong `combat.cpp`; đã thay bằng phép đổi độ sang radian tương đương. Các lần build/test cuối đều PASS.
- Smoke-test kiểm tra khởi tạo và teardown của ứng dụng, không tự động nhấn nút Cast/Reset trong DebugPanel.
