# Báo cáo kiểm thử: Tài khoản, đăng nhập và database

## Lệnh đã thực thi

- `cmake -S . -B build -G Ninja` — exit code 0.
- `cmake --build build` — exit code 0 (`ninja: no work to do` ở lượt xác nhận cuối; toàn bộ target đã build thành công ngay trước đó).
- `ctest --test-dir build --output-on-failure` — exit code 0.
- `$dbPath = Join-Path (Get-Location) 'build/accounts-smoke-reviewed.sqlite3'; .\build\glory_server.exe --smoke-test --port 17779 --database $dbPath` — exit code 0.
- `.\build\glory_client.exe --smoke-test` — exit code 0.

## Kết quả tổng quan

**PASS** — 26/26 CTest passed; build và smoke test server/client đều thành công.

## Chi tiết test case

- Khởi tạo SQLite `:memory:` và schema; đăng ký tài khoản — Passed.
- Từ chối username trùng, credential ngắn; password đúng/sai và account không tồn tại — Passed.
- Session token 256-bit, từ chối session không hợp lệ; tạo nhân vật với level/exp mặc định, tên trùng, lọc theo account và kiểm tra quyền sở hữu khi chọn vào world — Passed.
- Round-trip codec cho Register/Login/CharacterList/CreateCharacter/EnterWorld; từ chối request đăng ký bị cắt cụt — Passed.
- Toàn bộ suite hồi quy — 26/26 Passed.
- Server headless chạy đủ 120 tick ở 60 Hz với database SQLite file — Passed.
- Client khởi tạo SDL3/OpenGL, audio và chạy đủ 120 fixed tick — Passed.

## Ghi chú

- Lượt build đầu tiên bắt lỗi tên kiểu chưa qualified trong client response handler; đã sửa và build lại thành công.
- CMake có cảnh báo deprecation từ cấu hình FetchContent `miniaudio`/`imgui` hiện có và cảnh báo compatibility từ GLM; không ảnh hưởng cấu hình/build.
