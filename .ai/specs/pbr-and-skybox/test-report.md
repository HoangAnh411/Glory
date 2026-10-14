# Test Report: PBR cơ bản và skybox

## Lệnh đã thực thi

1. `& 'C:\Program Files\CMake\bin\cmake.exe' --build build`
2. `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure`
3. `.\build\glory_client.exe --smoke-test`

## Kết quả tổng quan

**PASS** — build thành công, CTest 8/8 PASS, smoke test thoát mã 0 sau 120 fixed ticks.

## Chi tiết

- Build: PASS (exit code 0). Lượt build đầu phát hiện lỗi include GLM và khai báo ma trận kiểu vexing parse; đã sửa rồi chạy lại build thành công.
- `glory_fixed_timestep`: Passed.
- `glory_input_mapping`: Passed.
- `glory_camera`: Passed.
- `glory_asset_loader`: Passed.
- `glory_directional_light`: Passed.
- `glory_skeletal_animation`: Passed.
- `glory_shadow`: Passed.
- `glory_pbr`: Passed; kiểm tra F0 dielectric 0.04, F0 metallic bằng albedo, GGX theo roughness và giới hạn Smith geometry.
- Smoke: PASS (exit code 0), output: `SDL3/OpenGL 4.5 context and 120 fixed ticks passed`.
- `git diff --check`: PASS; chỉ xuất warning chuẩn hóa CRLF của một số file đã có thay đổi trong worktree.

## Lỗi

Không còn lỗi sau lượt sửa build đầu tiên.
