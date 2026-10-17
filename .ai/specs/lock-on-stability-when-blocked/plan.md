# Lock-On Stability When Target Is Blocked

## 1. Objective

Giữ lock-on ổn định khi boss bị kẹt hoặc nằm trong vật thể. Kiểm tra nhìn thấy vẫn được dùng khi chọn mục tiêu ban đầu, nhưng không được tự hủy lock đang hoạt động chỉ vì collider tạm thời che tia; lock chỉ hủy khi mục tiêu chết hoặc vượt quá 30 m.

## 2. Tasks

1. Tách điều kiện duy trì lock khỏi điều kiện chọn lock trong module targeting.
2. Dùng điều kiện duy trì lock trong fixed update của gameplay.
3. Thêm test hồi quy với AABB chắn tia: chọn mới bị từ chối, nhưng lock đã có vẫn được giữ khi mục tiêu còn sống và trong 30 m.
4. Build, chạy focused test, toàn bộ CTest, smoke test và combat test.
5. Review thay đổi và ghi báo cáo.

## 3. Files to touch

- `game/include/glory/game/targeting.hpp`: khai báo điều kiện duy trì lock.
- `game/src/targeting.cpp`: cài đặt kiểm tra alive/range hữu hạn, không phụ thuộc line-of-sight.
- `game/src/main.cpp`: dùng điều kiện duy trì lock, bỏ hủy lock do LOS trong fixed update.
- `tests/targeting_tests.cpp`: thêm regression test cho mục tiêu bị collider che.
- `.ai/specs/lock-on-stability-when-blocked/test-report.md`: kết quả kiểm thử thật.
- `.ai/specs/lock-on-stability-when-blocked/review.md`: kết quả review.

## 4. Risks & Mitigations

- Lock có thể tồn tại khi boss tạm thời khuất: chỉ áp dụng sau khi lock đã được chọn hợp lệ; chọn mới vẫn yêu cầu LOS.
- Lock có thể tồn tại quá lâu: vẫn hủy ngay khi boss chết hoặc khoảng cách ngang vượt 30 m.
- Tấn công bị vật thể chặn: không thay đổi attack targeting hoặc collision; camera/lock state tiếp tục bám vị trí boss.
- Worktree có thay đổi collision/capture từ trước: không sửa hoặc stage các file ngoài phạm vi.

## 5. Acceptance Criteria

- `chooseLockTarget` tiếp tục từ chối mục tiêu bị tường che khi chọn mới.
- Một lock đã tồn tại vẫn giữ khi target còn sống, trong 30 m, dù LOS bị collider chặn.
- Lock hủy khi target chết hoặc cách quá 30 m.
- `cmake --build build` PASS.
- `ctest --test-dir build --output-on-failure` PASS.
- `build/glory_client.exe --smoke-test` PASS.
- `build/glory_client.exe --combat-test` PASS.
- Không thay đổi hitbox, combo, sát thương hoặc cooldown.
