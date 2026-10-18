# World Collision Prop Alignment

## 1. Objective

Sửa các collider vật thể mà người chơi có thể đi xuyên hoặc chỉ va chạm một phần dù mesh nhìn thấy là vật thể rắn. Bắt đầu từ lỗi đã tái hiện được: `lamp_wooden_lantern#10` bị đặt `blocksMovement: false`. Đồng thời đưa collider compound của hai cổng về đúng chiều cao mesh đã đo được, rồi kiểm tra các vật thể rắn còn lại bằng diagnostic bounds và đường chạy thật.

Không thay đổi hitbox combat, combo, sát thương, cooldown hoặc solver chuyển động dùng chung ngoài phần dữ liệu collider được chứng minh sai.

## 2. Tasks

1. **PLANNER evidence**
   - Giữ nguyên các thay đổi collision regression chưa commit trong worktree.
   - Đối chiếu `world_map.json`, `world_colliders.json`, loader `CollisionWorld::loadFromFiles`, diagnostic render/collider bounds và test collision hiện có.
   - Phân biệt collider cố ý thu gọn (thân cây) và collider của vật thể rắn bị thiếu blocker; không mở rộng collider cây nếu chưa có đường chạy tái hiện.
2. **CODER data fix**
   - Đổi `lamp_wooden_lantern#10.blocksMovement` thành `true` để trụ đèn trên đường spawn được solver chặn như các trụ đèn cùng loại.
   - Sửa chiều cao các phần hỗ trợ của `village_gate_tower#35` và `#51` từ `6.0` lên `8.5` để khớp bounds mesh đã đo, giữ nguyên bốn phần và khoảng trống cổng.
   - Không sửa `game/src/collision.cpp` nếu kiểm thử cho thấy solver đã chặn đúng AABB.
3. **CODER regression coverage**
   - Cập nhật `tests/collision_tests.cpp` để yêu cầu trụ đèn `#10` là blocker, kiểm tra capsule đi từ đường spawn tới trụ bị dừng trước bounds, và kiểm tra hai gate vẫn có bốn support với chiều cao `8.5`.
   - Giữ các kiểm thử đường đi qua khoảng trống cổng và các collider non-blocking hiện có.
4. **TESTER**
   - Chạy test collision trước thay đổi để ghi nhận test mới fail trên dữ liệu hiện tại.
   - Build bằng CMake/Ninja, chạy test collision và toàn bộ CTest.
   - Chạy smoke client và `--capture-colliders`, lưu log/capture validation ở đường dẫn mới để không ghi đè artifact hiện có.
   - Kiểm tra diagnostic không còn `NON-BLOCKING` cho trụ đèn #10 và không còn mismatch chiều cao ở hai gate; kiểm tra route capsule không xuyên trụ và gate opening vẫn đi được.
5. **REVIEWER**
   - Đối chiếu thay đổi với plan, `PLAN.md`, kiến trúc shared collision world và các file dirty có sẵn.
   - Ghi `test-report.md` và `review.md`; chỉ cập nhật `PLAN.md` nếu có roadmap item mới thực sự được chứng minh, không tick lại mục collision đã hoàn thành.

## 3. Files to touch

- `.ai/specs/world-collision-prop-alignment/plan.md`
- `.ai/specs/world-collision-prop-alignment/test-report.md`
- `.ai/specs/world-collision-prop-alignment/review.md`
- `assets/data/world_colliders.json`
- `tests/collision_tests.cpp`
- `game/src/main.cpp` to keep the live south gate route valid around the newly blocking lamp
- `game/src/collision.cpp` chỉ khi test chứng minh lỗi nằm trong solver
- `captures/world-collision-prop-alignment/` cho artifact validation mới

## 4. Risks & Mitigations

- Bật blocker cho trụ đèn có thể làm thay đổi route spawn đã có chủ ý; kiểm tra cả hai hướng đi quanh đường và giữ khoảng trống cổng.
- Tăng chiều cao gate support có thể ảnh hưởng camera hoặc nhảy; giữ nguyên footprint X/Z và chạy smoke/camera collision capture.
- Collider thân cây nhỏ hơn canopy là chủ ý để chặn thân cây; chỉ đổi khi có vị trí tái hiện cụ thể chứng minh đó là vật thể cần solid toàn bộ.
- Worktree có nhiều capture/spec dirty từ bước trước; không reset, xóa hoặc stage chúng ngoài file thuộc task này.

## 5. Acceptance Criteria

- Test mới fail trước khi sửa vì `lamp_wooden_lantern#10` đang non-blocking hoặc gate support chưa cao `8.5`.
- Sau sửa, build exit code 0, collision test PASS và toàn bộ CTest PASS.
- Capsule người chơi không đi xuyên qua `lamp_wooden_lantern#10` khi đi qua vị trí `[0, 0, -8.5]` từ hai hướng.
- Hai gate vẫn giữ đúng bốn support, chiều cao support khớp mesh `8.5`, và đường giữa cổng vẫn traversable hai chiều.
- `--capture-colliders` báo bounds khớp trong sai số `0.15 m`; trụ đèn #10 là blocking. Nếu capture dừng ở camera scenario cũ, phải ghi rõ lỗi đó và vẫn kiểm tra các diagnostic lines đã sinh.
- Smoke client PASS; không thay đổi hitbox, combo, sát thương hoặc cooldown.
- `test-report.md` có lệnh và output thực tế; `review.md` không có mục **Phải sửa** trước khi kết thúc.
