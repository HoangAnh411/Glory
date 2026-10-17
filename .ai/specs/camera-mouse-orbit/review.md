# Reviewer

## Phải sửa

- Không còn lỗi bắt buộc sau vòng build, test và camera-test cuối.

## Nên sửa

- Có thể tách camera-test thành binary kiểm chứng riêng để giảm thời gian render khi chạy CI.
- Có thể bổ sung test tương tác SDL tự động cho Alt/F3/Esc/focus; đường chạy hiện tại kiểm tra logic runtime nhưng không mô phỏng đầy đủ thao tác cửa sổ thật.

## Gợi ý

- Giữ sáu JSON/PNG camera-test làm artifact hồi quy cho các thay đổi camera sau này.

## Kết luận

Code khớp kế hoạch: relative mouse delta không nhân dt, bỏ delta khi bắt lại, orbit zoom mượt, auto-return opt-in và lưu settings, spring arm không slide ngang, không ép camera vào collider, fade nhân vật khi arm dưới 2 m, và runtime camera-test kiểm tra marker/NDC/arm/collider/FPS.

Build, 37 CTest, smoke và `--camera-test` đều PASS. `PLAN.md` được giữ nguyên vì chưa có baseline hình học cũ chạy cùng telemetry.
