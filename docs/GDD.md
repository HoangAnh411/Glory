# GDD: Lam Vực

## Tầm nhìn

Lam Vực là game hành động 3D tập trung vào kỹ năng cá nhân: đọc đòn, định vị bằng di chuyển tự do, né đúng nhịp và tự chọn hướng tấn công. Mục tiêu ban đầu là một trải nghiệm chơi đơn hoàn chỉnh về combat; cùng bộ luật đó có thể mở rộng thành đấu trường đối kháng. MMO là hướng dài hạn, không phải điều kiện để bản chơi được đầu tiên thành công.

## Bối cảnh

Lam Vực gồm những vùng đất quanh một lòng chảo biển sâu. Theo chu kỳ, một hiện tượng gọi là Triều Ảnh làm địa hình và tàn tích dịch chuyển, đồng thời đánh thức các Cổ Thủ Vệ bằng đá và kim loại. Người chơi là một Kiếm Vũ thuộc đoàn hộ hành, nhận nhiệm vụ ổn định các điểm Triều Ảnh và tìm hiểu vì sao chu kỳ đang ngắn lại. Cốt truyện của vertical slice khép lại trong một nhiệm vụ, không yêu cầu thế giới mở hay MMO.

## Fantasy và điểm khác biệt

Người chơi chiến thắng nhờ đọc telegraph, khoảng cách và hướng đòn thay vì khóa mục tiêu hay tự động tung kỹ năng. Đòn đánh, né và phản kích cần có cam kết thời gian; sai nhịp tạo cơ hội cho đối thủ. Camera third-person hỗ trợ ngắm tự do. Combat phải có phản hồi rõ qua animation, hit-stop, âm thanh và hiệu ứng, nhưng hiệu ứng không che telegraph.

Trong PvP, hai bên dùng cùng class và bộ kỹ năng; sát thương, thời gian hồi và tài nguyên được cân bằng độc lập với tiến trình PvE. Điều này giúp kiểm chứng chất lượng điều khiển và quyết định combat trước khi mở rộng class.

## Vòng lặp chơi

1. Tại trạm hộ hành, chọn nhiệm vụ, trang bị một biến thể kỹ năng và xem mục tiêu.
2. Vào khu vực, di chuyển/quan sát, đánh bại nhóm địch nhỏ và dùng địa hình để áp sát hoặc thoát nguy hiểm.
3. Đọc đòn boss, né hoặc phản kích để tạo cửa sổ gây sát thương; hoàn thành mục tiêu.
4. Nhận vật liệu và dữ liệu chiến đấu, mở một lựa chọn nâng cấp nhỏ hoặc ngoại hình.
5. Quay lại trạm, thử thay đổi trong lượt tiếp theo hoặc vào đấu trường luyện kỹ năng.

## Class đầu tiên: Kiếm Vũ

Kiếm Vũ là đấu sĩ cự ly gần, cơ động vừa phải, dùng kiếm một tay. Tài nguyên **Thế** tăng khi đánh trúng hoặc phản kích đúng nhịp, giảm dần khi không giao chiến. Năm kỹ năng khởi điểm:

1. **Khai Phong** — chuỗi ba nhát nhẹ để thăm dò; nhát cuối gây choáng tư thế nhỏ nếu nối đúng nhịp.
2. **Lưu Bộ** — lướt ngắn theo hướng di chuyển; né đúng khung đòn tạo một điểm Thế.
3. **Nguyệt Tảo** — chém vòng cung rộng trước mặt, tiêu Thế để tăng khả năng phá tư thế; khởi chiêu có thể bị ngắt.
4. **Tĩnh Kiếm** — đỡ đòn chính diện trong cửa sổ ngắn; đỡ chuẩn mở phản kích nhanh, hụt nhịp chịu phạt hồi phục.
5. **Hồi Triều** — tiêu toàn bộ Thế để lao chém theo hướng ngắm; sát thương tăng theo tài nguyên, không tự khóa mục tiêu.

## Vertical slice

- **Map PvE:** Miếu Triều Cạn, một khu di tích nhỏ gồm đường tiếp cận, khoảng giao chiến phụ và đấu trường boss; có checkpoint trước boss.
- **Người chơi:** một class Kiếm Vũ, di chuyển, camera third-person, năm kỹ năng, phản hồi trúng đòn và giao diện tối thiểu.
- **Boss:** Cổ Thủ Vệ Miếu, một boss có hai nhịp chiến đấu: đòn quét/chấn nền dễ đọc và pha tăng tốc khi mất một nửa giáp. Đánh trúng các cửa sổ sau telegraph làm lộ lõi trong thời gian ngắn.
- **PvP:** một đấu trường đối xứng, trận 1v1 với cùng class và bộ kỹ năng; dùng cùng luật combat với chỉ số được chuẩn hóa. Netcode authoritative thuộc giai đoạn 5, không nằm trong bootstrap giai đoạn 0.
- **Không thuộc slice:** nhiều class, thế giới mở, nhiệm vụ lặp vô hạn, guild/chợ, dungeon nhiều người, hệ thống MMO và nội dung live-service.

## Tiêu chí cảm nhận

Người chơi có thể giải thích vì sao bị trúng đòn, thấy rõ cơ hội phản công, và cảm nhận khác biệt giữa né, đỡ, đánh thường và kỹ năng tiêu tài nguyên. Một lượt boss ngắn đủ để học telegraph và thể hiện cải thiện ở lần thử tiếp theo.
