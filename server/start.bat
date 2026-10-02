@echo off
chcp 65001 >nul
title Godjo Role Play - сервер
cd /d "%~dp0"
echo ================================================
echo   Godjo Role Play  -  запуск сервера
echo ================================================
echo.
echo  Порты: 7777/UDP (игра), 7779/TCP+UDP (CEF-интерфейс)
echo  Если игроки не видят интерфейс - запустите open-ports.bat от имени администратора.
echo.
:loop
samp-server.exe
echo.
echo Сервер остановлен. Перезапуск через 5 секунд (Ctrl+C - выход)...
timeout /t 5 >nul
goto loop
