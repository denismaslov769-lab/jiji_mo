@echo off
rem Godjo RP - game pack maker. Put this file next to GodjoLauncher.exe
echo.
echo  === Godjo RP: upakovka igry ===
echo.
set /p GTA="Papka s igroy (naprimer C:\Games\GTA San Andreas): "
set /p OUT="Kuda sohranit arhiv (naprimer D:\godjo-game): "
echo.
"%~dp0GodjoLauncher.exe" --make-game-pack "%GTA%" "%OUT%" 1900
echo.
echo  Gotovo. Zaley vse fayly iz "%OUT%" na hosting.
pause
