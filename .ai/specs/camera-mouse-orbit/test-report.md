# Báo cáo kiểm thử

## Kết quả cuối

- `cmake --build build`: PASS.
- `build/glory_client.exe --camera-test`: PASS, exit code 0.
- `ctest --test-dir build --output-on-failure`: PASS, 37/37 test.
- `build/glory_client.exe --smoke-test`: PASS, SDL3/OpenGL 4.5, 120 fixed ticks, HDR framebuffer complete, exit code 0.
- `git diff --check`: PASS.

## Bằng chứng camera-test

Các JSON và PNG cuối nằm trong `captures/camera-test/`:

- `yaw_30fps_camera-test`: yaw cuối -90°, pivot trong NDC, arm 5.0 m.
- `yaw_60fps_camera-test`: yaw cuối -90°, pivot trong NDC, arm 3.603 m, ngoài collider.
- `yaw_144fps_camera-test`: yaw cuối -90°, pivot trong NDC, arm 3.449 m, ngoài collider.
- `pitch_limits_camera-test`: quan sát đủ pitch -10° và 70°, kết thúc ở -10°.
- `zoom_range_camera-test`: arm 3.001 m sau zoom về 3 m, pivot hợp lệ.
- `wall_arm_camera-test`: arm 2.761 m, ngoài collider, blocker thật `terrain_hill_green#63`.

Mỗi frame capture đặt marker magenta tại pivot và đọc marker bằng `glReadPixels`; HDR marker source ghi `255,0,255`. FPS rotation check trong JSON PASS cho 30/60/144 FPS giả lập.

## Baseline

Trước thay đổi, `HEAD:game/src/main.cpp` không có option `--camera-test`, nên đường chạy nghiệm thu này thất bại ở mức CLI và chưa thể đo hình học camera cũ bằng cùng telemetry. Bản cuối đã thêm đường chạy thật và PASS toàn bộ tiêu chí đã kiểm tra. Không đánh dấu `PLAN.md` vì baseline hình học cũ chưa có artifact runtime độc lập.

## Ảnh đã xem lại

Đã mở và kiểm tra trực tiếp cả sáu PNG; các ảnh hiển thị scene, nhân vật và marker pivot, gồm cả góc pitch cao/thấp, zoom gần và vị trí sát collider.
