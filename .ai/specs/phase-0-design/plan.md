# Kế Hoạch Giai Đoạn 0: Thiết Kế và Khởi Tạo Dự Án

## 1. Mục Tiêu

Hoàn tất bảy đầu việc của Giai đoạn 0 trong `PLAN.md`: chốt stack kỹ thuật, ghi nhận cách chuẩn bị môi trường, tạo cấu trúc repo có thể build/chạy, viết GDD, xác định class và năm skill đầu tiên, giới hạn vertical slice, và đặt tên riêng cho thế giới. Giữ project thuần C++20/CMake, không thêm engine hay dependency nhị phân ở giai đoạn này. Logic gameplay và phần render có thư mục riêng.

Các quyết định nội dung được xem là baseline thiết kế để một người có thể bắt đầu vertical slice; các hệ thống gameplay/render thật vẫn thuộc giai đoạn sau.

## 2. Danh Sách Task

1. Ghi quyết định công nghệ: C++20, SDL3, OpenGL 4.5, GLM, Dear ImGui; các thư viện cgltf, stb_image, EnTT, miniaudio, ENet và PostgreSQL/SQLite chỉ được đưa vào khi có nhu cầu ở giai đoạn phù hợp.
2. Tạo hướng dẫn kiểm tra/cài đặt compiler C++20, CMake, Git và RenderDoc cho Windows, ghi lệnh kiểm chứng và đánh dấu rõ đây là hướng dẫn, không tuyên bố đã cài đặt phần mềm trên máy.
3. Tạo cấu trúc `engine`, `game`, `server`, `assets`, `docs`; thêm một bootstrap C++20 không phụ thuộc thư viện ngoài để xác nhận CMake build và chương trình chạy được. Không khởi tạo SDL/OpenGL trong bước này.
4. Viết GDD ngắn bằng tiếng Việt: bối cảnh, fantasy người chơi, vòng lặp, chiến đấu, tiến trình và điểm khác biệt.
5. Chốt class mở đầu `Kiếm Vũ` với đúng năm skill có vai trò và chức năng riêng, không dựa theo tên/nội dung nhân vật của Vinh Diệu.
6. Chốt vertical slice: một map huấn luyện/chiến đấu, một class người chơi, một boss, một đấu trường PvP nhỏ; nêu rõ nội dung không nằm trong slice.
7. Chọn tên thế giới/IP nguyên bản `Lam Vực`, áp dụng thống nhất trong GDD và tài liệu thiết kế.

## 3. File Cần Tạo / Sửa

- `PLAN.md` — chỉ tick bảy việc Giai đoạn 0 sau khi Tester PASS và Reviewer không có mục Phải sửa.
- `.ai/specs/phase-0-design/plan.md` — kế hoạch này.
- `.ai/specs/phase-0-design/test-report.md` — kết quả chạy build, smoke executable và đối chiếu tài liệu.
- `.ai/specs/phase-0-design/review.md` — rà soát độc lập theo kế hoạch và nguyên tắc gốc.
- `CMakeLists.txt` — cấu hình CMake tối thiểu, target bootstrap C++20.
- `game/src/main.cpp` — chương trình bootstrap độc lập, không chứa logic game hay render.
- `engine/.gitkeep`, `game/include/.gitkeep`, `server/.gitkeep`, `assets/.gitkeep` — giữ các module rỗng trong Git.
- `docs/TECHNICAL_DECISIONS.md` — stack đã chốt và ranh giới module/dependency.
- `docs/ENVIRONMENT.md` — hướng dẫn cài/kiểm tra công cụ phát triển trên Windows.
- `docs/GDD.md` — game design document và phạm vi vertical slice.

Không sửa vai trò workflow hoặc các tài liệu thuộc tính năng khác.

## 4. Rủi Ro và Giảm Thiểu

- **Công cụ chưa cài hoặc không có trong PATH:** ghi lệnh xác minh và nguồn cài đặt chính thức trong hướng dẫn; Tester phải báo chính xác công cụ nào thiếu, không giả lập kết quả.
- **Thiết kế bị hiểu nhầm là nội dung đã triển khai:** ghi rõ GDD là mục tiêu sản phẩm; bootstrap chỉ chứng minh cấu hình CMake/C++ chạy được.
- **Phình phạm vi:** giữ SDL3/OpenGL và gameplay implementation ngoài bootstrap; chỉ thêm dependency khi bước roadmap cần đến.
- **Ranh giới client/server mờ:** dùng thư mục riêng cho engine, game, server; phần logic game không phụ thuộc renderer.

## 5. Tiêu Chí Chấp Nhận

1. `docs/TECHNICAL_DECISIONS.md` chốt đúng C++20, SDL3, OpenGL 4.5, GLM, Dear ImGui và giải thích thời điểm dùng các dependency còn lại.
2. `docs/ENVIRONMENT.md` hướng dẫn kiểm tra compiler C++20, CMake, Git, RenderDoc bằng lệnh cụ thể, không tuyên bố trạng thái cài đặt khi chưa xác minh.
3. Các thư mục `engine`, `game`, `server`, `assets`, `docs` có mặt; `game/src/main.cpp` biên dịch bằng CMake mà không cần dependency ngoài.
4. `cmake -S . -B build` và `cmake --build build` kết thúc mã thoát 0; chạy executable bootstrap in ra thông báo sẵn sàng.
5. `docs/GDD.md` có bối cảnh, vòng lặp chơi, điểm khác biệt, class Kiếm Vũ với đúng năm skill, và phạm vi slice gồm 1 map, 1 class, 1 boss, 1 đấu trường.
6. Tên thế giới `Lam Vực` được dùng thống nhất; nội dung và tên riêng độc lập, không dùng tên/nhân vật Vinh Diệu.
7. Giải thích và tài liệu cho người dùng bằng tiếng Việt; tên code bằng tiếng Anh; logic và render được tách ranh giới ngay từ cấu trúc.
