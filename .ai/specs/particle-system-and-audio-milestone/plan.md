# Particle System và Audio milestone

## 1. Mục tiêu

Hoàn tất hai mục cuối Giai đoạn 2 bằng hệ thống hạt có mô phỏng CPU độc lập OpenGL, renderer billboard có alpha blending, AudioSystem bọc miniaudio không lộ header ngoài engine, và nối event `swing_sound`/`hit_frame` với âm thanh cùng particle burst. DebugPanel có điều khiển thử nghiệm và trạng thái âm thanh/hạt.

## 2. Danh sách task

1. Thêm `Particle`, `ParticleConfig` và `ParticleSystem` thuần CPU: burst hình nón với vận tốc ngẫu nhiên có seed, gravity, lifetime, alpha fade và xóa hạt hết hạn.
2. Thêm `ParticleRenderer` RAII với quad billboards hướng camera, alpha blending, tắt depth write trong lúc draw và khôi phục OpenGL state.
3. Thêm miniaudio qua FetchContent v0.11.21; cô lập `MINIAUDIO_IMPLEMENTATION` vào một translation unit; bọc `ma_engine` bằng `AudioSystem` PIMPL, master volume/mute và phát sample WAV.
4. Thêm hai âm thanh mẫu ngắn dạng WAV tổng hợp để chạy được khi không có asset âm thanh khác.
5. Tích hợp ParticleSystem/ParticleRenderer vào ModelRenderer; cập nhật hạt từng frame và vẽ sau scene.
6. Đăng ký callback animation một lần trong application: `swing_sound` phát swing; `hit_frame` tạo burst tại vùng tấn công và phát hit. Ghi log event qua DebugPanel mà không ghi đè callback.
7. Mở rộng DebugPanel: nút Particle Burst, số hạt hoạt động, Master Volume và Mute.
8. Trong smoke mode trigger Attack, burst và sample sound; bảo đảm renderer, audio engine và SDL/GL resources được hủy trước context/window.
9. Thêm test CPU độc lập cho spawn, chuyển động/gravity và cleanup; đăng ký test CTest thứ chín; build/CTest/smoke, ghi test report và review.

## 3. File cần sửa / tạo mới

- Tạo `engine/include/glory/engine/particle_system.hpp`, `engine/src/particle_system.cpp`.
- Tạo `engine/include/glory/engine/particle_renderer.hpp`, `engine/src/particle_renderer.cpp`.
- Tạo `engine/include/glory/engine/audio_system.hpp`, `engine/src/audio_system.cpp`, `engine/src/miniaudio_impl.cpp`.
- Tạo `tests/particle_tests.cpp`.
- Tạo `assets/audio/swing.wav`, `assets/audio/hit.wav` (ngắn, mono/stereo PCM, tổng hợp trong repository).
- Sửa `engine/include/glory/engine/gl_api.hpp`, `engine/src/gl_api.cpp` để hỗ trợ blend function, depth write mask và disable.
- Sửa `engine/include/glory/engine/model_renderer.hpp`, `engine/src/model_renderer.cpp` để sở hữu/cập nhật/vẽ hạt và cung cấp burst/count.
- Sửa `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp` cho particle/audio UI và event log API.
- Sửa `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp`, `game/src/main.cpp` để truyền smoke config/audio asset path, kết nối callback và lifetime.
- Sửa `CMakeLists.txt` để lấy miniaudio, thêm nguồn/test/asset directory define.
- Tạo `.ai/specs/particle-system-and-audio-milestone/test-report.md`, `review.md`; cập nhật `PLAN.md` sau PASS.

## 4. Rủi ro và giảm thiểu

- Thiết bị audio không tồn tại trong môi trường headless: AudioSystem giữ trạng thái unavailable và bỏ qua phát âm thanh, không làm hỏng khởi động client hoặc smoke; vẫn kiểm tra API và resource cleanup.
- Callback hạt/âm thanh cần sống lâu hơn animation update: đặt AudioSystem, DebugPanel và ModelRenderer trong scope nằm trọn trước khi hủy GL context; callback đăng ký một lần, không thay thế trong mỗi frame.
- Hạt có thể gây allocation hoặc overdraw lớn: giới hạn burst count, reserve buffer và stream vertex data một lần mỗi frame.
- Hòa trộn OpenGL phải phục hồi state: bật blend/depth mask false chỉ trong hàm particle draw và khôi phục depth mask true, tắt blend trước khi quay lại ImGui.
- CMake FetchContent miniaudio là header-only: chỉ thêm include path, không thêm implementation thứ hai hoặc leak include directory sang game.

## 5. Tiêu chí chấp nhận

- Unit test CPU không khởi tạo GL/SDL/audio, kiểm tra số hạt sau burst, vận tốc/độ dịch chuyển chịu gravity và dọn sạch theo lifetime.
- Particle tạo burst được, alpha giảm theo lifetime, renderer billboard camera-facing; OpenGL blending được bật và depth mask luôn được khôi phục.
- `MINIAUDIO_IMPLEMENTATION` chỉ xuất hiện trong đúng một translation unit; game chỉ dùng API `AudioSystem` nội bộ; audio engine RAII được uninit khi còn SDL app scope.
- Animation event `swing_sound` gọi âm thanh chém; `hit_frame` gọi âm thanh trúng đòn và particle burst.
- DebugPanel có nút burst, particle count, volume slider và mute.
- Build PASS; toàn bộ 9 CTest PASS; `glory_client.exe --smoke-test` thoát mã 0 sau 120 tick và kích hoạt animation/particle/audio; tài nguyên được hủy trước GL context/window.
