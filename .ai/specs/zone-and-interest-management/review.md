# Review: Chia zone và interest management

## Phạm vi

Đã rà soát kế hoạch và thay đổi tại game logic, packet codec, server snapshot/loot routing, zone transition, client interpolation và DebugPanel.

### 1. Phải sửa

- Không phát hiện lỗi bắt buộc sửa. Snapshot được gửi theo từng client sau khi lọc zone/AOI; loot sync cũng lọc zone và bán kính. Zone request kiểm tra character đang chọn và session sở hữu trước khi teleport.
- Grid dùng khoảng cách 2D X/Z đúng yêu cầu, floor hỗ trợ tọa độ âm, cập nhật cell khi entity đổi vị trí/zone và từ chối tọa độ/radius ngoài miền an toàn.
- Khi nhận notification chuyển zone, client reset interpolation, dọn loot proxy và cập nhật spawn authoritative.

### 2. Nên sửa

- Khi số lượng entity server lớn hơn giới hạn 16-bit của bộ đếm snapshot, hiện tại cần mở rộng kiểu đếm hoặc giới hạn có chủ đích. Với phạm vi hiện tại (8 player, boss và tối đa 16 loot), giới hạn chưa thể đạt.

### 3. Gợi ý

- Bổ sung integration test chạy ENet với nhiều client ở hai zone để xác nhận trực tiếp từng gói snapshot/loot sau routing server. Unit test hiện kiểm tra đầy đủ grid/AOI, zone codec và snapshot codec riêng.
- Cân nhắc chuyển spawn point và định nghĩa zone ra dữ liệu cấu hình khi có map/portal thực tế; các spawn hiện là vị trí thử nghiệm cho hai nút teleport.

## Kết luận

Đạt kế hoạch và đủ điều kiện tick roadmap. Build, 28/28 CTest và smoke test server/client đều PASS.
