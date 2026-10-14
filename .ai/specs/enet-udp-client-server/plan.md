# ENet UDP Client/Server Plan

## 1. Mục tiêu

Thêm lớp mạng C++20 độc lập dựa trên ENet, protocol gọn có handshake, ping/pong, input và heartbeat/snapshot; cho phép server lắng nghe trên UDP và client kết nối bằng CLI mà không ghép networking vào `glory_game` hay renderer.

## 2. Tasks

1. Fetch ENet bằng CMake FetchContent, cấu hình socket libraries trên Windows và tạo target `glory_network`.
2. Cài `NetworkContext`, `NetworkServer`, `NetworkClient` RAII cùng protocol kiểm tra kích thước/type; channel 0 reliable, channel 1 unreliable sequenced.
3. Thêm `--port` và event/network processing vào `glory_server`; thêm `--connect IP:PORT` vào `glory_client`.
4. Viết loopback CTest bao gồm handshake, reliable/unreliable payload, ping/pong, server response và disconnect.
5. Build, chạy toàn bộ CTest và smoke test hai executable; lưu báo cáo test và review.
6. Tick roadmap sau khi tiêu chí build/test đạt.

## 3. Files

- Sửa `CMakeLists.txt`, `server/src/main.cpp`, `game/src/main.cpp`, `PLAN.md`.
- Tạo `network/include/glory/network/network.hpp`, `network/src/network.cpp`, `tests/network_tests.cpp`.
- Tạo báo cáo trong `.ai/specs/enet-udp-client-server/`.

## 4. Rủi ro và giảm thiểu

- FetchContent phụ thuộc mạng/cache: ghim tag ENet; nếu upstream CMake thiếu target thì build source qua target nội bộ.
- Lifetime của ENet global và host/peer: context phải sống lâu hơn host; host/client không copy được và hủy host trước context.
- Packet malformed: kiểm tra tối thiểu header/type/độ dài trước khi đọc; giới hạn payload.
- Windows linker: gắn `ws2_32` và `winmm` cho thư viện ENet.
- CLI `--connect` không làm mất smoke-test headless của client: chỉ khởi tạo networking khi có địa chỉ server.

## 5. Tiêu chí nghiệm thu

- `glory_network` chỉ phụ thuộc ENet và thư viện chuẩn; game/server/client liên kết đúng ranh giới.
- Server nhận port cấu hình, handshake cấp ClientId, xử lý Ping và gói tin input; client theo dõi trạng thái và RTT.
- Loopback CTest chứng minh connect, payload reliable/unreliable, phản hồi và disconnect.
- Cấu hình/build thành công, toàn bộ CTest PASS; server và client smoke-test PASS.
- Có test report/review; chỉ tick roadmap khi các kiểm tra này đạt.
