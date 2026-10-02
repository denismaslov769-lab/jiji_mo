@echo off
chcp 65001 >nul
rem  Упаковка полной сборки GTA SA + SA-MP для лаунчера Godjo.
rem  Положите этот файл рядом с GodjoLauncher.exe и запустите.
set /p GTA="Папка с ЧИСТОЙ GTA San Andreas 1.0 + SA-MP 0.3.7-R1/R3: "
set /p OUT="Куда сложить части архива (например D:\godjo-game): "
"%~dp0GodjoLauncher.exe" --make-game-pack "%GTA%" "%OUT%" 1900
echo.
echo Загрузите все файлы из "%OUT%" (game.json + game.zip.001, .002 ...) на хостинг.
pause
