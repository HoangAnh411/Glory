# Kế hoạch: Dodge và Capsule Collision

## 1. Mục tiêu

Hoàn thành hai mục kế tiếp liền nhau ở Giai đoạn 3: né đòn có cooldown/I-frame và va chạm capsule với mặt đất, vật cản, dốc cùng bậc thấp. Toàn bộ vận động và va chạm chạy bằng C++/GLM trong `glory_game`, không phụ thuộc SDL hay renderer.

## 2. Thiết kế

- Thêm action `Dodge`, mặc định gán Left Shift. SDL chỉ chuyển scancode sang `InputFrame`; `ActionMap` và controller xử lý state.
- `CharacterController` lưu hướng lướt, elapsed/duration, cooldown và `isInvulnerable`. Dodge đi theo vận tốc ngang hiện tại; nếu đứng yên dùng facing yaw. Thời gian mặc định 0.35s, speed 12 m/s, cooldown 0.8s, I-frame [0.05s, 0.28s].
- Thêm `collision.hpp` trong module game: cấu trúc capsule có base/radius/height, AABB và ramp; `CollisionWorld` chứa mặt đất y=0, AABB obstacles, ramp có góc kiểm tra qua normal.y, và AABB step. `CapsuleCollisionResolver` xử lý dịch chuyển theo bước nhỏ, đẩy capsule ra khỏi overlap theo tiếp tuyến gần nhất, trượt bằng cách thử các trục tiếp tuyến, dừng trên mặt dốc không đi được, bám mặt dốc đi được và thử nâng capsule tối đa 0.25m khi vướng step thấp.
- `CharacterController` sở hữu sample collision world mặc định; resolver cũng nhận world tường minh để test các tình huống biệt lập. Kết quả trả position, velocity đã giải quyết, grounded/contact và số contact trong lượt cập nhật.
- Mở rộng `PlayerFrameState` và DebugPanel với dodge state, cooldown, i-frame, contact count; không đưa logic vật lý vào engine.

## 3. Tasks tuần tự

1. Mở rộng `Action`/`Control`/SDL mapping cho Dodge và Left Shift; cập nhật test ActionMap.
2. Thêm `collision.hpp` cùng hình học AABB/ramp, world sample và resolver capsule độc lập.
3. Tích hợp collision resolution vào fixed update của controller; cập nhật nhảy, grounding và collision telemetry từ kết quả resolver.
4. Bổ sung state machine Dodge, hướng lướt, thời gian I-frame, cooldown và ưu tiên điều khiển khi đang dash.
5. Truyền telemetry thuần dữ liệu qua callback ứng dụng và trình bày Dodge/I-frame/contact trong DebugPanel.
6. Thêm CPU tests cho dodge timing/speed/cooldown và collisions: depenetration, wall slide, slope dưới 45°, slope từ 45° trở lên, step <= 0.25m.
7. Build, chạy đủ CTest và smoke-test 120 tick; sửa lỗi; ghi kết quả terminal vào `test-report.md`.
8. Review code theo Phải sửa/Nên sửa/Gợi ý, ghi `review.md`, xử lý mọi mục Phải sửa và chạy lại kiểm tra cần thiết.
9. Chỉ sau PASS, đánh dấu `[x]` đúng hai mục Dodge và Capsule collision trong `PLAN.md`.

## 4. File dự kiến tạo/sửa

- Tạo `game/include/glory/game/collision.hpp`.
- Sửa `game/include/glory/game/action_map.hpp`, `game/src/action_map.cpp`.
- Sửa `shared/include/glory/input/input_frame.hpp`, `engine/src/input_system.cpp`.
- Sửa `game/include/glory/game/character_controller.hpp`, `game/src/character_controller.cpp`.
- Sửa `engine/include/glory/engine/application.hpp`, `engine/src/application.cpp`, `engine/include/glory/engine/debug_panel.hpp`, `engine/src/debug_panel.cpp` cho telemetry hiển thị.
- Sửa `tests/input_mapping_tests.cpp`, `tests/character_controller_tests.cpp`; tạo `tests/collision_tests.cpp`.
- Sửa `CMakeLists.txt`, tạo `test-report.md` và `review.md`, rồi cập nhật hai checkbox trong `PLAN.md`.

## 5. Rủi ro và cách giảm thiểu

- Capsule/AABB xử lý sai khi tick dài có thể xuyên tường: phân đoạn displacement theo kích thước capsule và kiểm tra depenetration mỗi đoạn.
- Độ dốc/step dễ bị nhầm thành tường: dùng normal.y >= cos(45°) cho ramp đi được; chỉ thử step-up nếu mặt trên nằm trong stepHeight và capsule không còn overlap.
- Dodge và gravity cùng hoạt động: dash chỉ ghi đè vận tốc ngang; trục Y vẫn thuộc jump/gravity/collision.
- Cooldown không được khởi động lại khi nút giữ: chỉ chấp nhận `pressed`, và test trigger trước/sau khi cooldown hết.
- Diff trước đó có thay đổi của người dùng trong CMake/PLAN/application/main; chỉnh sửa phải giữ nguyên thay đổi sẵn có ngoài hai checkbox đã yêu cầu.

## 6. Tiêu chí nghiệm thu

- Dodge mặc định trên Left Shift; dash 12m/s trong 0.35s; dùng move vector hoặc facing khi đứng yên; I-frame đúng khoảng 0.05-0.28s; cooldown 0.8s ngăn tái kích hoạt.
- Controller sau collision không đi xuyên tường, được depenetrate, trượt dọc tường, bám ramp normal.y >= 0.707, bị chặn bởi ramp dốc hơn và leo step cao không quá 0.25m.
- Hiển thị dodging, cooldown, i-frame/contact count trong DebugPanel; logic game/physics không gọi SDL/OpenGL.
- Build thành công; toàn bộ 11 CTest cũ và test mới PASS; `glory_client.exe --smoke-test` kết thúc với exit code 0 sau đủ 120 fixed ticks.
- Có test report và review; hai mục roadmap chỉ tick khi build/test PASS và review không có mục Phải sửa.
