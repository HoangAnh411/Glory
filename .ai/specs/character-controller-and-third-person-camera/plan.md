# Kế hoạch: Character Controller và Third-Person Camera

## 1. Mục tiêu

Hoàn thành bước đầu tiên của Giai đoạn 3 trong `PLAN.md`: điều khiển nhân vật chạy/nhảy trên mặt phẳng y = 0, xoay theo hướng di chuyển, điều khiển camera third-person bằng chuột, đồng bộ Idle/Run/Attack và hiển thị trạng thái trong DebugPanel. Giữ `CharacterController` trong `glory_game`, chỉ chứa logic toán học và không phụ thuộc renderer/OpenGL.

## 2. Khảo sát hiện trạng và quyết định tích hợp

- `ActionMap` hiện đã ánh xạ W/A/S/D, Space và chuột trái; `InputFrame` có pressed/held/released cùng relative mouse delta.
- `FreeCamera` được tạo, cập nhật và truyền vào renderer bên trong `runApplication`. `DebugPanel` nhận `FreeCamera` trực tiếp.
- `Simulation` hiện chỉ đếm fixed tick; `main.cpp` cập nhật `ActionMap` và `Simulation` qua callback.
- Renderer đã có API đặt trạng thái animation, particle burst và animation event gọi audio. Smoke-test hiện chủ động kích hoạt Attack.
- Cách nối gameplay dự kiến: `glory_game` sở hữu controller và expose snapshot trạng thái qua API trung lập với renderer; callback fixed update của ứng dụng đưa snapshot đó vào phần trình bày. Renderer vẫn tiêu thụ pose/camera từ lớp ứng dụng, trong khi controller không include bất kỳ header engine/render nào. Third-person camera là lớp engine toán học độc lập và được cập nhật từ vị trí nhân vật cùng input delta.
- Giữ Free-Fly làm lựa chọn debug; phím F1 chuyển chế độ camera. Nếu SDL input hiện chưa có F1, bổ sung control và binding/input forwarding nhỏ nhất cần thiết.

## 3. Danh sách task

1. **Third-person camera:** thêm `ThirdPersonCamera` với target, offset, distance, yaw/pitch, sensitivity, pitch clamp, aspect ratio và ma trận view/projection. Bỏ qua delta/tham số không hữu hạn; kiểm tra cấu hình camera hợp lệ.
2. **Character controller:** thêm `CharacterController` trong `glory_game`. Nhận `ActionMap` và yaw camera, chuẩn hóa hướng chạy XZ (kể cả diagonal), áp dụng tốc độ mặc định 5.5 m/s, gravity 18 m/s², jump velocity 7 m/s và ground check y=0. `facingYaw` nội suy theo góc ngắn nhất với tốc độ quay có giới hạn theo thời gian.
3. **Trạng thái gameplay:** cung cấp position, velocity, facingYaw, isGrounded và ý định animation/attack trong state thuần dữ liệu; PrimaryAttack chỉ phát lệnh Attack ở cạnh pressed, không lặp khi giữ nút. State API không gọi renderer.
4. **Tích hợp ứng dụng:** nối controller vào fixed update; gửi player position/facing vào transform hiển thị, target camera và DebugPanel. Camera có Third-Person/Free-Fly toggle bằng F1. Gọi Idle/Run theo state ở mặt đất và Attack theo sự kiện PrimaryAttack; giữ các callback event hiện có cho particle/audio. Cập nhật transform model và camera view/projection mỗi frame mà không chuyển trách nhiệm render vào module game.
5. **DebugPanel và input:** hiển thị tọa độ, vận tốc, isGrounded, chế độ camera; bảo đảm ImGui capture vẫn chặn gameplay input phù hợp.
6. **Kiểm thử CPU:** thêm `character_controller_tests` vào CTest, kiểm tra tiến/lùi/rẽ theo camera yaw, chuẩn hóa di chuyển, nhảy/đỉnh/rơi/tiếp đất, và xoay mặt. Thêm camera third-person tests cho follow, mouse orbit, pitch clamp và ma trận.
7. **Build, test, smoke, review:** build bằng build tree hiện có; chạy toàn CTest (9 hiện tại + controller tests + camera tests nếu tách suite), và `glory_client.exe --smoke-test` phải chạy đủ 120 fixed ticks, thoát mã 0. Ghi log thật trong `test-report.md`; review theo ba nhóm trong `review.md`; xử lý mọi mục Phải sửa rồi test lại.
8. Chỉ sau khi PASS build/test và review không còn Phải sửa, tick mục Character controller trong `PLAN.md`.

## 4. File dự kiến tạo/sửa

- Tạo `engine/include/glory/engine/third_person_camera.hpp` (API camera toán học).
- Tạo `game/include/glory/game/character_controller.hpp` và `game/src/character_controller.cpp`.
- Sửa `CMakeLists.txt` để biên dịch controller, link GLM cho `glory_game`, đăng ký test controller và camera.
- Sửa `engine/include/glory/engine/application.hpp` và `engine/src/application.cpp` để nối gameplay state, camera mode và cập nhật pose trước render.
- Sửa `engine/include/glory/engine/model_renderer.hpp` / `engine/src/model_renderer.cpp` nếu cần API transform nhân vật; giữ renderer chịu trách nhiệm duy nhất về vẽ.
- Sửa `engine/include/glory/engine/debug_panel.hpp` / `engine/src/debug_panel.cpp` cho player telemetry và camera mode.
- Sửa `engine/include/glory/engine/input_system.hpp` / `engine/src/input_system.cpp` và `shared/include/glory/input/input_frame.hpp` nếu cần F1 toggle.
- Sửa `game/src/main.cpp` và `game/include/glory/game/simulation.hpp` / `game/src/simulation.cpp` để sở hữu, cập nhật controller và cung cấp snapshot gameplay.
- Tạo `tests/character_controller_tests.cpp`; tạo `tests/third_person_camera_tests.cpp` nếu camera test cần suite riêng.
- Sau khi hoàn tất các vai trò, tạo `.ai/specs/character-controller-and-third-person-camera/test-report.md`, `.ai/specs/character-controller-and-third-person-camera/review.md` và cập nhật `PLAN.md`.

## 5. Rủi ro và cách giảm thiểu

- **API callback ứng dụng đã được dùng trong smoke test:** thay đổi callback có thể làm đứt vòng lặp. Giữ một nguồn fixed tick rõ ràng và xác nhận smoke test đúng 120 tick.
- **Xung đột tọa độ camera/nhân vật:** thống nhất yaw độ, Y-up, hướng trước theo quy ước yaw hiện tại của `FreeCamera`; camera chỉ dùng projection/view, character chạy ngang XZ.
- **Quyền sở hữu logic:** không để `CharacterController` phụ thuộc `ModelRenderer`, SDL hoặc OpenGL; animation state là dữ liệu/lệnh do lớp tích hợp tiêu thụ.
- **Animation Attack bị ghi đè bởi Idle/Run:** ưu tiên Attack khi có cạnh pressed và chỉ tiếp tục locomotion sau khi state attack kết thúc hoặc theo cơ chế transition sẵn có.
- **Thay đổi hiện có trong working tree:** `CMakeLists.txt`, `PLAN.md`, application và main đang modified trước tác vụ này. Cần đọc diff hiện tại trước khi chỉnh để bảo toàn thay đổi sẵn có.
- **Smoke-test phụ thuộc display/OpenGL/audio:** ghi chính xác kết quả thực thi và phân biệt lỗi môi trường với lỗi gameplay; không báo PASS nếu lệnh hoặc exit code chưa xác nhận.

## 6. Tiêu chí nghiệm thu

- `CharacterController` nằm trong `glory_game`, chỉ phụ thuộc ActionMap/GLM và dữ liệu game; không gọi API render.
- Di chuyển forward/back/strafe theo yaw camera, tốc độ không tăng khi đi chéo; facingYaw xoay mượt và ổn định khi không có input.
- Jump chỉ bắt đầu khi grounded; trọng lực làm nhân vật đạt đỉnh rồi rơi về y=0, velocity.y=0 và grounded=true.
- ThirdPersonCamera orbit bằng relative mouse delta, theo target + offset ở khoảng cách đã cấu hình, pitch không lộn ngược; view/projection hữu hạn và cập nhật aspect ratio.
- Gameplay tích hợp truyền transform/camera đến renderer; Idle/Run và Attack được kích hoạt theo trạng thái/hành động; DebugPanel hiển thị player telemetry và mode.
- Build thành công; tất cả CTest hiện có và test mới PASS; `glory_client.exe --smoke-test` thoát mã 0 sau đúng 120 fixed ticks.
- `test-report.md` ghi lệnh và kết quả thật; `review.md` có đủ Phải sửa/Nên sửa/Gợi ý; `PLAN.md` chỉ được tick sau các điều kiện trên.
