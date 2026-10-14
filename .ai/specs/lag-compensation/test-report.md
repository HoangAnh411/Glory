# Báo cáo kiểm thử: Lag compensation

Ngày chạy: 2026-10-14

## Lệnh đã thực thi

- `cmake --build build` — PASS.
- `ctest --test-dir build --output-on-failure` — PASS, 24/24 tests.
- `.\build\glory_server.exe --smoke-test --port 17779` — PASS, 120 fixed ticks ở 60 Hz, exit code 0.
- `.\build\glory_client.exe --smoke-test` — PASS, SDL3/OpenGL 4.5 và 120 fixed ticks, exit code 0.

## Kiểm thử lag compensation

- History query tại tick nguyên và nội suy hurtbox giữa hai tick: PASS.
- Target đi tuyến tính X=0→5; ở tick nhìn thấy X=2, hitbox trúng dù vị trí hiện tại ở X=4: PASS.
- Không bù trễ thì đòn đánh trượt; bù trễ thì HP giảm 100→75: PASS.
- Transform authoritative vẫn ở X=4 sau hit; chỉ hit test đọc hurtbox lịch sử: PASS.
- Rewind cũ hơn 0.35 giây bị kẹp về 21 tick; tick tương lai bị kẹp về server tick hiện tại: PASS.
- Buffer giữ đúng cửa sổ 61 sample và loại tick quá cũ: PASS.
- Binary input codec round-trip `targetTick`: PASS.
- CTest authoritative PvP và snapshot/network hiện hữu: PASS.

## Tổng quan

Build và toàn bộ 24 CTest đều PASS (`100% tests passed out of 24`). Smoke test server và client đều PASS. Lần biên dịch đầu phát hiện test truyền `glm::vec3` qua khởi tạo copy không hợp lệ; đã đổi sang khởi tạo tường minh và build cuối thành công.
