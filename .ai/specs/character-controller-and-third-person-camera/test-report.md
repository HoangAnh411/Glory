# Báo cáo kiểm thử: Character Controller và Third-Person Camera

## Lệnh đã thực thi

1. `& 'C:\Program Files\CMake\bin\cmake.exe' --build build`
2. `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure`
3. `.\build\glory_client.exe --smoke-test`

## Kết quả tổng quan

**PASS** — build thành công; 11/11 bài CTest thành công; smoke-test trả exit code 0 sau 120 fixed ticks.

## Chi tiết

- Build CMake/Ninja: PASS (exit code 0). Lần configure phát sinh cảnh báo CMake deprecation trong dependency GLM/FetchContent và thiếu PkgConfig/LibUSB tùy chọn; không ảnh hưởng build.
- `glory_fixed_timestep`: PASS.
- `glory_input_mapping`: PASS.
- `glory_camera`: PASS.
- `glory_asset_loader`: PASS.
- `glory_directional_light`: PASS.
- `glory_skeletal_animation`: PASS.
- `glory_shadow`: PASS.
- `glory_pbr`: PASS.
- `glory_particle`: PASS.
- `glory_character_controller`: PASS — movement theo yaw camera, tiến/lùi/rẽ, chuẩn hóa diagonal, jump/apex/gravity/grounding, smooth facing và attack edge.
- `glory_third_person_camera`: PASS — follow offset/distance, mouse orbit, pitch clamp, view-projection hữu hạn và cấu hình không hợp lệ.
- Smoke-test: PASS (exit code 0), output `SDL3/OpenGL 4.5 context and 120 fixed ticks passed`; audio engine báo `ready`.

## Ghi chú vòng kiểm thử

Lần CTest đầu tiên báo FAIL ở assertion hướng rẽ phải trong test mới; controller theo đúng quy ước yaw của FreeCamera, còn dấu kỳ vọng của assertion bị ngược. Đã sửa assertion, build lại và chạy lại toàn bộ CTest cùng smoke-test; lượt cuối đều PASS.
