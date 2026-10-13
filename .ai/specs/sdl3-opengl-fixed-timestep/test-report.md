# Báo Cáo Kiểm Thử — SDL3/OpenGL và Fixed Timestep

**Ngày:** 2026-10-13  
**Toolchain:** CMake 4.4.4, Ninja 1.13.2, LLVM-MinGW Clang++ 23.1.1 (UCRT)  
**Kết quả cuối:** **PASS**

## Lệnh đã thực thi

Đã thêm thư mục compiler và Ninja vào PATH trong phiên PowerShell, sau đó chạy:

```powershell
$compilerDir = 'C:\Users\kaede\AppData\Local\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin'
$env:PATH = "$compilerDir;C:\Users\kaede\AppData\Local\Microsoft\WinGet\Links;$env:PATH"
$env:SDL_VIDEODRIVER = 'windows'
& 'C:\Program Files\CMake\bin\cmake.exe' -S . -B build -G Ninja "-DCMAKE_CXX_COMPILER=$compilerDir\clang++.exe"
& 'C:\Program Files\CMake\bin\cmake.exe' --build build
& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure
.\build\glory_client.exe --smoke-test
```

SDL3 3.2.10 được lấy từ tag `release-3.2.10` qua FetchContent. CMake tìm thấy OpenGL (`opengl32`).

## Kết quả

| Kiểm tra | Trạng thái | Kết quả thực tế |
|---|---|---|
| CMake configure với Ninja và Clang++ 23.1.1 | PASS | `configure_exit=0`; CMake báo `Configuring done` và `Generating done`. |
| Build | PASS | `build_exit=0`; Ninja báo `no work to do` ở lần xác nhận cuối sau khi source đã biên dịch/link thành công. |
| CTest fixed timestep | PASS | `ctest_exit=0`; `glory_fixed_timestep` 1/1 passed. |
| SDL3/OpenGL smoke test | PASS | With SDL Windows video driver selected, the visible-window smoke exited `0`; output `SDL3/OpenGL 4.5 context and 120 fixed ticks passed`. |
| Tách simulation khỏi SDL/OpenGL | PASS (build structure) | `glory_game` là library riêng, không link SDL/OpenGL; `glory_engine` sở hữu application/platform loop. |
| Roadmap scope | PASS (đối chiếu file) | Chỉ hai đầu việc đầu Giai đoạn 1 được triển khai; các mục sau chưa được tick. |

## Vòng sửa Tester

Lần build đầu thất bại tại `engine/src/application.cpp:49` vì SDL3 không định nghĩa `SDL_WINDOW_SHOWN`:

```text
error: use of undeclared identifier 'SDL_WINDOW_SHOWN'
```

Coder đã sửa theo SDL3: dùng `SDL_WINDOW_HIDDEN` riêng cho cấu hình ẩn và gọi `SDL_ShowWindow` rõ ràng cho cửa sổ tương tác/visible smoke. Sau sửa, configure/build, CTest và visible-window smoke đều PASS.

## Cảnh báo cấu hình

CMake báo không tìm thấy PkgConfig và LibUSB. Hai dependency này không bắt buộc cho target Windows/OpenGL đã build; SDL3, client và tests đều configure/build thành công.

Một lần chạy phụ qua `Start-Process` trong pseudo-console không hoàn tất trong giới hạn 7 giây và không cho phép liệt kê HWND SDL trong phiên đó; tiến trình kiểm tra do Tester khởi chạy đã được dừng sau khi xác minh đường dẫn executable. Lệnh smoke chạy trực tiếp trong terminal, ép SDL Windows video driver, hoàn tất thành công như kết quả trên. Việc quan sát cửa sổ tương tác lâu dài nên được nghiệm thu từ desktop session thông thường.
