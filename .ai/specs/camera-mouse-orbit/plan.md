# Camera Mouse Orbit và Kiểm Chứng Runtime

## Mục tiêu

Hoàn thiện điều khiển chuột relative mode và camera orbit theo yêu cầu: input chuột không phụ thuộc FPS; con trỏ được nhả/bắt lại đúng trạng thái cửa sổ và UI; orbit dùng pivot ngực, spring arm va chạm AABB, zoom mượt; có đường chạy `--camera-test` kiểm chứng qua vòng lặp SDL/OpenGL thực tế. Không thay đổi hitbox, combo, sát thương hoặc tick `PLAN.md` khi chưa đủ chứng cứ.

## Khảo sát hiện trạng

- `ThirdPersonCamera` đã có mặc định 5 m, FOV 55°, pivot cao 1.4 m, sensitivity 0.1°/pixel, pitch [-10°, 70°], spring pivot, arm tối thiểu 2 m và zoom range 3–8 m.
- `InputSystem` đã nhận relative motion delta, wheel và mất focus; `application.cpp` đã bật/tắt relative mode theo Alt/F3/Esc/focus và bỏ delta lúc chuyển mode.
- F3 đã lưu sensitivity, invert Y, zoom, FOV và shoulder vào `settings/camera.json`.
- Chưa có `--camera-test`; cần kiểm chứng độ phủ thật của runtime, camera collider, pixel marker và nhiều FPS thay vì chỉ dựa unit test.
- Có thay đổi collision chưa commit trong `game/src/main.cpp`, test và assets. Mọi commit camera phải chỉ chứa các hunk/file camera; bảo toàn thay đổi đang có.

## Kế hoạch triển khai

1. **Camera/input runtime**
   - Rà soát và hoàn thiện `engine/include/glory/engine/third_person_camera.hpp`: yaw/pitch áp delta trực tiếp không nhân `dt`; zoom smoothing; giữ giới hạn pitch, ground clearance, spring pivot và arm rút nhanh/giãn chậm; không tự xoay về sau lưng khi không khóa mục tiêu.
   - Rà soát `engine/src/application.cpp` và `engine/src/input_system.cpp`: relative mouse mode, raw delta nếu SDL/platform hỗ trợ, bỏ frame delta đầu khi bắt chuột, xử lý Alt/F3/Esc/click/focus, wheel và không xoay camera khi con trỏ được nhả hoặc UI đang mở.
   - Rà soát `engine/src/debug_panel.cpp` và header tương ứng: sensitivity, invert Y, zoom, FOV, shoulder và công tắc auto-return; lưu/cập nhật thiết lập tại `settings/camera.json`.
   - Spring arm dùng camera AABB collider thật hiện có; bỏ qua player/boss và collider nhỏ hơn 1.2 m; không slide ngang; giữ camera ngoài collider và làm mờ nhân vật khi arm chạm ngưỡng 2 m nếu renderer hỗ trợ alpha/dither.

2. **Kiểm chứng runtime**
   - Thêm cờ `--camera-test` trong `game/src/main.cpp`/`ApplicationConfig`, chạy qua cùng event/update/fixed timestep/render pipeline với input chuột kịch bản có kiểm soát.
   - Tạo các chặng xoay 360° trong 4 giây, pitch tới hai giới hạn, zoom 3–8 m và đi sát vật cản. Vẽ marker magenta tại pivot, dùng `glReadPixels` xác nhận marker thực sự được render.
   - Ghi telemetry JSON: yaw/pitch, pivot NDC, actual arm, trạng thái giao cắt collider và kết quả cùng tổng delta chuột tại 30/60/144 FPS giả lập. Thất bại nếu pivot vượt x [-0.12, 0.12], y [-0.35, -0.05], arm < 2 m, camera nằm trong collider, pixel marker không đọc được, hoặc rotation khác nhau theo FPS.
   - Chạy một baseline trước khi sửa logic liên quan để lưu chứng cứ FAIL của tiêu chí camera cũ; không làm thay đổi file roadmap.
   - Xuất sáu ảnh vào thư mục capture riêng của tác vụ, đọc lại ảnh để xác nhận framing và marker. Không ghi đè artifacts collision hiện có.

3. **Quy trình xác nhận**
   - Cập nhật/tạo unit test cho giới hạn và phép tính độc lập FPS.
   - Build chuẩn CMake/Ninja, chạy CTest, `glory_bootstrap.exe --camera-test`, smoke test và lưu kết quả thật vào `test-report.md`.
   - Reviewer đối chiếu plan, `PLAN.md` và diff; ghi `review.md`. Chỉ sửa mục camera trong roadmap nếu mọi tiêu chí và đường chạy thật PASS; theo yêu cầu hiện tại giữ nguyên roadmap nếu chưa có chứng cứ đủ.
   - Tạo 1–3 commit nhỏ, tách riêng nội dung camera khỏi các thay đổi va chạm đang có trong worktree.

## File dự kiến

- `engine/include/glory/engine/third_person_camera.hpp`
- `engine/include/glory/engine/application.hpp`
- `engine/src/application.cpp`
- `engine/src/input_system.cpp` và header liên quan nếu cần raw-delta/state helper
- `engine/include/glory/engine/debug_panel.hpp`
- `engine/src/debug_panel.cpp`
- `engine/include/glory/engine/model_renderer.hpp` / `engine/src/model_renderer.cpp` nếu cần kiểm chứng/dither camera
- `game/src/main.cpp` (chỉ stage hunk của `--camera-test`, giữ thay đổi collision chưa commit)
- `tests/third_person_camera_tests.cpp` và/hoặc test mới
- `.ai/specs/camera-mouse-orbit/test-report.md`
- `.ai/specs/camera-mouse-orbit/review.md`
- Capture/telemetry riêng tại `captures/camera-test/`

## Tiêu chí nghiệm thu

- Mouse look: SDL relative mode và con trỏ ẩn lúc chơi; dùng raw delta; delta pixel nhân sensitivity, không `dt`/lọc; bỏ delta đầu khi bắt lại. Alt/F3/Esc/focus/click hoạt động đúng mô tả.
- Cấu hình: sensitivity mặc định 0.10°/pixel, giới hạn 0.02–0.5, invert Y; lưu bền; wheel zoom 3–8 m, mặc định 5 m; FOV mặc định 55°; shoulder mặc định 0 và tối đa 0.4 m.
- Orbit: pivot 1.4 m; pitch -10°..70°, mặc định 15°; pivot spring khoảng 0.1 s; mặt đất clearance 0.4 m; arm xử lý AABB, không slide, arm tối thiểu 2 m và giãn khoảng 0.5 s; auto-return tắt mặc định.
- `--camera-test` dùng đường chạy game/render thật; pixel marker được đọc bằng `glReadPixels`; kiểm tra NDC, collider, min arm và độ nhất quán ở 30/60/144 FPS; baseline camera cũ thể hiện FAIL và bản sửa PASS.
- Sáu ảnh được ghi, mở/kiểm tra trực quan, build/CTest/smoke PASS; không thay đổi hitbox/combo/sát thương; báo cáo và review được ghi; không stage/commit thay đổi collision khác.
