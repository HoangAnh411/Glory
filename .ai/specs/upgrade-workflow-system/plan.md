# Kế Hoạch Nâng Cấp Hệ Thống Workflow Đa Vai Trò (upgrade-workflow-system)

## 1. Mục Tiêu (Objective)
Nâng cấp toàn diện hệ thống workflow trong `D:\Data\Workflow` để:
1. Biến script cài đặt (`apply-workflow.ps1`) thành công cụ thông minh:
   - Tự động nhận diện stack của dự án đích (Node.js/Next.js, Python, Go) qua các file cấu hình (`package.json`, `pyproject.toml`, `go.mod`).
   - Tự động đọc và trích xuất lệnh test/build/lint thật điền sẵn vào `AGENTS.md` của dự án đích.
   - An toàn tuyệt đối: Nếu dự án đích đã có sẵn `AGENTS.md` (như Next.js auto-rule trong `weddingProject`), script sẽ tự động gộp (append) nội dung mà không ghi đè mất cấu hình cũ.
2. Bổ sung kịch bản chuẩn cho tác vụ **"Khám sức khỏe & Hoàn thiện dự án cũ" (Project Audit & Refactor)** vào quy trình `.ai/workflow.md` và các vai trò `.ai/roles/`.
3. Cập nhật tài liệu `README.md` hướng dẫn chi tiết cả 2 chế độ: Bán tự động (Interactive) và Tự động hoàn toàn (Auto-Pilot).

---

## 2. Danh Sách Task Cụ Thể (Tasks)

- [ ] **Task 1: Nâng cấp script `apply-workflow.ps1` & `apply-workflow.bat`:**
  - Viết logic kiểm tra: Nếu `AGENTS.md` tại thư mục đích đã tồn tại, kiểm tra xem đã có nội dung quy trình chưa. Nếu chưa có, gộp thêm vào cuối file thay vì ghi đè.
  - Viết logic quét tự động dự án đích:
    - Nếu phát hiện `package.json`: Đọc các lệnh trong `scripts` (`test`, `build`, `lint`) và tên dự án để tự tạo nội dung `AGENTS.md` tương thích 100%.
    - Nếu phát hiện `requirements.txt` / `pyproject.toml`: Tự động cấu hình Python + pytest.
    - Nếu phát hiện `go.mod`: Tự động cấu hình Go + `go test ./...`.
  - Giữ nguyên mã hóa an toàn tránh lỗi string trong PowerShell 5.1.

- [ ] **Task 2: Bổ sung kịch bản "Audit & Refactor Dự Án Cũ" vào `.ai/workflow.md`:**
  - Bổ sung quy trình đặc thù khi nhận dự án chưa hoàn thiện:
    - Planner: Chạy lệnh build/lint thử nghiệm để bắt log lỗi thật và lập danh sách lỗi cần vá.
    - Coder: Vá các lỗi gãy code, types, logic theo đúng plan.
    - Tester: Chạy lại build/lint thật để xác nhận hệ thống đã xanh lá.
    - Reviewer: Rà soát bảo mật & kiến trúc.

- [ ] **Task 3: Cập nhật các file vai trò trong `.ai/roles/` và `.claude/agents/`:**
  - Cập nhật `planner.md`: Hướng dẫn cách xử lý cả 2 tình huống (Làm tính năng mới vs Audit dự án cũ).
  - Cập nhật `tester.md`: Hướng dẫn Tester ưu tiên chạy lệnh build và lint trước khi chạy test unit/e2e đối với dự án web.

- [ ] **Task 4: Cập nhật tài liệu `README.md`:**
  - Thêm phần mô tả tính năng tự nhận diện stack thông minh của `apply-workflow.bat`.
  - Bổ sung bảng so sánh và câu lệnh mẫu cho cả 2 chế độ: Bán tự động và Auto-Pilot.

- [ ] **Task 5: Kiểm thử thực tế (Verification):**
  - Chạy thử `apply-workflow.bat` với dự án mẫu `D:\Data\weddingProject` để kiểm tra khả năng giữ nguyên nội dung gốc của Next.js và tự nhận diện lệnh `lint`, `build`, `test:e2e`.

---

## 3. Các File Cần Chỉnh Sửa / Tạo Mới (Files to touch)
1. `D:\Data\Workflow\apply-workflow.ps1` (Chỉnh sửa logic gộp file & nhận diện stack)
2. `D:\Data\Workflow\.ai\workflow.md` (Bổ sung kịch bản Audit & Refactor)
3. `D:\Data\Workflow\.ai\roles\planner.md` (Bổ sung hướng dẫn audit)
4. `D:\Data\Workflow\.ai\roles\tester.md` (Bổ sung thứ tự chạy lint/build/test)
5. `D:\Data\Workflow\.claude/agents/planner.md` & `tester.md` (Đồng bộ)
6. `D:\Data\Workflow\README.md` (Cập nhật tài liệu hướng dẫn)

---

## 4. Rủi Ro Tiềm Ẩn & Giải Pháp Dự Phòng (Risks & Mitigations)
- **Rủi ro 1:** Script PowerShell đọc `package.json` bị lỗi nếu file JSON không hợp lệ hoặc chứa cú pháp lạ.
  - *Giải pháp:* Dùng khối `try { ... } catch { ... }` an toàn, nếu parse JSON lỗi thì fallback về template mặc định an toàn.
- **Rủi ro 2:** Gộp file `AGENTS.md` nhiều lần làm lặp nội dung khi chạy script nhiều lần.
  - *Giải pháp:* Kiểm tra nếu trong file đích đã chứa chuỗi `[.ai/workflow.md]` thì bỏ qua việc gộp thêm.

---

## 5. Tiêu Chí Chấp Nhận (Acceptance Criteria)
1. [ ] Chạy `apply-workflow.ps1` vào một thư mục có sẵn `package.json` sẽ tự động phát hiện lệnh test/build và điền vào `AGENTS.md`.
2. [ ] Chạy `apply-workflow.ps1` vào thư mục đã có file `AGENTS.md` (như `weddingProject`) không làm mất nội dung ban đầu của file đó.
3. [ ] Chạy nhiều lần không bị duplicate nội dung.
4. [ ] `.ai/workflow.md` có đầy đủ mục hướng dẫn nhiệm vụ "Audit & Refactor dự án cũ".
5. [ ] Script chạy ổn định, không phát sinh lỗi cú pháp hay bảng mã trên PowerShell 5.1 và CMD.
