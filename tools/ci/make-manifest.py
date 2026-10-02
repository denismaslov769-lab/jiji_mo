#!/usr/bin/env python3
"""Формирует manifest.json для лаунчера: список пакетов с SHA-256."""
import hashlib, json, os, sys, datetime

out_dir, version = sys.argv[1], sys.argv[2]
server_ip = os.environ.get("GODJO_SERVER_IP", "127.0.0.1")
server_port = int(os.environ.get("GODJO_SERVER_PORT", "7777"))

PACKAGES = [
    # name, file, title, check, skipIfExists
    ("asiloader", "asi-loader.zip", "ASI Loader", "", "dinput8.dll;vorbisHooked.dll"),
    ("cef", "cef-client.zip", "CEF-плагин", "cef.asi", ""),
    ("ui", "godjo-ui.zip", "Интерфейс Godjo", "cef/assets/godjo/index.html", ""),
]

def sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest().upper()

pk = []
for name, file, title, check, skip in PACKAGES:
    p = os.path.join(out_dir, file)
    if not os.path.exists(p):
        print("skip missing", file); continue
    pk.append({"name": name, "title": title, "url": file, "sha256": sha(p), "size": os.path.getsize(p), "check": check, "skipIfExists": skip})

news_path = os.path.join(os.path.dirname(__file__), "..", "..", "docs", "news.json")
news = json.load(open(news_path, encoding="utf-8")) if os.path.exists(news_path) else []
manifest = {
    "version": version,
    "built": datetime.datetime.utcnow().strftime("%Y-%m-%d %H:%M UTC"),
    "server": {"name": "Godjo Role Play", "ip": server_ip, "port": server_port},
    "news": news,
    "launcher": {"version": version.split("+")[0], "url": "https://github.com/" + os.environ.get("GITHUB_REPOSITORY", "denismaslov769-lab/jiji_mo") + "/releases/latest"},
    "packages": pk,
}
with open(os.path.join(out_dir, "manifest.json"), "w", encoding="utf-8") as f:
    json.dump(manifest, f, ensure_ascii=False, indent=2)
print(json.dumps(manifest, ensure_ascii=False, indent=2))
