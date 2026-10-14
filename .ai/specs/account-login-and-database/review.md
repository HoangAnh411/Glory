# Review: Tài khoản, đăng nhập và database

## 1. Phải sửa (Critical / Required)

- Không còn lỗi bắt buộc sửa. Các hạng mục trong plan đã được đối chiếu: SQLite schema và prepared statements, PBKDF2-HMAC-SHA256 với salt từ CSPRNG, so sánh hash constant-time, token ngẫu nhiên, kiểm tra session/quyền sở hữu, giới hạn codec và UI, build/test pass.

## 2. Nên sửa (Recommended)

- Nên bổ sung test đọc trực tiếp file SQLite để khẳng định giá trị `password_hash`/`salt` không chứa password ban đầu và kiểm tra các cột schema; test hiện xác nhận gián tiếp qua đăng nhập và hành vi database.
- Nên cân nhắc giới hạn tốc độ đăng nhập/đăng ký và thời hạn thu hồi session khi hệ thống có nhiều người chơi; hiện session sống đến khi tiến trình server dừng.

## 3. Gợi ý (Suggestions)

- ENet hiện không mã hóa payload; password trong Register/Login packet có thể bị quan sát trên đường truyền. Trước khi cho phép kết nối qua mạng không tin cậy, bổ sung transport bảo mật có xác thực (ví dụ DTLS/TLS) và kiểm thử handshake.
- Nên thay giá trị class tự do bằng danh sách class hợp lệ do server quản lý khi có hệ thống class trong gameplay.
- Việc chọn nhân vật hiện được server xác thực và gắn với kết nối; bước kế tiếp có thể nạp tiến trình nhân vật vào simulation và lưu thay đổi level/EXP.
