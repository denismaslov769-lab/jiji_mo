# GodjoSetup — установщик лаунчера

.NET Framework 4.8 (встроен в Windows 10/11), WinForms со своей отрисовкой. Внутрь вшит `payload.zip` (GodjoLauncher.exe + launcher.json).

Сборка (работает и на Linux):
```
dotnet build installer/GodjoSetup.csproj -c Release -p:Payload=путь/к/payload.zip
```
Ключи: `/S` — тихо, `/D=путь` — папка, `/uninstall` — удаление.
Ставит в профиль пользователя (HKCU), права администратора не нужны.
