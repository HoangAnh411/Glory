# Client-side prediction và reconciliation

## 1. Mục tiêu

Cho client dự đoán chuyển động của nhân vật cục bộ ngay trong fixed tick, gắn sequence cho từng input, nhận ACK từ snapshot authoritative, và rewind/replay khi server position lệch quá ngưỡng.

## 2. Danh sách task

1. Mở rộng `ClientInputPacket` bằng `inputSequence` và `SnapshotPlayer` bằng `lastProcessedInputSequence`; cập nhật codec nhị phân có kiểm tra kích thước/schema.
2. Server gắn sequence vào input session và đưa sequence cuối đã áp dụng vào snapshot của từng player.
3. Thêm logic prediction trong `glory_game`: lưu frame/yaw/delta/state theo sequence, ACK input theo thứ tự wrap-safe, so sai lệch authoritative, khôi phục controller (grounded, dodge và knockback state) và replay input chưa ACK.
4. Khi kết nối, client cập nhật prediction mỗi fixed tick thay vì chờ snapshot; snapshot cập nhật authoritative state và reconciliation.
5. Hiển thị sequence client/server, số input chưa ACK, số lần reconciliation và sai lệch lớn nhất/gần nhất trong DebugPanel.
6. Thêm unit test cho prediction tức thời, ACK/prune, rewind/replay và đăng ký CTest.

## 3. File cần sửa / tạo

- `network/include/glory/network/network.hpp`
- `network/src/network.cpp`
- `server/include/glory/server/server_simulation.hpp`
- `server/src/server_simulation.cpp`
- `server/src/main.cpp`
- `game/include/glory/game/character_controller.hpp`
- `game/src/character_controller.cpp`
- `game/include/glory/game/prediction.hpp`
- `game/src/prediction.cpp`
- `game/src/main.cpp`
- `engine/include/glory/engine/application.hpp`
- `engine/src/debug_panel.cpp`
- `tests/network_tests.cpp`
- `tests/server_authoritative_tests.cpp`
- `tests/prediction_reconciliation_tests.cpp`
- `CMakeLists.txt`
- `PLAN.md`
- `.ai/specs/prediction-and-reconciliation/{plan,test-report,review}.md`

## 4. Rủi ro và giảm thiểu

- Sequence wrap-around: so sánh bằng hiệu signed 32-bit, bỏ qua ACK cũ.
- Replay khác mô phỏng server: lưu nguyên input, yaw và delta của từng tick; controller hỗ trợ restore trạng thái authoritative cần cho di chuyển/dodge.
- ACK đến mà input đã bị loại khỏi ring buffer: giữ giới hạn queue hữu hạn và dùng vị trí authoritative làm điểm phục hồi.
- Codec không tương thích phiên bản cũ: tăng schema version và từ chối payload sai kích thước/phiên bản.
- Tách lớp prediction vào `glory_game`, không phụ thuộc SDL/OpenGL/ImGui.

## 5. Tiêu chí chấp nhận

- Input packet round-trip giữ sequence; snapshot round-trip giữ ACK từng player.
- Client controller đổi vị trí ngay khi predict, không cần snapshot.
- ACK loại đúng input đã xử lý, không loại input mới hơn.
- Misprediction lớn rewind về authoritative position rồi replay phần còn chưa ACK; sai lệch nhỏ không gây correction không cần thiết.
- Server snapshot báo ACK sequence thực thi trên server.
- DebugPanel hiển thị các chỉ số prediction.
- Build thành công, 23 CTest PASS, server và client smoke-test PASS.
