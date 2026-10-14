# Review: Lag compensation

## 1. Phải sửa

- Không phát hiện lỗi chặn. History giới hạn 61 frame; target tick bị giới hạn trong 21 tick/0.35 giây; tick tương lai không vượt server hiện tại. Combat nhận hurtbox lịch sử chỉ cho phép kiểm tra hit, còn HP và transform hiện tại vẫn authoritative.
- `targetTick` được gửi trong input và server bỏ qua sequence trùng/cũ để gói cũ không thay lại thời điểm rewind.

## 2. Nên sửa

- Server hiện chọn một player gần nhất theo vị trí lịch sử làm mục tiêu cho `CombatSystem`, phù hợp với đường combat đơn mục tiêu hiện tại. Trước khi mở rộng đấu 3v3, nên bổ sung danh sách mục tiêu/hit event để một hitbox có thể kiểm tra nhiều hurtbox lịch sử và chọn theo vùng va chạm.
- Client cung cấp `targetTick` và server chỉ kẹp theo cửa sổ cho phép. Khi có handshake/session clock ổn định, có thể đối chiếu tick với snapshot gần nhất server đã phát cho client để tăng khả năng phát hiện timestamp giả.

## 3. Gợi ý

- Thêm metric về tick yêu cầu, tick sau clamp và tỷ lệ hit được xác nhận sau rewind để cân chỉnh giới hạn 0.35 giây.
- Mở rộng lịch sử cho hurtbox nhiều phần khi hệ thống combat có hitbox theo vùng cơ thể.

## Kết luận

Đạt tiêu chí của plan; các điểm mở rộng multi-target và xác thực tick được ghi nhận cho các bước netcode tiếp theo.
