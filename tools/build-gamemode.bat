@echo off
rem Build gamemode: build-gamemode.bat [path to pawncc.exe]
set PAWNCC=%1
if "%PAWNCC%"=="" set PAWNCC=pawncc.exe
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-gamemode.ps1" -Pawncc "%PAWNCC%"
