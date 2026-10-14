# Review: Server Authoritative Tick

## 1. Phải sửa

Không phát hiện lỗi bắt buộc phải sửa. Server nhận button masks/yaw thay vì tọa độ hoặc damage; movement, collision, dodge/I-frame và PvP damage được tính trong server simulation. Các tiêu chí nghiệm thu đều được kiểm chứng bằng loopback test.

## 2. Nên sửa

- Snapshot chỉ bao gồm tối đa 8 player và chuỗi animation/skill tối đa 32 byte để giữ gói unreliable trong datagram giới hạn. Khi cần tăng quy mô, nên dùng snapshot delta/chunking và version negotiation.
- Client dùng authoritative state cho nhân vật của mình; remote-player rendering và interpolation thuộc mục roadmap kế tiếp.
- Input session hiện hết hiệu lực sau ba tick không nhận gói. Nên chuyển timeout thành cấu hình hoặc session policy khi có packet loss/reconnect requirements cụ thể.

## 3. Gợi ý

- Thêm input sequence/tick acknowledgment để phát hiện gói cũ và hỗ trợ prediction/reconciliation.
- Thêm test cho nhiều lần connect/disconnect, invulnerability khi bị đánh, payload snapshot đạt giới hạn và packet input malformed.
- Khi tích hợp boss AI vào multiplayer encounter, chọn target/session rõ ràng thay vì dùng legacy single-player controller của server simulation.

## Kết luận

Session/entity, controller và CombatSystem được sở hữu theo ClientId; ENet chỉ vận chuyển input và snapshot. Client kết nối bỏ qua local simulation và dựng state của player từ snapshot server. Review không có mục bắt buộc phải sửa; giới hạn protocol được ghi rõ để phát triển ở bước snapshot/interpolation tiếp theo.
