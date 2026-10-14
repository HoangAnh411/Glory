# Báo cáo kiểm thử: Chia zone và interest management

## Lệnh đã chạy

- `cmake -S . -B build -G Ninja` — exit code 0.
- `cmake --build build` — exit code 0.
- `ctest --test-dir build --output-on-failure` — exit code 0, 28/28 passed.
- `.\build\glory_server.exe --smoke-test --port 17784 --database :memory:` — exit code 0.
- `.\build\glory_client.exe --smoke-test` — exit code 0.

## Kết quả theo tiêu chí

- Cell 32m theo X/Z, gồm tọa độ âm dùng floor — Passed.
- AOI bán kính 40m theo X/Z; entity cùng zone trong tầm xuất hiện, entity ngoài tầm biến mất sau khi di chuyển — Passed.
- Chuyển entity từ Zone 1 sang Zone 2: truy vấn zone cũ không trả entity, truy vấn zone mới chỉ trả entity thuộc zone đó — Passed.
- Snapshot có zone và số lượng entity tổng/visible; codec snapshot, zone request và notification round-trip — Passed.
- Decoder từ chối notification bị cắt — Passed.
- Hồi quy toàn bộ CTest — 28/28 Passed.
- Server smoke — 120 fixed ticks at 60 Hz, Passed.
- Client smoke — SDL3/OpenGL 4.5 context và 120 fixed ticks, Passed.

## Ghi chú lần chạy

Lượt build đầu tiên bắt lỗi cú pháp khởi tạo `glm::vec3` trong test mới; test được sửa sang khởi tạo tường minh. Build và toàn bộ kiểm thử sau sửa đều PASS.
