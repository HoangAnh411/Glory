# Báo cáo kiểm thử: Dodge và Capsule Collision

## Lệnh đã thực thi

1. `& 'C:\Program Files\CMake\bin\cmake.exe' --build build`
2. `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure`
3. `.\build\glory_client.exe --smoke-test`

## Kết quả tổng quan

**PASS** — build thành công; 12/12 CTest thành công (11 test hiện có và test collision mới); smoke-test exit code 0 sau đúng 120 fixed ticks.

## Chi tiết test

- `glory_fixed_timestep`: PASS.
- `glory_input_mapping`: PASS — bao gồm binding mặc định Dodge = Left Shift.
- `glory_camera`: PASS.
- `glory_asset_loader`: PASS.
- `glory_directional_light`: PASS.
- `glory_skeletal_animation`: PASS.
- `glory_shadow`: PASS.
- `glory_pbr`: PASS.
- `glory_particle`: PASS.
- `glory_character_controller`: PASS — tốc độ dash, hướng facing khi đứng yên, I-frame start/end, thời lượng dodge và cooldown.
- `glory_third_person_camera`: PASS.
- `glory_collision`: PASS — wall depenetration, vận tốc tiếp tuyến khi trượt tường, ramp đi được, chặn ramp 45°, leo và đứng trên step cao 0.2m, cấu hình terrain mẫu.
- Smoke-test: PASS, output `SDL3/OpenGL 4.5 context and 120 fixed ticks passed`; audio engine báo `ready`.
- `git diff --check`: PASS, không phát hiện whitespace error.

## Vòng sửa trong quá trình kiểm thử

- Build ban đầu phát hiện thiếu include GLM `vec2`; đã bổ sung include cần thiết.
- CTest lần đầu phát hiện ngưỡng float khiến ramp đúng 45° bị xếp walkable và test step kết thúc ngoài mặt step. Resolver đã đổi sang kiểm tra góc nghiêm ngặt dưới 45°; test step kết thúc trên bề mặt bậc để xác nhận grounded. Build và toàn bộ test sau sửa đều PASS.
