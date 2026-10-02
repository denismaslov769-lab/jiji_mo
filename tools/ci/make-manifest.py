#!/usr/bin/env python3
"""Формирует manifest.json для лаунчера: список пакетов с SHA-256."""
import hashlib, json, os, sys, datetime

out_dir, version = sys.argv[1], sys.argv[2]
server_ip = os.environ.get("GODJO_SERVER_IP", "127.0.0.1")
server_port = int(os.environ.get("GODJO_SERVER_PORT", "7777"))

import zipfile

# name, file, title, check, skipIfExists, category, optional, default, requires, removeDir, description
PACKAGES = [
    ("asiloader", "asi-loader.zip", "ASI Loader", "", "dinput8.dll;vorbisHooked.dll", "core", False, True, "", "",
     "Загрузчик .asi-плагинов (нужен CEF и Modloader)"),
    ("cef", "cef-client.zip", "CEF-плагин", "cef.asi", "", "core", False, True, "", "",
     "Браузерный интерфейс внутри игры"),
    ("ui", "godjo-ui.zip", "Интерфейс Godjo", "cef/assets/godjo/index.html", "", "core", False, True, "", "",
     "HUD, меню, телефон, инвентарь"),
    ("modloader", "modloader.zip", "Modloader", "modloader.asi", "modloader.asi", "core", False, True, "", "",
     "Подключает моды сборки без замены оригинальных файлов GTA"),
    ("cars", "godjo-cars.zip", "Автопак Godjo (77 машин)", "modloader/godjo_cars/turismo.dff", "", "build", True, True, "modloader", "modloader/godjo_cars",
     "Реальные автомобили: BMW, Mercedes, Audi, Lada, Porsche, Bugatti и др."),
    ("textures", "godjo-textures.zip", "Текстуры Godjo", "modloader/godjo_textures/txd/loadscs.txd", "", "build", True, True, "modloader", "modloader/godjo_textures",
     "Фирменные экраны загрузки и меню"),
]

TIPS = [
    "Анимации: /anims — сесть, лечь, станцевать",
    "В автосалоне Godjo Motors теперь 94 модели — 77 из них новые",
    "Пристегнитесь: /belt снижает урон в ДТП",
    "Передать деньги игроку рядом: /pay [id] [сумма]",
    "Сборку можно настроить во вкладке «Сборка» лаунчера",
    "Площадь Godjo у станции Unity — место встреч",
    "Капот, багажник, фары: /hood /trunk /lights",
    "Правила сервера: /rules, помощь: /help",
]

def sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest().upper()

pk = []
for name, file, title, check, skip, cat, opt, dflt, req, rmdir, desc in PACKAGES:
    p = os.path.join(out_dir, file)
    if not os.path.exists(p):
        print("skip missing", file); continue
    with zipfile.ZipFile(p) as z:
        unpacked = sum(i.file_size for i in z.infolist())
    pk.append({"name": name, "title": title, "description": desc, "category": cat, "url": file, "sha256": sha(p),
               "size": os.path.getsize(p), "unpacked": unpacked, "check": check, "skipIfExists": skip,
               "optional": opt, "default": dflt, "requires": req, "removeDir": rmdir})

news_path = os.path.join(os.path.dirname(__file__), "..", "..", "docs", "news.json")
news = json.load(open(news_path, encoding="utf-8")) if os.path.exists(news_path) else []
manifest = {
    "version": version,
    "built": datetime.datetime.utcnow().strftime("%Y-%m-%d %H:%M UTC"),
    "server": {"name": "Godjo Role Play", "ip": server_ip, "port": server_port},
    "news": news,
    "launcher": {"version": version.split("+")[0], "url": "https://github.com/" + os.environ.get("GITHUB_REPOSITORY", "denismaslov769-lab/jiji_mo") + "/releases/latest"},
    "packages": pk,
    "tips": TIPS,
    "gameManifestUrl": os.environ.get("GODJO_GAME_MANIFEST", ""),
}
with open(os.path.join(out_dir, "manifest.json"), "w", encoding="utf-8") as f:
    json.dump(manifest, f, ensure_ascii=False, indent=2)
print(json.dumps(manifest, ensure_ascii=False, indent=2))
