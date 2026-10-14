# Headless Dedicated Server Plan

## 1. Mục tiêu

Tạo executable `glory_server` chạy logic game không cần engine đồ họa. Server nạp skills/world JSON, cập nhật controller, va chạm, combat, boss và trigger với timestep cố định 60 Hz; hỗ trợ `--smoke-test`, `--ticks N` và chạy liên tục đến Ctrl+C.

## 2. Tasks

1. Thêm `ServerSimulation` thuần game trong `server/`, sở hữu ActionMap, CharacterController, Boss, CombatSystem, WorldEncounter, status và bộ đếm tick/thời gian mô phỏng; mỗi `tick` dùng đúng 1/60 giây.
2. Tạo `server/src/main.cpp` parse CLI, nạp assets theo `GLORY_ASSET_DIR`, chạy vòng lặp `std::chrono::steady_clock` cố định 60 Hz, hỗ trợ tín hiệu dừng và in snapshot trạng thái khi chạy hữu hạn.
3. Đăng ký executable `glory_server` chỉ link `glory_game`; biên dịch `server_simulation.cpp` trực tiếp vào executable. Thêm CTest executable cho logic server bằng cách biên dịch cùng source và link duy nhất `glory_game`.
4. Thêm test server tick count/elapsed, collision với sample world wall, combat damage/hit event không có render context.
5. Configure/build, chạy 19 CTest, `glory_server --smoke-test`, `glory_client --smoke-test`; ghi report, review ranh giới link và tick roadmap sau PASS.

## 3. Files cần sửa / tạo

- `server/include/glory/server/server_simulation.hpp` (mới)
- `server/src/server_simulation.cpp` (mới)
- `server/src/main.cpp` (mới)
- `tests/server_simulation_tests.cpp` (mới)
- `CMakeLists.txt`
- `.ai/specs/headless-dedicated-server/test-report.md` (Tester)
- `.ai/specs/headless-dedicated-server/review.md` (Reviewer)
- `PLAN.md` (chỉ mục server headless sau PASS)

## 4. Rủi ro & Giảm thiểu

- Server vô tình kéo đồ họa qua dependency: target executable và test link trực tiếp duy nhất `glory_game`; kiểm tra link graph/source includes.
- Scheduler trôi nhịp: dùng `steady_clock` và deadline tăng tuần tự theo 1/60 giây; mô phỏng luôn nhận timestep hằng số, không phụ thuộc thời gian frame.
- Test real-time có dao động hệ điều hành: kiểm tra tick count và tổng simulation time từ timestep cố định, đồng thời kiểm tra scheduler dùng clock monotonic mà không đặt ngưỡng wall-clock mong manh.
- Logic combat phụ thuộc vị trí fixture: test đặt combat target gần hitbox qua public session state, xác nhận hit/damage.
- Smoke test tốn khoảng 2 giây theo yêu cầu 120 tick ở 60 Hz.

## 5. Tiêu chí chấp nhận

- `glory_server` target link interface chỉ tới `glory_game`, không link engine/SDL/OpenGL/ImGui/miniaudio.
- Server load `world_spawns.json` và `skills.json`, mô phỏng các state game headless.
- `--smoke-test` chạy đúng 120 tick, báo cáo trạng thái, exit 0; `--ticks N` chạy đúng N tick; CLI sai trả mã khác 0; mặc định dừng an toàn khi nhận Ctrl+C.
- Test xác nhận timestep 1/60, tick/time chính xác, collision server-side và combat damage/hit event không cần render context.
- CMake configure/build PASS, CTest tổng 19/19 PASS, server và client smoke test đều PASS.
- Không thêm SDL/OpenGL vào server/game logic; giải thích tiếng Việt, identifier/comment kỹ thuật tiếng Anh.
