# Quy Định Dành Cho AI Agent (Dự Án Game Glory)

Khi người dùng yêu cầu làm tính năng mới hoặc triển khai dự án trong repository này:
- **BẮT BUỘC** phải tuân thủ nghiêm ngặt quy trình làm việc đa vai trò được mô tả tại [.ai/workflow.md](.ai/workflow.md).
- **Trường hợp ngoại lệ:** Với các tác vụ rất nhỏ (sửa 1 dòng code, sửa lỗi chính tả, đổi chữ, cập nhật nhỏ không ảnh hưởng logic) thì được phép bỏ qua quy trình và xử lý trực tiếp.

---

## Kết Hợp Với Lộ Trình Tổng Thể ([PLAN.md](PLAN.md))

Dự án này có sẵn file **[PLAN.md](PLAN.md)** ở thư mục gốc chứa **Lộ trình tổng thể (Master Roadmap)**, kiến trúc và danh sách các đầu việc cần làm (`- [ ]`).

Khi triển khai dự án này, AI **BẮT BUỘC** phải kết hợp `PLAN.md` và `.ai/workflow.md` theo đúng chu trình sau:

1. **Đọc kỹ [PLAN.md](PLAN.md) trước tiên:**
   - Tuân thủ tuyệt đối các quy tắc trong mục *"Hướng dẫn cho AI"* của `PLAN.md`:
     - Tách logic game khỏi phần render ngay từ đầu.
     - Giải thích bằng tiếng Việt; đặt tên biến, hàm, comment kỹ thuật bằng tiếng Anh.
     - Không dùng game engine có sẵn; không dùng tên hay nhân vật của Vinh Diệu.
     - Mỗi bước phải **build và chạy được**.
2. **Chọn đầu việc tiếp theo:**
   - Tìm **việc đầu tiên chưa tick `[x]`** trong `PLAN.md` (hoặc nhóm nhỏ liền kề theo yêu cầu của người dùng), không nhảy cóc giai đoạn.
3. **Thực thi đầu việc đó qua 4 vai trò (`.ai/workflow.md`):**
   - **PLANNER:** Soạn kế hoạch kỹ thuật chi tiết cho bước đó vào `.ai/specs/<ten-buoc>/plan.md` (xác định file cần tạo/sửa, cấu trúc CMake/C++, tiêu chí nghiệm thu).
   - **CODER:** Viết code C++ / CMake / tài liệu theo đúng `.ai/specs/<ten-buoc>/plan.md`.
   - **TESTER:** Chạy lệnh build và kiểm thử thật trên terminal, ghi kết quả thật vào `.ai/specs/<ten-buoc>/test-report.md`.
   - **REVIEWER:** Đối chiếu code với cả `.ai/specs/<ten-buoc>/plan.md` và các quy tắc trong `PLAN.md` gốc, ghi nhận xét vào `.ai/specs/<ten-buoc>/review.md`.
4. **Cập nhật tiến độ vào [PLAN.md](PLAN.md):**
   - Chỉ sau khi hoàn tất cả 4 vai trò và đảm bảo bước đó đã build/test PASS, AI mới **đổi `[ ]` thành `[x]` trong file `PLAN.md` ở thư mục gốc** và tóm tắt kết quả cho người dùng.

---

## Thông Tin Project & Lệnh Build Thật Trên Máy

- **Tên dự án:** Glory — Game 3D Hành Động MMORPG (Custom Engine)
- **Ngôn ngữ & Chuẩn:** C++ (C++20), CMake 4.4.4, Ninja 1.13.2, LLVM-MinGW Clang++ 23.1.1 (UCRT)
- **Đường dẫn công cụ (dùng nếu terminal chưa nạp PATH mới):**
  - CMake: `C:\Program Files\CMake\bin\cmake.exe`
  - Ninja: `C:\Users\kaede\AppData\Local\Microsoft\WinGet\Links\ninja.exe`
  - Compiler (`clang++` / `g++`): `C:\Users\kaede\AppData\Local\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin`
- **Lệnh configure & build chuẩn:**
  ```powershell
  cmake -S . -B build -G Ninja
  cmake --build build
  ```
- **Lệnh chạy kiểm thử / smoke test:**
  ```powershell
  .\build\glory_bootstrap.exe
  ```
