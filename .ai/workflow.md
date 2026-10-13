# Quy Trình Làm Việc Đa Vai Trò (Multi-Role Workflow)

Quy trình chuẩn áp dụng cho tất cả các yêu cầu phát triển tính năng hoặc dự án trong repository:
**PLANNER → CODER → TESTER → REVIEWER**

Mỗi tính năng sẽ có một thư mục đặc tả riêng biệt tại:
`.ai/specs/<ten-tinh-nang>/`

Mọi trao đổi giữa các vai trò được thực hiện tuần tự và minh bạch thông qua các file tài liệu trong thư mục tính năng này.

---

## Nguyên Tắc Cốt Lõi

> **Chỉ chuyển bước khi bước trước đã ghi xong file đầu ra tương ứng.**

---

## 2 Chế Độ Thực Thi (Execution Modes)

1. **Chế độ Tương tác (Interactive Mode - Mặc định):**
   - AI dừng lại sau bước Planner để người dùng phê duyệt `plan.md` trước khi Coder bắt đầu.
   - Thích hợp khi muốn kiểm soát chặt chẽ thiết kế và kiến trúc.

2. **Chế độ Tự động hoàn toàn (Auto-Pilot / Autonomous Mode):**
   - Kích hoạt khi người dùng yêu cầu: *"Chạy tự động / Auto-pilot"* hoặc *"Tự động từ A đến Z"*.
   - AI đóng vai trò **Orchestrator**, tự động chuyển tiếp qua toàn bộ 4 vai trò:
     `Planner (ghi plan.md) → Coder (viết code) → Tester (chạy test & tự sửa nếu fail tối đa 3 lần) → Reviewer (review & sửa nếu có Phải sửa) → Tổng kết`.
   - AI chạy liên tục cho đến khi hoàn thành tính năng và chỉ báo cáo kết quả tổng thể cuối cùng.

---

## Chi Tiết Các Bước Thực Hiện

### Bước 1: PLANNER (Lập Kế Hoạch)
- **Tài liệu vai trò:** [.ai/roles/planner.md](roles/planner.md)
- **Hành động:**
  1. Đọc và khảo sát codebase hiện tại.
  2. Nếu yêu cầu còn mơ hồ hoặc thiếu thông tin, phải hỏi lại người dùng để làm rõ.
  3. Tạo thư mục `.ai/specs/<ten-tinh-nang>/` nếu chưa có.
  4. Soạn thảo và lưu kế hoạch chi tiết tại `.ai/specs/<ten-tinh-nang>/plan.md`.
- **Điểm dừng bắt buộc:**
  - Sau khi ghi xong `plan.md`, Planner **DỪNG LẠI**, thông báo cho người dùng và **chờ người dùng phê duyệt** kế hoạch trước khi chuyển sang bước tiếp theo.

---

### Bước 2: CODER (Viết Code)
- **Tài liệu vai trò:** [.ai/roles/coder.md](roles/coder.md)
- **Điều kiện bắt đầu:** Người dùng đã phê duyệt `plan.md`.
- **Hành động:**
  1. Đọc kỹ file `plan.md`.
  2. Triển khai code theo đúng từng task được định nghĩa trong `plan.md`.
  3. Không tự ý viết thêm các tính năng nằm ngoài phạm vi kế hoạch (nếu có lý do bắt buộc phải lệch plan, phải ghi rõ nguyên nhân).
  4. Nếu có phản hồi sửa lỗi từ `test-report.md` hoặc `review.md`, tiến hành sửa triệt để các vấn đề được nêu.
- **Đầu ra:** Mã nguồn đã được cập nhật/tạo mới hoàn chỉnh.

---

### Bước 3: TESTER (Kiểm Thử)
- **Tài liệu vai trò:** [.ai/roles/tester.md](roles/tester.md)
- **Điều kiện bắt đầu:** Coder đã hoàn thành mã nguồn của tính năng.
- **Hành động:**
  1. Đọc tiêu chí chấp nhận (acceptance criteria) trong `plan.md`.
  2. Viết các test case kiểm thử tương ứng.
  3. Chạy test thực tế bằng lệnh test của dự án (xem trong [AGENTS.md](../AGENTS.md) hoặc file cấu hình repo).
  4. Ghi nhận toàn bộ kết quả vào `.ai/specs/<ten-tinh-nang>/test-report.md`.
- **Xử lý kết quả kiểm thử:**
  - **Nếu PASS:** Chuyển sang bước 4 (REVIEWER).
  - **Nếu FAIL:** Chuyển lại cho CODER để sửa lỗi.
    - *Lưu ý vòng lặp:* Tối đa **3 vòng lặp** sửa lỗi giữa Coder và Tester.
    - Nếu sau 3 vòng test vẫn thất bại, **DỪNG LẠI** và báo cáo ngay cho người dùng xin ý kiến.

---

### Bước 4: REVIEWER (Đánh Giá Code)
- **Tài liệu vai trò:** [.ai/roles/reviewer.md](roles/reviewer.md)
- **Điều kiện bắt đầu:** Tester đã xác nhận PASS và ghi nhận `test-report.md`.
- **Hành động:**
  1. Đọc và rà soát code đã thay đổi so với `plan.md`.
  2. Đánh giá chất lượng, logic, bảo mật, xử lý lỗi và tính dễ đọc.
  3. Ghi kết quả vào `.ai/specs/<ten-tinh-nang>/review.md` theo 3 nhóm rõ ràng:
     - **Phải sửa:** Các lỗi nghiêm trọng, sai logic, rủi ro bảo mật hoặc không đạt plan.
     - **Nên sửa:** Các cải tiến về tối ưu hiệu năng, cấu trúc code, clean code.
     - **Gợi ý:** Các đề xuất cải thiện trong tương lai.
- **Xử lý kết quả review:**
  - **Nếu có mục "Phải sửa":** Quay lại bước 2 (CODER) để sửa, sau đó qua bước 3 (TESTER) kiểm tra lại trước khi Reviewer duyệt lại.
  - **Nếu không có mục "Phải sửa" (hoặc chỉ có "Nên sửa" / "Gợi ý"):** Chuyển sang bước Tổng kết.

---

### Bước 5: TỔNG KẾT
- Tóm tắt toàn bộ kết quả công việc đã thực hiện cho người dùng:
  - Các file mã nguồn đã tạo mới hoặc chỉnh sửa.
  - Tóm tắt kết quả kiểm thử (`test-report.md`).
  - Tóm tắt kết quả đánh giá (`review.md`).
  - Hướng dẫn người dùng nghiệm thu hoặc chạy thử.

---

## Quy Trình Đặc Thù: Khám Sức Khỏe & Vá Lỗi Dự Án Cũ (Audit & Refactor)

Áp dụng khi đưa workflow vào một dự án đã có sẵn mã nguồn nhưng **chưa từng kiểm thử, có nguy cơ gãy code, hoặc cần hoàn thiện**:

1. **Tên thư mục đặc tả:** `.ai/specs/project-audit-and-fix/`
2. **Hành động từng vai trò:**
   - **PLANNER:**
     - Chạy thử các lệnh kiểm tra thực tế trên terminal (`npm run lint`, `npm run build`, hoặc lệnh type-check).
     - Rà soát các cảnh báo lỗi compiler, lỗi typescript, import gãy, cấu hình DB/env thiếu.
     - Lập `plan.md` liệt kê danh sách cụ thể các lỗi cần vá và tiêu chí chấp nhận (ví dụ: `build` thành công 100%, không còn lỗi lint).
   - **CODER:**
     - Lần lượt sửa các lỗi cú pháp, types, cấu hình theo đúng danh sách trong `plan.md`.
   - **TESTER:**
     - Chạy lại các lệnh build và lint thật (`npm run build`, `npm run lint`).
     - Chạy các test unit/e2e nếu có (`npm test`, `npm run test:e2e`).
     - Ghi nhận output sạch (Exit code 0, không có lỗi đỏ) vào `test-report.md`.
   - **REVIEWER:**
     - Đánh giá độ ổn định tổng thể của dự án và xác nhận dự án đã sẵn sàng chạy production.

---

## Quy Trình Đặc Thù: Chạy Theo Lộ Trình Tổng Thể Có Sẵn (`PLAN.md` Ở Gốc Dự Án)

Áp dụng khi dự án đã có sẵn một file `PLAN.md` ở thư mục gốc đóng vai trò **Lộ trình tổng thể (Master Roadmap)** với danh sách các bước lớn (`- [ ] ...`):

1. **Phân biệt rõ 2 cấp độ Plan:**
   - **`PLAN.md` (Ở thư mục gốc):** Là lộ trình tổng thể toàn dự án (Master Checklist) và các nguyên tắc kiến trúc chung.
   - **`.ai/specs/<ten-buoc>/plan.md`:** Là kế hoạch kỹ thuật chi tiết cho **từng bước nhỏ** được lấy ra từ `PLAN.md` gốc.
2. **Chu trình thực hiện mỗi bước (Step-by-Step Loop):**
   - **Bước 1 (PLANNER):**
     - Đọc toàn bộ `PLAN.md` ở thư mục gốc.
     - Tìm **mục đầu tiên chưa được đánh dấu `[x]`** (hoặc nhóm việc nhỏ liền kề theo yêu cầu).
     - Soạn kế hoạch triển khai kỹ thuật chi tiết cho mục đó vào `.ai/specs/<ten-buoc>/plan.md` (file cần tạo, cấu trúc class/hàm, lệnh build/test kiểm chứng).
   - **Bước 2 (CODER):**
     - Viết code theo đúng `.ai/specs/<ten-buoc>/plan.md` và tuân thủ mọi quy tắc kiến trúc trong `PLAN.md` gốc (ví dụ: tách logic khỏi render, đặt tên biến/hàm tiếng Anh).
   - **Bước 3 (TESTER):**
     - Chạy lệnh build và test thật trên terminal để đảm bảo bước đó biên dịch và chạy thành công.
     - Ghi kết quả vào `.ai/specs/<ten-buoc>/test-report.md`.
   - **Bước 4 (REVIEWER):**
     - Đối chiếu code với cả `plan.md` chi tiết và các quy tắc trong `PLAN.md` gốc.
     - Ghi nhận xét vào `.ai/specs/<ten-buoc>/review.md`.
   - **Bước 5 (CẬP NHẬT LỘ TRÌNH & TỔNG KẾT):**
     - Chỉ sau khi hoàn tất cả 4 vai trò và build/test PASS, AI mới **đổi `[ ]` thành `[x]` trong file `PLAN.md` ở thư mục gốc** và báo cáo tóm tắt cho người dùng.


