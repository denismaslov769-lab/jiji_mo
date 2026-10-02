# GodjoLauncher

C# .NET 8, WinForms + WebView2 (интерфейс — `ui/index.html`, встроен в exe).

Сборка (Windows):
```
certutil -decode app.ico.b64 app.ico
dotnet publish -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true
```
Иконка хранится в виде `app.ico.b64`, т.к. репозиторий заполняется текстовыми файлами.

`launcher.json` рядом с exe задаёт адрес `manifest.json`, IP и порт сервера. Если рядом лежит папка `client/` с пакетами — лаунчер ставит файлы из неё без интернета.

### 2.0
* Экран загрузки при «Играть», вкладка «Сборка» (вкл/выкл модов), проверка файлов, история ников, звуки.
* `launcher.json`: `manifestUrl`, `serverName`, `serverIp`, `serverPort`, `site`, `discord`, `vk`, `telegram`.
* Обмен с интерфейсом — JSON в camelCase (раньше PascalCase ломал статус GTA и показывал пустой «⚠»).
* Установщик — см. `installer/`.
