# Lag compensation cho đòn đánh

## 1. Mục tiêu

Cho server kiểm tra hitbox của đòn đánh lên hurtbox tại tick mà client đang nhìn thấy, trong giới hạn tối đa 0.35 giây. Kết quả sát thương vẫn được áp dụng trên trạng thái authoritative hiện tại.

## 2. Danh sách task

1. Thêm `WorldHistoryBuffer` thuần game, lưu vị trí/hurtbox/HP của player và boss tối đa 61 sample (một giây ở 60 Hz), truy vấn theo tick có nội suy transform.
2. Thêm `LagCompensator` để kẹp `targetTick` về quá khứ tối đa 21 tick (0.35 giây), rồi truy vấn lịch sử an toàn.
3. Mở rộng `ClientInputPacket` để mang `targetTick`; client gửi tick snapshot đã lùi theo interpolation delay 5 tick.
4. Server truyền tick vào session, ghi world history mỗi fixed tick và cung cấp hurtbox lịch sử làm override riêng cho bước hit-test của `CombatSystem`; transform sống của entity không bị tua ngược nên không cần phục hồi sau kiểm tra.
5. Giữ HP server hiện tại làm nguồn authoritative; nếu hit, áp dụng damage/knockback lên entity hiện tại.
6. Thêm CTest kiểm tra query/interpolation, hit tại vị trí lịch sử trong khi target hiện tại ở xa, HP giảm nhưng transform không bị tua ngược, và giới hạn rewind.

## 3. File cần sửa / tạo

- `game/include/glory/game/world_history.hpp`
- `game/src/world_history.cpp`
- `network/include/glory/network/network.hpp`
- `network/src/network.cpp`
- `game/src/main.cpp`
- `server/include/glory/server/server_simulation.hpp`
- `server/src/server_simulation.cpp`
- `server/src/main.cpp`
- `tests/lag_compensation_tests.cpp`
- `CMakeLists.txt`
- `PLAN.md`
- `.ai/specs/lag-compensation/{plan,test-report,review}.md`

## 4. Rủi ro và giảm thiểu

- Client gửi tick tương lai hoặc quá cũ: kẹp vào `[currentTick - 21, currentTick]` và không cho truy vấn ngoài vùng lịch sử.
- HP/transform bị rollback ngoài ý muốn: chỉ mượn historical hurtbox trong hit check, không thay đổi controller hiện tại; damage áp dụng lên HP hiện tại.
- World entity không có sample: bỏ qua rewind cho entity đó và dùng hit test hiện tại.
- Buffer tăng bộ nhớ: giữ tối đa 61 snapshot world, giới hạn 8 player và một boss theo giới hạn server hiện tại.
- Phụ thuộc render: module history và lag compensation chỉ dùng `glory_game`/GLM, không dùng SDL/OpenGL.

## 5. Tiêu chí chấp nhận

- History query đúng tick và nội suy đúng transform giữa hai tick.
- Kịch bản target di chuyển từ X=0 tới X=5: tick nhìn thấy X=2 có hit dù vị trí hiện tại X=4; HP giảm và vị trí authoritative vẫn X=4.
- Rewind request vượt giới hạn bị kẹp ở 21 tick; tick tương lai cũng không vượt current tick.
- Server dùng targetTick trong input và tích hợp rewind với hit check của combat PvP.
- Build thành công, 24 CTest PASS, server và client smoke-test PASS.
