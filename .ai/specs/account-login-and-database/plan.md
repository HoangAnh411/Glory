# Kế hoạch: Tài khoản, đăng nhập và database

## 1. Mục tiêu

Thêm database SQLite bền vững và dịch vụ xác thực tài khoản/nhân vật chạy độc lập với renderer. Mở rộng giao thức ENet để đăng ký, đăng nhập, lấy danh sách và tạo nhân vật; cung cấp thao tác tương ứng trong DebugPanel. Mật khẩu chỉ lưu dưới dạng PBKDF2-HMAC-SHA256 với salt ngẫu nhiên.

## 2. Danh sách task nhỏ

1. Tích hợp SQLite3 và backend PBKDF2 CNG/OpenSSL vào CMake, tạo target `glory_db` dùng chung cho server và test.
2. Cài đặt `AccountDatabase`: schema accounts/characters, validation, truy vấn prepared statement, salt bảo mật, password hash, token phiên ngẫu nhiên và các kết quả nghiệp vụ có kiểu.
3. Mở rộng `glory_network` với packet types, cấu trúc request/response và codec binary có kiểm tra kích thước/string/enum; thêm request xác nhận nhân vật vào world.
4. Đưa service vào server: khởi tạo DB file cấu hình được, xử lý đăng ký/login/list/create/select trên kết nối ENet, xác thực token và quyền sở hữu; giới hạn danh sách 16 nhân vật để vừa packet.
5. Thêm trạng thái/callback UI account trong `ApplicationConfig`, xử lý response ở client và form Register/Login/Character List/Create/Enter trong DebugPanel.
6. Viết `tests/account_database_tests.cpp`, đăng ký CTest; bao gồm schema, đăng ký trùng, credential đúng/sai, phân quyền danh sách nhân vật và round-trip codec.
7. Build, chạy toàn bộ CTest, smoke test server và client theo môi trường khả dụng; sửa lỗi trong tối đa 3 vòng Tester. Reviewer rà soát plan, logic và bảo mật; sau khi đạt mới tick roadmap.

## 3. File cần sửa / tạo mới

- `CMakeLists.txt`
- `network/include/glory/network/network.hpp`
- `network/src/network.cpp`
- `server/include/glory/server/account_database.hpp`
- `server/src/account_database.cpp`
- `server/src/main.cpp`
- `engine/include/glory/engine/application.hpp`
- `engine/src/application.cpp`
- `engine/include/glory/engine/debug_panel.hpp`
- `engine/src/debug_panel.cpp`
- `game/src/main.cpp`
- `tests/account_database_tests.cpp`
- `PLAN.md` (chỉ tick đúng mục sau khi Tester PASS và Reviewer không còn mục bắt buộc sửa)
- `.ai/specs/account-login-and-database/{plan.md,test-report.md,review.md}`

## 4. Rủi ro tiềm ẩn

- CNG PBKDF2 và OpenSSL khác API; cô lập triển khai theo nền tảng và link thư viện phù hợp.
- SQLite fetch/build có thể chịu ảnh hưởng mạng hoặc toolchain; khóa phiên bản, ưu tiên package SQLite có target CMake ổn định và không đổi các dependency hiện tại.
- Thay đổi codec cần giữ nguyên số/giá trị packet hiện có; thêm giá trị sau `MatchmakingRequest`, giới hạn packet tối đa hiện tại.
- DB file không được ghi đè hoặc tạo trong thư mục build/source không chủ định; server nhận `--database`, mặc định `data/accounts.sqlite3`, tự tạo thư mục cha.
- Không trả lộ thông tin hash/salt; so sánh hash constant-time; token là random 256-bit và không dùng account id thay token.
- ENet handshake hiện tạo gameplay player ngay khi kết nối; account là flow bổ sung tương thích. Các packet tài khoản vẫn phải kiểm tra session token trước khi thao tác nhân vật.
- UI dùng cùng event loop/network client hiện có; phản hồi cần cập nhật trạng thái mà không chặn render thread.

## 5. Tiêu chí chấp nhận

- `accounts` và `characters` được tạo đúng schema, `:memory:` hoạt động; truy vấn dùng prepared statements.
- Username 3–24 ký tự ASCII chữ/số/underscore; password 8–128 byte; character name 3–24 ký tự; lỗi length/trùng có kết quả phân biệt.
- Database không lưu plain-text password; salt ngẫu nhiên tối thiểu 16 byte; PBKDF2-HMAC-SHA256 có số vòng lặp cao; login sai không tạo session.
- Đăng nhập thành công cấp token 256-bit ngẫu nhiên; list/create yêu cầu token hợp lệ và list chỉ trả nhân vật đúng account.
- Các packet Register/Login/CharacterList/CreateCharacter encode/decode round-trip và từ chối payload thiếu, thừa hoặc vượt giới hạn.
- Enter World xác nhận session và quyền sở hữu nhân vật phía server trước khi chọn nhân vật cho kết nối.
- Server nhận đường dẫn DB từ CLI, xử lý Auth packets qua ENet, và vẫn build/run được headless.
- DebugPanel có form đăng ký/đăng nhập, trạng thái thông báo, danh sách tên/level/class, tạo nhân vật và chọn vào world.
- `cmake -S . -B build -G Ninja`, `cmake --build build`, 26 CTest, server smoke test và client smoke test hoàn tất thành công.
- Có `test-report.md`, `review.md`; roadmap chỉ được tick sau các điều kiện pass.
