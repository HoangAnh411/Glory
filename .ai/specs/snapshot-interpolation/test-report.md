# Báo cáo kiểm thử: Snapshot và nội suy

Ngày chạy: 2026-10-14

## Lệnh đã thực thi

- `cmake --build build` — PASS, Ninja báo `no work to do` (build đã cập nhật).
- `ctest --test-dir build --output-on-failure` — PASS, 22/22 tests.
- `./build/glory_server.exe --smoke-test --port 17779` — PASS, 120 fixed ticks ở 60 Hz, exit code 0.
- `./build/glory_client.exe --smoke-test` — PASS, SDL3/OpenGL 4.5 và 120 fixed ticks, exit code 0.

## Kết quả test mới

- Snapshot endpoint và vị trí tại alpha 0.5: PASS.
- Yaw shortest path từ 350° tới 10°: PASS.
- Snapshot đến sai thứ tự và buffer giữ thứ tự tick: PASS.
- Extrapolation bị giới hạn tối đa: PASS.
- Remote player bị loại khi snapshot mới không còn client: PASS.

## Tổng quan

Tất cả build, CTest và smoke test yêu cầu đều PASS. CTest ghi nhận `100% tests passed out of 22` (tổng thời gian 0.51 giây). Không có log lỗi.
