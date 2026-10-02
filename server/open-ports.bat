@echo off
chcp 65001 >nul
echo Открываю порты Godjo RP в брандмауэре Windows (нужны права администратора)...
netsh advfirewall firewall add rule name="Godjo SA-MP UDP 7777" dir=in action=allow protocol=UDP localport=7777
netsh advfirewall firewall add rule name="Godjo CEF TCP 7779" dir=in action=allow protocol=TCP localport=7779
netsh advfirewall firewall add rule name="Godjo CEF UDP 7779" dir=in action=allow protocol=UDP localport=7779
echo Готово.
pause
