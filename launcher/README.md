# GodjoLauncher

C# .NET 8, WinForms + WebView2 (интерфейс — `ui/index.html`, встроен в exe).

Сборка (Windows):
```
certutil -decode app.ico.b64 app.ico
dotnet publish -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true
```
Иконка хранится в виде `app.ico.b64`, т.к. репозиторий заполняется текстовыми файлами.

`launcher.json` рядом с exe задаёт адрес `manifest.json`, IP и порт сервера. Если рядом лежит папка `client/` с пакетами — лаунчер ставит файлы из неё без интернета.
