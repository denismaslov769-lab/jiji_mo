# Полная сборка игры (скачивание GTA через лаунчер)

Если у игрока нет GTA San Andreas, лаунчер показывает **«Скачать игру»**: скачивает готовую сборку
(GTA SA + SA-MP), распаковывает её в `…\Godjo RP`, а затем сам ставит всю сборку Godjo
(ASI Loader, CEF, интерфейс, Modloader, 77 машин, текстуры). Загрузка докачивается после обрыва,
каждая часть проверяется по SHA-256.

## 1. Подготовить чистую копию игры
* GTA San Andreas **1.0 US** (`gta_sa.exe` 14 383 616 байт) — с неё работает SA-MP.
* Установить в эту копию **SA-MP 0.3.7-R1** (или R3).
* Модов Godjo туда класть не нужно — они ставятся отдельными пакетами и обновляются сами.

## 2. Упаковать
Положите `make-game-pack.bat` (есть в `GodjoRP-Launcher.zip`) рядом с `GodjoLauncher.exe` и запустите,
или из консоли:
```
GodjoLauncher.exe --make-game-pack "D:\GTA чистая" "D:\godjo-game" 1900
```
Получится `game.json` и части `game.zip.001`, `game.zip.002`, … по 1900 МБ.

## 3. Залить на хостинг
Все файлы из папки — в одну папку на хостинге с прямыми ссылками
(свой VPS/nginx, Cloudflare R2, Backblaze B2, Selectel S3 и т.п.).
Ссылки на части в `game.json` относительные — достаточно, чтобы они лежали рядом.
Хостинг должен поддерживать HTTP Range (докачку) — все перечисленные поддерживают.

## 4. Указать ссылку
В GitHub: **Settings → Secrets and variables → Actions → Variables → New variable**
`GODJO_GAME_MANIFEST` = `https://ваш-хостинг/godjo-game/game.json` и перезапустить workflow *build*.
Либо вручную в `launcher.json`: `"gameManifestUrl": "https://…/game.json"`.

> GitHub для хранения самой GTA не подходит: файлы игры там удалят по жалобе правообладателя.
