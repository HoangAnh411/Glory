# Kế Hoạch: SDL3/OpenGL và Fixed Timestep

## 1. Mục Tiêu

Hoàn thành hai mục đầu tiên còn mở của Giai đoạn 1: mở cửa sổ SDL3 với OpenGL 4.5 Core context và chạy game loop với fixed timestep. Giữ simulation trong `game/` độc lập với SDL/OpenGL; `engine/` sở hữu platform, context, event loop và nhịp cập nhật. Cửa sổ hiển thị màu nền để xác nhận context/render target hoạt động; chưa vẽ triangle hay thêm input remapping.

SDL3 được lấy ở phiên bản cố định 3.2.10 bằng CMake FetchContent để cấu hình build tái lập mà không cần cài thư viện thủ công. Toolchain build dùng CMake, Ninja và LLVM-MinGW Clang++ theo `AGENTS.md`.

## 2. Danh Sách Task

1. Mở rộng CMake để tải/build SDL3 3.2.10 static, tìm OpenGL hệ thống, tạo executable `glory_client`, thư viện logic `glory_game` và test fixed-step.
2. Tạo lớp cửa sổ trong `engine/`: khởi tạo SDL video, yêu cầu OpenGL 4.5 Core, tạo/make-current context, xử lý lỗi có thông báo, chạy event pump, clear/swap buffer và giải phóng tài nguyên theo đúng thứ tự.
3. Tạo `FixedTimestep` trong `engine/` với bước mặc định 1/60 giây, giới hạn frame delta và số bước bù tối đa để tránh spiral-of-death; mọi update gameplay nhận cùng một delta cố định.
4. Tạo `game::Simulation` thuần C++ (không include SDL/OpenGL) với bộ đếm tick/thời gian mô phỏng làm ví dụ logic độc lập; nối callback từ application loop sang simulation.
5. Thêm `--smoke-test`: tạo cửa sổ/context hiển thị, chạy đúng 120 tick fixed-step rồi tự đóng và thoát mã 0; chế độ thường mở cửa sổ hiển thị cho đến khi nhận yêu cầu đóng hoặc Escape.
6. Thêm CTest cho số tick, accumulator phần dư, delta âm/giới hạn frame và giới hạn catch-up; cập nhật `docs/ENVIRONMENT.md` cho dependency fetch, configure/build/test và hai cách chạy executable.
7. Chạy CMake configure/build bằng Ninja + LLVM-MinGW Clang++, CTest và smoke executable; ghi output/exit code thật vào `test-report.md`.

## 3. File Cần Tạo / Sửa

- `CMakeLists.txt` — SDL3 FetchContent, OpenGL, targets, CTest.
- `engine/include/glory/engine/fixed_timestep.hpp` — scheduler fixed timestep độc lập SDL.
- `engine/include/glory/engine/application.hpp` — cấu hình/chữ ký application loop.
- `engine/src/application.cpp` — SDL3 window/context, events, timing, clear/swap, smoke mode.
- `game/include/glory/game/simulation.hpp` — API simulation thuần C++.
- `game/src/simulation.cpp` — cập nhật tick/thời gian mô phỏng.
- `game/src/main.cpp` — entry point, callback wiring và parse `--smoke-test`.
- `tests/fixed_timestep_tests.cpp` — kiểm tra logic scheduler không cần SDL/window.
- `docs/ENVIRONMENT.md` — hướng dẫn build/test/chạy mới.
- `PLAN.md` — chỉ tick hai mục đầu Giai đoạn 1 sau khi test PASS và review không còn Phải sửa.
- `.ai/specs/sdl3-opengl-fixed-timestep/test-report.md` và `review.md` — báo cáo hai vai trò cuối.

## 4. Rủi Ro và Giảm Thiểu

- **FetchContent cần mạng ở lần configure đầu:** pin tag SDL3 và để build tái sử dụng source trong build tree; báo lỗi tải thật nếu mạng không sẵn sàng.
- **Máy/driver không hỗ trợ OpenGL 4.5:** context creation phải fail rõ ràng và trả exit code khác 0; không hạ phiên bản âm thầm.
- **SDL3 API khác SDL2:** chỉ dùng SDL3 headers/events/API và xác minh bằng build thực tế.
- **Frame bị treo hoặc giật tạo catch-up quá lớn:** clamp frame delta và cap số bước/tick mỗi frame; giữ lại phần lẻ accumulator sau khi bỏ phần backlog nguyên bước.
- **Smoke chạy trên máy không có display:** test scheduler thuần C++ vẫn chạy headless; smoke context cần desktop/video driver thật và nếu thất bại sẽ được ghi là không đạt thay vì giả lập PASS.
- **Thay đổi bootstrap cũ:** executable chính chuyển thành `glory_client`; cập nhật tài liệu chạy để không còn hướng dẫn chạy bootstrap cũ.

## 5. Tiêu Chí Chấp Nhận

1. CMake configure/build thành công với CMake 4.4.4, Ninja và LLVM-MinGW Clang++ theo toolchain trong `AGENTS.md`; SDL3 3.2.10 được pin rõ ràng.
2. Chạy `glory_client` tạo cửa sổ SDL3, có OpenGL 4.5 Core context hiện hành; lỗi init/context trả về thông báo SDL và exit code khác 0; đóng cửa sổ/Escape dọn context/window/SDL đúng thứ tự.
3. `game/` build thành thư viện logic không link/include SDL hoặc OpenGL; engine gọi simulation qua callback fixed-step.
4. Scheduler dùng bước cố định 1/60 giây, giới hạn catch-up, không gọi update khi accumulator chưa đủ bước và gọi đúng số update khi đủ thời gian.
5. CTest pass cho các trường hợp tick, remainder, delta âm/clamp và cap; `glory_client --smoke-test` hiện cửa sổ, tạo context 4.5 Core, chạy đủ 120 tick rồi thoát 0.
6. `docs/ENVIRONMENT.md` phản ánh lệnh build/test/run chính xác; hai mục đã làm trong Giai đoạn 1 được tick, các mục sau vẫn `[ ]`.
