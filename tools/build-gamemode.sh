#!/usr/bin/env bash
# Сборка мода Godjo RP: перекодирует исходники UTF-8 -> CP1251 и компилирует pawncc.
# Использование: PAWNCC=/path/to/pawncc tools/build-gamemode.sh [out.amx]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/build/server/gamemodes/godjo.amx}"
PAWNCC="${PAWNCC:-pawncc}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
cp -r "$ROOT/gamemode/." "$TMP/"
find "$TMP" -type f \( -name '*.pwn' -o -name '*.inc' \) | while read -r f; do
  iconv -f UTF-8 -t CP1251 "$f" > "$f.cp" && mv "$f.cp" "$f"
done
mkdir -p "$(dirname "$OUT")"
OUT_ABS="$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")"
cd "$TMP"
"$PAWNCC" "$TMP/godjo.pwn" "-i$TMP/include" "-i$TMP" "-o$OUT_ABS" ${PAWN_DEBUG:--d0} -O1 "-;+"
echo "OK: $OUT_ABS"
