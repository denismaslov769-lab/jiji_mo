@echo off
cd /d "%~dp0"
net session >nul 2>&1
if errorlevel 1 (
  echo Run this file as Administrator: right click - "Run as administrator".
  pause
  exit /b 1
)
echo Opening Godjo RP ports in Windows Firewall...
netsh advfirewall firewall delete rule name="Godjo SA-MP UDP 7777" >nul 2>&1
netsh advfirewall firewall delete rule name="Godjo CEF TCP 7779" >nul 2>&1
netsh advfirewall firewall delete rule name="Godjo CEF UDP 7779" >nul 2>&1
netsh advfirewall firewall add rule name="Godjo SA-MP UDP 7777" dir=in action=allow protocol=UDP localport=7777
netsh advfirewall firewall add rule name="Godjo CEF TCP 7779" dir=in action=allow protocol=TCP localport=7779
netsh advfirewall firewall add rule name="Godjo CEF UDP 7779" dir=in action=allow protocol=UDP localport=7779
netsh advfirewall firewall add rule name="Godjo samp-server.exe" dir=in action=allow program="%~dp0samp-server.exe" enable=yes
echo Done.
pause
