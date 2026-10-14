# Báo cáo kiểm thử Server Authoritative Tick

## Kết quả tổng quan

**PASS** — Configure/build thành công, toàn bộ 21/21 CTest PASS và smoke test server/client PASS trên Windows với Ninja, LLVM-MinGW Clang và CMake.

## Lệnh đã chạy

- `cmake -S . -B build -G Ninja` — exit code 0.
- `cmake --build build` — exit code 0; build cập nhật target/test authoritative.
- `ctest --test-dir build --output-on-failure` — 21/21 PASS, 0 FAIL.
- `./build/glory_server.exe --smoke-test --port 17778` — exit code 0; lắng nghe UDP port 17778, hoàn tất 120 tick và in `Headless server smoke test passed: 120 fixed ticks at 60 Hz`.
- `./build/glory_client.exe --smoke-test` — exit code 0; SDL3/OpenGL context và 120 fixed ticks PASS.

## Kiểm thử authoritative multiplayer

`glory_server_authoritative` chạy hai client qua ENet loopback và PASS:

- Handshake tạo hai session/ClientId riêng: PASS.
- Client 1 gửi input di chuyển; vị trí được tính ở server và thay đổi: PASS.
- Client 2 gửi input trung tính; vị trí không bị ảnh hưởng bởi input Client 1: PASS.
- Combat hitbox/hurtbox chạy trên server; HP Client 2 giảm: PASS.
- Snapshot encode/decode mang state của cả world và HP đã giảm được client nhận: PASS.
- Ngắt Client 2 xóa đúng session/entity, Client 1 vẫn hoạt động: PASS.

`glory_network` cũng kiểm tra round-trip snapshot, flags, state boss và từ chối snapshot có byte thừa. Bộ snapshot giới hạn 8 player, chuỗi tối đa 32 byte và payload trong giới hạn ENet đã đặt.
