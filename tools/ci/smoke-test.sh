#!/usr/bin/env bash
# Дымовой тест: запускает мод на Linux-сервере SA-MP 0.3.7 с заглушкой CEF-плагина
# и проверяет, что база данных создаётся и мод загружается без ошибок.
set -euo pipefail
AMX="$1"; ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
W="$(mktemp -d)"; cd "$W"
curl -fsSL --retry 3 -o svr.tgz "https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/master/samp037svr_R2-1.tar.gz"
tar xzf svr.tgz && cd samp03
gcc -m32 -shared -nostdlib -fPIC -O1 -o cef.so "$ROOT/tools/ci/cefstub.c"
mkdir -p plugins scriptfiles/godjo gamemodes
cp cef.so plugins/ && cp "$AMX" gamemodes/godjo.amx
for f in schema seed; do iconv -f UTF-8 -t CP1251 "$ROOT/database/$f.sql" > scriptfiles/godjo/$f.sql; done
cat > server.cfg <<CFG
lanmode 0
rcon_password smoketest123
maxplayers 50
port 17777
hostname smoke
gamemode0 godjo 1
announce 0
query 0
plugins cef.so
CFG
chmod +x samp03svr
timeout 15 ./samp03svr || true
iconv -f CP1251 -t UTF-8 server_log.txt | tee log.txt
grep -q "Мод загружен" log.txt
grep -q "Загружено домов: 14" log.txt
! grep -qi "run time error\|AMX backtrace" log.txt
echo "SMOKE TEST OK"
