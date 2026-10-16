# Báo Cáo Kiểm Thử (Test Report): Combat Controls, Arena Map & Play Modes

- **Thời gian thực hiện:** 2026-10-14 20:45
- **Người thực hiện (Vai trò):** TESTER
- **Kết quả tổng quan:** **PASS (37/37 Tests Passed - 100%)**

---

## 1. Lệnh Đã Thực Thi

### Lệnh 1: Biên dịch mã nguồn
```powershell
cmake --build build
```
- **Exit Code:** 0 (Thành công sạch sẽ, không có cảnh báo hoặc lỗi biên dịch)

### Lệnh 2: Kiểm thử tự động (CTest)
```powershell
ctest --test-dir build --output-on-failure
```
- **Exit Code:** 0
- **Kết quả:** 37/37 tests pass 100% trong 4.01 giây.

### Lệnh 3: Smoke test tích hợp Game Client
```powershell
.\build\glory_client.exe --smoke-test
```
- **Exit Code:** 0
- **Output:**
  ```text
  Smoke audio engine: ready
  SDL3/OpenGL 4.5 context and 120 fixed ticks passed
  ```

---

## 2. Chi Tiết Các Tiêu Chí Kiểm Thử (Acceptance Criteria)

| STT | Tiêu chí chấp nhận theo Plan | Trạng thái | Ghi chú kiểm chứng |
|:---:|:---|:---:|:---|
| 1 | Bổ sung phím số `Key1..Key4` vào InputSystem | **PASSED** | Ánh xạ chính xác `SDL_SCANCODE_1..4` sang `Control::Key1..4`. |
| 2 | Action Map hỗ trợ `Skill1..Skill4` | **PASSED** | Unit test `testSkillActionBindings()` xác nhận `Skill1..4` gắn mặc định vào `Key1..4` và phản hồi trạng thái pressed chính xác. |
| 3 | Phím Né đòn `[Shift] Dodge` | **PASSED** | Sửa nhãn HUD thành `[Shift] Dodge`, `Dodge` gắn `KeyLeftShift`, `Jump` gắn `KeySpace`. |
| 4 | Click chuột vào icon Skill để tung chiêu | **PASSED** | Khi hover ô skill trên action bar và click chuột trái, `requestedSkillId` nhận ID skill và thực thi đòn đánh. |
| 5 | Tự động chuyển Offline Simulation khi không có server | **PASSED** | Client chỉ khóa vòng lặp khi `networkClient->state() == Connected && accountPanel.authenticated`. Khi server chưa bật, game tự động chuyển về chế độ chơi Offline Sandbox mượt mà, không bị treo. |
| 6 | Nút "Play Offline" trong cửa sổ Login | **PASSED** | Người chơi có thể bấm nút này để đóng ngay modal và chơi đơn. |
| 7 | Mở rộng sàn đấu Arena Ground | **PASSED** | `floorY = 0.0F` (trùng khớp mặt phẳng va chạm chân nhân vật Y=0), kích thước mở rộng 60x60m với màu sàn đá đấu trường sáng rõ. |
| 8 | Launcher `play_game.bat` linh hoạt | **PASSED** | Hỗ trợ 2 tùy chọn: [1] Chơi Offline Sandbox ngay lập tức; [2] Tự động bật Dedicated Server chạy nền và kết nối Client. |

---

## 3. Kết Luận
Tất cả các tiêu chí đã hoàn thành và kiểm thử thực tế đạt 100% PASS. Đủ điều kiện chuyển tiếp sang bước **REVIEWER**.
