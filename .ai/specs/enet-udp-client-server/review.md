# Review: ENet UDP Client/Server

## 1. Phải sửa

Không có mục bắt buộc phải sửa. Các tiêu chí trong plan đã được đối chiếu với source và kiểm chứng bằng build, loopback test, CTest và smoke test.

## 2. Nên sửa

- Phiên bản đầu tiên chuyển input của mọi client vào cùng một player simulation. Khi có nhiều người chơi, cần lưu input/session theo ClientId và mô phỏng entity riêng cho từng client.
- `ServerSnapshot` hiện mang tick counter để xác nhận đường truyền; khi triển khai snapshot/interpolation ở mục roadmap kế tiếp, cần định nghĩa schema có transform và trạng thái gameplay, kèm versioning.

## 3. Gợi ý

- Thêm xác thực/rate limit cho ConnectRequest, giới hạn số client cấu hình được và cơ chế timeout kết nối ở lớp session.
- Mở rộng loopback test với packet malformed, nhiều client và kiểm tra giá trị RTT trong biên thời gian.
- Hỗ trợ cú pháp địa chỉ IPv6 `[host]:port` nếu cần ngoài mạng IPv4 hiện tại.

## Kết luận

Target mạng tách biệt khỏi `glory_game` và `glory_engine`; ENet host/context có ownership rõ, gói tin bị giới hạn kích thước và type được kiểm tra. Không phát hiện lỗi cản trở nghiệm thu bước UDP cơ bản.
