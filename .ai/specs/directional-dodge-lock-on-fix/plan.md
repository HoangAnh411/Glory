# Directional Dodge During Lock-On

## 1. Objective

Sửa lỗi né khi đang khóa mục tiêu: nếu người chơi đang giữ WASD, hướng né phải dùng hướng di chuyển tương đối theo camera; nếu không có phím di chuyển, vẫn né về phía mục tiêu như hành vi lock-on mặc định. Không thay đổi hitbox, combo, sát thương, cooldown hoặc solver va chạm.

## 2. Tasks

1. Đổi thứ tự ưu tiên chọn `dodgeDirection` trong `CharacterController::update` để `hasMoveInput` được xét trước `targetLocked`.
2. Thêm regression test cho trường hợp giữ hướng di chuyển và nhấn Dodge khi mục tiêu bị khóa.
3. Build target test, chạy test focused và toàn bộ CTest, sau đó chạy smoke test client.
4. Review thay đổi và ghi báo cáo tester/reviewer.

## 3. Files to touch

- `game/src/character_controller.cpp`: sửa thứ tự chọn hướng né.
- `tests/character_controller_tests.cpp`: thêm test hướng né khi lock-on.
- `.ai/specs/directional-dodge-lock-on-fix/test-report.md`: kết quả kiểm thử thật.
- `.ai/specs/directional-dodge-lock-on-fix/review.md`: kết quả review.

## 4. Risks & Mitigations

- Lock-on không có input di chuyển có thể bị thay đổi ngoài ý muốn: giữ nhánh target direction cho trường hợp không có WASD.
- Hướng né có thể bị sai do vector chưa chuẩn hóa: dùng `moveDirection`, vốn đã được chuẩn hóa trước khi chọn hướng.
- Thay đổi có thể ảnh hưởng collision: chỉ đổi vector đầu vào dodge; displacement vẫn đi qua `CollisionWorld::resolve`.
- Worktree có thay đổi collision/capture chưa commit: không sửa hoặc đưa các file đó vào commit.

## 5. Acceptance Criteria

- Khi `targetLocked=true` và giữ W/A/S/D, `state().dodgeDirection` khớp hướng camera-relative tương ứng.
- Khi `targetLocked=true` và không giữ WASD, `state().dodgeDirection` vẫn hướng từ nhân vật tới target.
- Dodge tiếp tục dùng collision solver hiện có.
- `cmake --build build` PASS.
- `ctest --test-dir build --output-on-failure` PASS.
- `build/glory_client.exe --smoke-test` PASS.
- Không thay đổi hitbox, combo, sát thương hoặc cooldown.
