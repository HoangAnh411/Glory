@echo off
cd /d "%~dp0"
echo ====================================================
echo   KHOI CHAY GAME GLORY - 3D ACTION MMORPG
echo ====================================================
echo.
echo [1] Choi Offline Sandbox (Khong can Server, vao chem quai ngay)
echo [2] Choi Online Multiplayer (Tu dong bat Server va ket noi Client)
echo.
set /p choice="Chon che do (1 hoac 2, mac dinh la 1): "
if "%choice%"=="2" (
    echo.
    echo Dang khoi dong Glory Dedicated Server...
    start "Glory Server" build\glory_server.exe --port 7777
    timeout /t 2 >nul
    echo Dang ket noi Client toi Dedicated Server tai 127.0.0.1:7777...
    build\glory_client.exe --connect 127.0.0.1:7777
) else (
    echo.
    echo Dang khoi dong Glory Client che do Offline Sandbox...
    build\glory_client.exe
)
pause
