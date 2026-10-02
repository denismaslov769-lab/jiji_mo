# Godjo Role Play

Полноценный RP-мод для **SA-MP 0.3.7**, написанный с нуля, с собственным CEF-интерфейсом и лаунчером.
Игрок начинает бездомным у вокзала Юнити и проходит сюжет «С самого дна» вместе с Дядей Вовой.

| Папка | Что внутри |
|---|---|
| `gamemode/` | Исходники мода на Pawn (UTF-8, при сборке перекодируются в CP1251) |
| `database/` | `schema.sql`, `seed.sql` — база SQLite создаётся сама при первом запуске |
| `cef/godjo/` | Интерфейс (HTML/CSS/JS): вход, создание персонажа, HUD, инвентарь, телефон, банк, казино, рыбалка… |
| `launcher/` | Лаунчер `GodjoLauncher.exe` (C# .NET 8 + WebView2) |
| `server/` | `server.cfg`, `start.bat`, `open-ports.bat` |
| `vendor/` | `samp-server.exe` 0.3.7-R2-2-1 (в base64, собирается в CI) |
| `tools/` | Скрипты сборки и CI |
| `docs/` | Документация |

## Как получить готовые файлы
Всё собирается автоматически в **GitHub Actions** (вкладка *Actions* → workflow *build*) при каждом пуше в `main`.
Результат публикуется в **Releases**:

* `GodjoRP-Server.zip` — сервер для Windows: распаковать → `start.bat`.
* `GodjoRP-Launcher.zip` — лаунчер для игроков (внутри уже есть все файлы клиента).
* `GodjoLauncher.exe` — только лаунчер (файлы скачает из релиза сам).

Подробно: [docs/SERVER.md](docs/SERVER.md) · [docs/PLAYER.md](docs/PLAYER.md) · [docs/SYSTEMS.md](docs/SYSTEMS.md) · [docs/COMMANDS.md](docs/COMMANDS.md)

## Важно
* Интерфейс работает через [samp-cef](https://github.com/ZOTTCE/samp-cef): **клиент игрока — SA-MP 0.3.7-R1 или 0.3.7-R3** (R2 клиент плагином не поддерживается). Сервер — ваш 0.3.7-R2-2-1, это нормально.
* Для CEF нужен дополнительный порт: **игровой порт + 2** (по умолчанию `7779` TCP/UDP).
* Лаунчер скачивает файлы из Releases. Пока репозиторий приватный, игроки не смогут их скачать — раздавайте `GodjoRP-Launcher.zip` вручную или сделайте репозиторий/релизы публичными.
