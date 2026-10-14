# Review: Client prediction và reconciliation

## 1. Phải sửa

- Không phát hiện lỗi chặn. Input được sequence, server ACK theo player, client lưu/replay input chưa ACK, khôi phục trạng thái di chuyển authoritative gồm grounded/dodge/knockback và chỉ correction khi sai lệch vượt ngưỡng. Các tiêu chí được build/test xác nhận.

## 2. Nên sửa

- Buffer input giới hạn 256 frame (xấp xỉ 4.3 giây ở 60 Hz). Nếu mất snapshot/ACK lâu hơn, frame cũ có thể bị loại trước khi ACK đến; nên bổ sung thống kê overflow hoặc chính sách phục hồi rõ ràng khi triển khai session mạng dài hạn.
- Snapshot schema hiện ở version 2 và decoder từ chối schema cũ. Khi phát hành client/server độc lập, nên thêm thương lượng phiên bản hoặc thông báo lỗi tương thích rõ trong handshake.

## 3. Gợi ý

- Prediction hiện chạy `CharacterController`; combat vẫn authoritative và không speculative. Có thể bổ sung dự đoán phản hồi animation/combat sau khi có cơ chế rollback trạng thái `CombatSystem`.
- Theo dõi phân bố misprediction offset và tần suất reconciliation trong debug telemetry để tinh chỉnh ngưỡng 0.05 m theo ping thực tế.

## Kết luận

Đạt tiêu chí của plan. Không có mục bắt buộc phải sửa; có thể đánh dấu hoàn tất trong `PLAN.md`.
