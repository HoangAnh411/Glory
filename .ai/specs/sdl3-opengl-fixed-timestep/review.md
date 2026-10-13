# Báo Cáo Review — SDL3/OpenGL và Fixed Timestep

**Ngày:** 2026-10-13  
**Phạm vi:** Đối chiếu `.ai/specs/sdl3-opengl-fixed-timestep/plan.md`, code, tài liệu và nguyên tắc trong `PLAN.md`.

## 1. Phải sửa

Không có.

## 2. Nên sửa

- Pin SDL3 bằng commit SHA hoặc archive có checksum thay vì chỉ tag Git để tránh tag bị di chuyển làm thay đổi dependency ở lần configure sau.
- Phiên chạy `Start-Process` tách khỏi pseudo-console không hoàn tất smoke window trong 7 giây và không để Tester liệt kê được HWND SDL; chạy smoke trực tiếp trong terminal với Windows video driver hoàn tất. Nên xác nhận cửa sổ tương tác lâu dài từ desktop session thông thường.
- Khi project thêm renderer/shader ở đầu việc kế tiếp, dùng OpenGL function loader cho API sau OpenGL 1.1; hiện tại `glViewport`, `glClearColor` và `glClear` dùng được từ OpenGL import library trên Windows.

## 3. Gợi ý

- Mở rộng test scheduler với nhiều nhịp frame khác nhau (frame time không đều) để theo dõi độ lệch accumulator trong thời gian dài.
- Khi thêm renderer, chuyển phần clear/frame presentation khỏi application loop sang một interface renderer riêng; simulation hiện không phụ thuộc SDL/OpenGL như yêu cầu kiến trúc.

## Kết Luận

Đạt kế hoạch và các tiêu chí nghiệm thu. SDL3 tạo được OpenGL 4.5 Core context; cửa sổ hiển thị được gọi tường minh, fixed-step loop gọi simulation qua callback với bước 1/60 giây và giới hạn catch-up; CTest và smoke run trực tiếp đều PASS. `glory_game` không link SDL/OpenGL, SDL/window lifecycle nằm trong `glory_engine`. Chỉ hai mục đầu Giai đoạn 1 thuộc phạm vi thay đổi; không thấy lỗi bắt buộc phải sửa.
