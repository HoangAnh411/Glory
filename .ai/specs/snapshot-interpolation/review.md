# Review: Snapshot và nội suy

## 1. Phải sửa

- Không phát hiện lỗi chặn. Các tiêu chí về buffer, thứ tự tick, nội suy yaw/vị trí, extrapolation giới hạn và cleanup đều được bao phủ bởi test; build và smoke test đều PASS.

## 2. Nên sửa

- `glory_engine` tạo một `ModelRenderer` riêng cho mỗi remote player. Cách này đơn giản và cho phép animation độc lập, nhưng tăng đáng kể tài nguyên GPU/CPU khi số người chơi tăng. Nên chuyển dần sang chia sẻ model/mesh và chỉ giữ animation instance riêng.

## 3. Gợi ý

- Ước lượng render tick hiện dựa trên tick 60 Hz và thời gian nhận snapshot từ đồng hồ monotonic phía client. Khi triển khai đồng bộ clock/độ trễ tốt hơn, có thể ánh xạ trực tiếp timestamp server để giảm sai số jitter.
- Bổ sung test cho player mới xuất hiện giữa hai snapshot và animation state thay đổi tại thời điểm nội suy.

## Kết luận

Đạt tiêu chí của plan và có thể đánh dấu hoàn tất trong `PLAN.md`.
