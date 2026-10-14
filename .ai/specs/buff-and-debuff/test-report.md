# Báo cáo kiểm thử: Buff và Debuff

## Lệnh đã chạy
- `& 'C:\Program Files\CMake\bin\cmake.exe' --build build` — PASS (exit code 0).
- `& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure` — PASS (14/14 test).
- `.\build\glory_client.exe --smoke-test` — PASS (khởi tạo SDL3/OpenGL; hoàn tất 120 fixed tick).

## Phạm vi kiểm tra
- Bleed gây sát thương và Regen hồi máu đúng chu kỳ — PASS.
- Speed Boost/Slow kết hợp đúng; hệ số sát thương gây ra và nhận vào chính xác — PASS.
- Stun khóa hành động trong thời hạn và tự hết hiệu lực — PASS.
- Áp dụng lại hiệu ứng tăng stack có giới hạn, làm mới thời gian; hiệu ứng hết hạn được xóa — PASS.
- Skills JSON nạp Stun và Bleed; combo trúng mục tiêu gây sát thương đã nhân hệ số và gắn Bleed — PASS.
- 13 CTest có sẵn — PASS; test mới `glory_status_effect` — PASS.

## Kết quả
Build, CTest và smoke test đều PASS. Build có các cảnh báo cấu hình/deprecation từ dependency hiện có, không có lỗi biên dịch.
