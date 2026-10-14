# Báo cáo REVIEWER: Chống gian lận phía server

## 1. Phải sửa

- Không phát hiện lỗi bắt buộc phải sửa. Tracker chạy trong server, tọa độ client probe không được áp dụng làm trạng thái authoritative, yêu cầu skill được kiểm tra trước khi đưa vào combat, và policy kick gửi cảnh báo rồi đóng peer.

## 2. Nên sửa

- Policy hiện đếm vi phạm tích lũy theo session; ngưỡng 8 có thể kick cả các vi phạm rời rạc cách xa nhau. Khi có dữ liệu latency thực tế, cân nhắc cửa sổ thời gian hoặc mức phạt giảm dần để cân bằng false positive.
- Tracker dùng cùng `CollisionWorld` mẫu hiện tại. Khi server nạp collision theo map, nên inject collision world của zone vào tracker để kiểm tra đúng vật cản đang chạy.

## 3. Gợi ý

- Mở rộng telemetry theo class/zone và bổ sung log server có cấu trúc để điều tra hành vi trước khi kick.
- Theo dõi telemetry và tỷ lệ false positive trong thử nghiệm nhiều client trước khi chọn ngưỡng kick cho production.

## Kết luận

Đạt plan. Build, 33/33 CTest và smoke test server/client đều PASS. Kiến trúc kiểm tra nằm phía server/game, không phụ thuộc renderer; tên kỹ thuật dùng tiếng Anh và roadmap chưa được tick trước bước này.
