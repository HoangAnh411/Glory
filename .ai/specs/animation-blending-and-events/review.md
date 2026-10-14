# Code Review: Animation Blending and Events

### 1. Phải sửa (Critical / Required)

- Không còn vấn đề bắt buộc sửa. CPU pose sampling/blending và event dispatcher không phụ thuộc OpenGL; skin palette đi qua pose đã blend. Cross-fade dùng pose đang hiển thị làm nguồn cho lần chuyển state kế tiếp, event chạy qua wrap-around, và callback có thể truyền exception về vòng xử lý ứng dụng thay vì terminate do `noexcept`.
- Getter clip/state/time đã theo state đang hoạt động nên các nút, timeline và nhãn DebugPanel không bị giữ ở clip cũ.
- Hai lỗi phát hiện khi review đã được sửa; build, sáu CTest và smoke test 120 tick đã chạy lại PASS sau các sửa đổi.

### 2. Nên sửa (Recommended)

- Event metadata từ glTF extras hiện được đọc bằng regex cho cấu trúc đơn giản, với thời gian số thập phân và tham số dạng chuỗi. Khi metadata phát triển, nên chuyển sang parser JSON có sẵn trong pipeline để hỗ trợ đầy đủ escape, exponent và kiểu tham số khác.
- UI event log hiển thị thời gian authored của event trong clip; có thể bổ sung thời gian playback tuyệt đối nếu cần phân biệt các lần lặp trong cùng log.

### 3. Gợi ý (Suggestions)

- Khi hệ thống combat/audio được triển khai, đăng ký consumer cho `hit_frame`, `footstep` và `swing_sound` để nối event với hitbox và phát âm thanh.
- Bổ sung kiểm thử nhiều event cùng thời điểm và delta lớn đi qua nhiều vòng; hiện đã kiểm tra crossing bình thường, chưa tới mốc và wrap-around.

## Kết luận

Đạt yêu cầu blend animation và animation event; không có lỗi chặn. Có thể tick hai mục tương ứng trong roadmap.
