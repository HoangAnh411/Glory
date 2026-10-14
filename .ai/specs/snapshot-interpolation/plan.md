# Snapshot và nội suy cho nhân vật khác

## 1. Mục tiêu

Hiển thị trạng thái người chơi từ xa mượt giữa các snapshot authoritative, xử lý gói đến sai thứ tự và mất gói ngắn, đồng thời dọn dữ liệu/render resource khi người chơi rời server.

## 2. Danh sách task

1. Bổ sung `SnapshotInterpolator` vào `glory_network`, giữ snapshot theo `serverTick` và thời điểm nhận.
2. Nội suy vị trí, vận tốc và yaw theo đường góc ngắn nhất với độ trễ mặc định 75 ms; giới hạn extrapolation ở 100 ms.
3. Client nhận snapshot authoritative, cập nhật local player và tạo danh sách remote player nội suy.
4. Render remote player với animation state từ snapshot, màu phân biệt; xóa renderer khi snapshot mới loại client.
5. Hiển thị client id, vị trí, khoảng cách, HP và animation trong DebugPanel.
6. Thêm CTest kiểm tra endpoint/alpha 0.5, yaw qua 360 độ, packet out-of-order, extrapolation và cleanup.

## 3. File cần sửa / tạo

- `network/include/glory/network/network.hpp`
- `network/src/network.cpp`
- `game/src/main.cpp`
- `engine/include/glory/engine/application.hpp`
- `engine/src/application.cpp`
- `engine/src/debug_panel.cpp`
- `engine/include/glory/engine/model_renderer.hpp`
- `engine/src/model_renderer.cpp`
- `tests/snapshot_interpolation_tests.cpp`
- `CMakeLists.txt`
- `PLAN.md`
- `.ai/specs/snapshot-interpolation/plan.md`
- `.ai/specs/snapshot-interpolation/test-report.md`
- `.ai/specs/snapshot-interpolation/review.md`

## 4. Rủi ro và giảm thiểu

- Snapshot đến sai thứ tự: chèn theo tick, thay thế tick trùng và giới hạn dung lượng buffer.
- Yaw qua ranh giới 0/360: dùng độ lệch góc ngắn nhất.
- Snapshot đến trễ: extrapolate theo velocity nhưng kẹp thời gian tối đa.
- Disconnect: tạo danh sách từ snapshot mới nhất và xóa renderer không còn client tương ứng.
- Không đưa render dependency vào netcode: bộ nội suy chỉ phụ thuộc kiểu dữ liệu mạng; renderer thuộc `glory_engine`.

## 5. Tiêu chí chấp nhận

- Snapshot được lưu theo thứ tự tick và remote local id không xuất hiện trong kết quả sample.
- Vị trí khớp hai đầu và trung điểm; yaw 350°→10° đi qua 0° theo đường ngắn.
- Extrapolation không vượt giới hạn cấu hình; player biến mất khỏi snapshot mới sẽ bị loại.
- Remote player được render/animate và có thông tin debug.
- Build thành công, toàn bộ 22 CTest PASS, server `--smoke-test` và client `--smoke-test` thoát mã 0.
