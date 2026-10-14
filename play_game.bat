@echo off
cd /d "%~dp0"
echo ====================================================
echo   KHOI CHAY GAME GLORY - 3D ACTION MMORPG CLIENT
echo ====================================================
echo Dang ket noi toi Dedicated Server tai 127.0.0.1:7777...
echo (Luu y: Nhan F8 trong game de mo Feedback Form)
echo.
build\glory_client.exe --connect 127.0.0.1:7777
pause
