# Đánh Giá Kỹ Thuật (Review Report)
## Feature: Thành Tân Thủ (Novice Village World & Mouse Capture)
**Spec:** `.ai/specs/novice-village-world-and-mouse-capture/plan.md`  
**Báo cáo kiểm thử:** `.ai/specs/novice-village-world-and-mouse-capture/test-report.md`  
**Ngày review:** 2026-10-15  
**Trạng thái nghiệm thu:** **PASS (ĐẠT)**

---

### 1. Đối Chiếu Kế Hoạch & Quy Chuẩn (Compliance Check)

| Tiêu chí | Trạng thái | Đánh giá chi tiết |
| :--- | :---: | :--- |
| **Ẩn & Khóa chuột trong game** | **ĐẠT** | Sử dụng `SDL_SetWindowRelativeMouseMode` trong vòng lặp chính của `application.cpp`. Chuột được ẩn và bắt chuyển động delta mượt mà; tự động nhả chuột khi mở cửa sổ Dev Tools (`F3`). |
| **Khắc phục lỗi đất ám xanh biển** | **ĐẠT** | Cải tiến shader ambient trong `model_renderer.cpp`: chuyển từ lấy mẫu trực tiếp bầu trời xanh sang mô hình bán cầu (Hemispheric Ambient) có khử bão hòa ánh trời xanh và phản xạ màu đất ấm. Màu cỏ xanh, đường sỏi xám và đất nâu hiển thị chân thực, tươi sáng. |
| **Phân loại mô hình & Tô màu PBR** | **ĐẠT** | Đã tạo bảng màu PBR `novice_village_palette.png` với 16 chất liệu riêng biệt và sinh 18 mô hình 3D glTF 2.0 phân loại rõ ràng (nhà cửa, quán trọ, tiệm rèn, cối xay gió, tháp canh cổng làng, giếng nước, cây sồi, phong vàng, thông, hoa, đèn, xe rơm, thùng, ghế, rào, đồi gò, vách đá). |
| **Khám phá Thành Tân Thủ** | **ĐẠT** | Tọa độ người chơi spawn tại `(0, 0, 0)` ngay trung tâm quảng trường giếng nước và cây sồi. Người chơi có thể tự do di chuyển (WASD), nhảy (Space), né (Shift) và dạo quanh các phân khu: Phố thợ rèn, Quán trọ, Nhà dân, Đồi cối xay gió, Rừng thông hoang sơ. |
| **Hệ thống va chạm (Collision AABB)** | **ĐẠT** | Đã cấu hình AABB va chạm chính xác cho các vật thể. Cổng làng dùng `multi_collision` để người chơi có thể bước xuyên qua vòm cổng giữa hai tháp canh. |
| **Bảo toàn các ràng buộc kiểm thử gốc** | **ĐẠT** | Tọa độ `boss_start` tại `[-3.8, 0, 0]` và `player_start` tại `[0, 0, 0]` được giữ vững, đảm bảo 37/37 bài unit test gốc tiếp tục pass 100%. |
| **Quy ước đặt tên & Bản quyền** | **ĐẠT** | Tuân thủ tuyệt đối quy định: Không sử dụng tên riêng hoặc nhân vật bản quyền của Vinh Diệu/Toàn Chức Cao Thủ; sử dụng thuật ngữ thế giới mở MMORPG nguyên bản ("Thành Tân Thủ - Glory Novice Village"). Giải thích bằng tiếng Việt, mã nguồn và comment chuẩn tiếng Anh. |

---

### 2. Đánh Giá Kiến Trúc Mã Nguồn (Code Quality)
- **Tách biệt logic & đồ họa:** Dữ liệu thế giới được tải độc lập từ JSON (`world_map.json`), renderer chỉ phụ trách nạp asset và vẽ.
- **Tối ưu hóa hiệu năng:** Toàn bộ 18 loại mô hình tĩnh dùng chung một Texture Atlas duy nhất (`novice_village_palette.png`), giảm thiểu state change trong GPU; các mô hình cùng loại tái sử dụng chung `Model` template đã nạp sẵn trong bộ nhớ (`propModelTemplates_`).
- **An toàn bộ nhớ (Memory Safety):** Không xuất hiện cấp phát con trỏ trần (`new`/`delete` thủ công), toàn bộ tài nguyên OpenGL được quản lý qua RAII (`std::unique_ptr`, `Texture2D`, `Mesh`).

---

### 3. Kết Luận
Tính năng đáp ứng xuất sắc mọi tiêu chí đã đề ra từ yêu cầu của người dùng và quy trình `.ai/workflow.md`. Sẵn sàng để người chơi khởi chạy `.\play_game.bat` và dạo quanh khám phá Thành Tân Thủ!
