#!/usr/bin/env python3
"""Генерирует gamemode/modules/carlist.inc из mods/cars.csv (каталог автосалона).
Цена в игре = цена из CSV / 20 (округление до 500)."""
import os
root = os.path.join(os.path.dirname(__file__), "..")
CATS = ["economy", "middle", "premium", "sport", "suv", "truck", "moto"]
STOCK = [  # штатные модели, оставшиеся в салоне
    (466, "Glendale", 9000, "economy"), (401, "Bravura", 12000, "economy"), (410, "Manana", 7500, "economy"),
    (418, "Moonbeam", 14000, "economy"), (404, "Perenniel", 10000, "economy"), (405, "Sentinel", 28000, "middle"),
    (445, "Admiral", 26000, "middle"), (507, "Elegant", 30000, "middle"), (547, "Primo", 18000, "economy"),
    (550, "Sunrise", 22000, "middle"), (579, "Huntley", 60000, "suv"), (400, "Landstalker", 35000, "suv"),
    (560, "Sultan", 120000, "sport"), (562, "Elegy", 110000, "sport"), (411, "Infernus", 450000, "sport"),
    (462, "Faggio", 3500, "moto"), (521, "FCR-900", 40000, "moto"),
]
rows = list(STOCK)
for line in open(os.path.join(root, "mods", "cars.csv"), encoding="utf-8"):
    line = line.strip()
    if not line or line.startswith("#"): continue
    mid, gta, src, name, price, cat = line.split(";")
    rows.append((int(mid), name, max(500, round(int(price) / 20 / 500) * 500), cat))
rows.sort(key=lambda r: (CATS.index(r[3]), r[2]))
out = ["// АВТОГЕНЕРАЦИЯ: tools/gen-carlist.py из mods/cars.csv — не редактировать вручную", f"#define CAR_COUNT {len(rows)}",
       "new const gCarModel[CAR_COUNT] = {" + ", ".join(str(r[0]) for r in rows) + "};",
       "new const gCarPrice[CAR_COUNT] = {" + ", ".join(str(r[2]) for r in rows) + "};",
       "new const gCarCat[CAR_COUNT] = {" + ", ".join(str(CATS.index(r[3])) for r in rows) + "};",
       "new const gCarName[CAR_COUNT][32] = {"]
out.append(",\n".join('    "%s"' % r[1].replace('"', "'") for r in rows))
out.append("};")
open(os.path.join(root, "gamemode", "modules", "carlist.inc"), "w", encoding="utf-8").write("\n".join(out) + "\n")
print(len(rows), "cars")
