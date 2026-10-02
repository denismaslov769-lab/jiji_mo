@echo off
title Godjo Role Play - server
cd /d "%~dp0"
echo ================================================
echo   Godjo Role Play  -  server start
echo ================================================
echo.
echo  Ports: 7777/UDP (game), 7779/TCP+UDP (CEF UI)
echo  If players do not see the UI - run open-ports.bat as Administrator.
echo.
:loop
samp-server.exe
echo.
echo Server stopped. Restart in 5 seconds (Ctrl+C - exit)...
timeout /t 5 >nul
goto loop
