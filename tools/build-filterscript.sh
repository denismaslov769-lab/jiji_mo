#!/usr/bin/env bash
# Сборка filterscript (например, боты FCNPC): UTF-8 -> CP1251 и pawncc.
# Использование: PAWNCC=/path/to/pawncc tools/build-filterscript.sh godjo_bots out/godjo_bots.amx
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
NAME="$1"; OUT="${2:-$ROOT/build/server/filterscripts/$NAME.amx}"
PAWNCC="${PAWNCC:-pawncc}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cp -r "$ROOT/gamemode/include" "$TMP/include"
cp -r "$ROOT/gamemode/filterscripts/." "$TMP/"
find "$TMP" -type f \( -name '*.pwn' -o -name '*.inc' \) | while read -r f; do
  iconv -f UTF-8 -t CP1251 "$f" > "$f.cp" && mv "$f.cp" "$f"
done
mkdir -p "$(dirname "$OUT")"
OUT_ABS="$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")"
cd "$TMP"
"$PAWNCC" "$TMP/$NAME.pwn" "-i$TMP/include" "-i$TMP" "-o$OUT_ABS" -d0 -O1 "-;+"
echo "OK: $OUT_ABS"
