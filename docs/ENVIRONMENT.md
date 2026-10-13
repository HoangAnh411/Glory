# Chuẩn Bị Môi Trường Phát Triển (Windows)

Build hiện đã được xác nhận bằng CMake 4.4.4, Ninja 1.13.2 và LLVM-MinGW Clang++ 23.1.1 theo toolchain trong `AGENTS.md`. Các lệnh dưới đây kiểm tra lại công cụ và dependency trên máy.

## Công cụ

1. **Compiler C++20:** Visual Studio 2022 Build Tools hoặc Visual Studio 2022, workload **Desktop development with C++**. Chọn MSVC v143 và Windows SDK. Mở **Developer PowerShell for VS 2022** để lệnh compiler có trong PATH.
2. **CMake 3.25 trở lên:** cài CMake for Windows từ trang chính thức hoặc bật thành phần CMake trong Visual Studio Installer.
3. **Git for Windows:** cài bản chính thức để có Git và tích hợp shell.
4. **RenderDoc:** cài bản Windows x64 từ trang tải chính thức; công cụ này dùng để kiểm tra frame đồ họa sau khi renderer được tạo.

## Kiểm tra cài đặt

Chạy trong Developer PowerShell:

```powershell
cl /Bv
cmake --version
git --version
Get-Command renderdoccmd -ErrorAction SilentlyContinue
Test-Path 'C:\Program Files\RenderDoc\renderdoccmd.exe'
```

Compiler phải nhận cờ C++20 (MSVC: `/std:c++20`); CMake phải là 3.25+. Nếu `cl` không được nhận diện, dùng Developer PowerShell hoặc sửa workload C++ trong Visual Studio Installer. Hai lệnh RenderDoc chỉ giúp xác định vị trí/command có sẵn, vì cài đặt có thể chọn thư mục khác.

## Build, kiểm thử và chạy client

Từ thư mục gốc repo:

```powershell
$compilerDir = 'C:\Users\kaede\AppData\Local\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260908-ucrt-x86_64\bin'
$env:PATH = "$compilerDir;C:\Users\kaede\AppData\Local\Microsoft\WinGet\Links;$env:PATH"
& 'C:\Program Files\CMake\bin\cmake.exe' -S . -B build -G Ninja "-DCMAKE_CXX_COMPILER=$compilerDir\clang++.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& 'C:\Program Files\CMake\bin\cmake.exe' --build build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& 'C:\Program Files\CMake\bin\ctest.exe' --test-dir build --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
.\build\glory_client.exe --smoke-test
```

SDL3 3.2.10 được CMake tải và build tự động ở lần configure đầu, nên cần kết nối mạng. Smoke test mở cửa sổ, xác nhận context 4.5 Core và chạy 120 tick trước khi tự thoát. Để mở cửa sổ tương tác, chạy `.\build\glory_client.exe` và nhấn Escape hoặc đóng cửa sổ để thoát.

Nguồn chính thức: Visual Studio / Build Tools (`visualstudio.microsoft.com`), CMake (`cmake.org`), Git for Windows (`git-scm.com`), RenderDoc (`renderdoc.org`).
