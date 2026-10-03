#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор собственных интерьеров Godjo (сейчас: больница All Saints).

Пишет:
  gamemode/modules/gen_interiors.inc   — объекты, материалы, таблички, актёры
  database/migrations/005.sql          — точки locations для больницы
и прогоняет валидатор:
  * мебель не пересекается между собой и со стенами;
  * мебель целиком внутри своей комнаты;
  * все точки/актёры стоят на свободном полу;
  * от входа (hospital_in) пешком (радиус игрока 0.3 м) достижимы все точки.
Запуск:  python3 tools/interiors/gen_interiors.py
Размеры моделей: tools/interiors/model_aabb.json (Model-Sizes-Plus, AABB min/max).
"""
import json, math, os, sys
from collections import deque

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
AABB = {int(k): v for k, v in json.load(open(os.path.join(os.path.dirname(__file__), "model_aabb.json"))).items()}

# ---------------------------------------------------------------- геометрия
X0, Y0, Z0 = 2600.0, -2900.0, 1500.0       # мировой угол здания и уровень пола
INT_ID, VW = 21, 7
L, M, S = 9.633, 3.211, 1.602             # длины стен 19447 / 19353 / 19431
WALL = {L: 19447, M: 19353, S: 19431}
T = 0.172                                 # толщина стены
H = 3.5                                   # высота стен
W = 9 * M                                 # 28.899
D = 2 * L                                 # 19.266
PR = 0.30                                 # радиус игрока для проверки проходимости

objects = []   # dict(model,x,y,z,rx,ry,rz,color,kind,room,box)
texts = []     # dict(x,y,z,rz,text,size,font,fsize,bold,fg,bg,align)
actors = []    # dict(skin,x,y,a,anim)
points = {}    # code -> (x,y,a,title)

def rot_box(model, rz):
    mnx, mny, mnz, mxx, mxy, mxz = AABB[model]
    r = int(round(rz)) % 360
    if r == 0:   bx = (mnx, mxx, mny, mxy)
    elif r == 90:  bx = (-mxy, -mny, mnx, mxx)
    elif r == 180: bx = (-mxx, -mnx, -mxy, -mny)
    elif r == 270: bx = (mny, mxy, -mxx, -mnx)
    else: raise ValueError("rz must be multiple of 90")
    return bx, (mnz, mxz)

def zbase(model):
    """z происхождения модели так, чтобы низ стоял на полу"""
    mnz = AABB[model][2]
    if -0.05 <= mnz <= 0.25: return Z0
    return Z0 - mnz

def wall(x1, y1, x2, y2, color, room="shell"):
    """стена по осевой линии отрезка (горизонтального или вертикального)"""
    ln = round(abs(x2 - x1) + abs(y2 - y1), 3)
    key = min(WALL, key=lambda k: abs(k - ln))
    if abs(key - ln) > 0.02: raise ValueError("wall length %s" % ln)
    cx, cy = (x1 + x2) / 2, (y1 + y2) / 2
    rz = 0.0 if abs(x2 - x1) < 1e-6 else 90.0
    if rz == 0: box = (cx - T / 2, cx + T / 2, cy - ln / 2, cy + ln / 2)
    else:       box = (cx - ln / 2, cx + ln / 2, cy - T / 2, cy + T / 2)
    objects.append(dict(model=WALL[key], x=cx, y=cy, z=Z0 + H / 2, rx=0, ry=0, rz=rz, color=color,
                        kind="wall", room=room, box=box, zr=(0, H)))

def slab(cx, cy, z, color, kind):
    objects.append(dict(model=19377, x=cx, y=cy, z=z, rx=0, ry=90.0, rz=0.0, color=color, kind=kind,
                        room="shell", box=None, zr=None))

def prop(model, x, y, rz, room, z=None, color=None, solid=True):
    bx, zr = rot_box(model, rz)
    zz = zbase(model) if z is None else Z0 + z
    box = (x + bx[0], x + bx[1], y + bx[2], y + bx[3])
    objects.append(dict(model=model, x=x, y=y, z=zz, rx=0, ry=0, rz=float(rz), color=color, kind="prop",
                        room=room, box=box, zr=(zz - Z0 + zr[0], zz - Z0 + zr[1]), solid=solid))

def sign(x, y, z, face, text, fsize=40, fg=0xFFFFFFFF, bg=0xFF2E7D6B, size="OBJECT_MATERIAL_SIZE_256x128", bold=1):
    """табличка-плоскость 19483 на стене; face — куда смотрит текст: N/S/E/W"""
    rz = {"S": 90.0, "N": 270.0, "E": 180.0, "W": 0.0}[face]
    off = {"S": (0, -1), "N": (0, 1), "E": (1, 0), "W": (-1, 0)}[face]
    texts.append(dict(x=x + off[0] * (T / 2 + 0.03), y=y + off[1] * (T / 2 + 0.03), z=Z0 + z, rz=rz, text=text,
                      fsize=fsize, fg=fg, bg=bg, size=size, bold=bold))

def actor(skin, x, y, a, anim=None):
    actors.append(dict(skin=skin, x=x, y=y, a=a, anim=anim))

def point(code, x, y, a, title):
    points[code] = (x, y, a, title)

# ---------------------------------------------------------------- цвета
C_LOBBY, C_FRONT, C_PART = 0xFFE9F3F1, 0xFFDDEFEA, 0xFFF2F2F2
C_R = [0xFFDCE9F5, 0xFFE6E1F2, 0xFFF4F0DC, 0xFFE2F0DF]
C_FLOOR, C_CEIL = 0xFFD5DADB, 0xFFF7F7F7
ROOMS = {  # (x1,x2,y1,y2) по осям стен
    "lobby": (0, W, 0, L), "R1": (0, 2 * M, L, D), "R2": (2 * M, 4 * M, L, D),
    "R3": (4 * M, 6 * M, L, D), "R4": (6 * M, W, L, D),
}

# ---------------------------------------------------------------- оболочка
for i in range(3): wall(i * L, 0, (i + 1) * L, 0, C_LOBBY)                    # южная (с входом)
for i in range(9):                                                            # северная
    wall(i * M, D, (i + 1) * M, D, C_R[min(i // 2, 3)] if i < 6 else C_R[3])
wall(0, 0, 0, L, C_LOBBY); wall(0, L, 0, D, C_R[0])                           # западная
wall(W, 0, W, L, C_LOBBY); wall(W, L, W, D, C_R[3])                           # восточная
for k in range(3):                                                            # фасад кабинетов
    a = k * 2 * M
    wall(a, L, a + M, L, C_FRONT); wall(a + M, L, a + M + S, L, C_FRONT)      # проём a+4.813..a+6.422
a = 6 * M
wall(a + 1.609, L, a + 1.609 + S, L, C_FRONT)                                 # палата: проём слева
wall(a + 1.609 + S, L, a + 1.609 + S + M, L, C_FRONT); wall(a + 1.609 + S + M, L, a + 1.609 + S + 2 * M, L, C_FRONT)
for k in (1, 2, 3): wall(k * 2 * M, L, k * 2 * M, D, C_PART)                  # перегородки
DOORS = [(4.813, 6.422), (11.235, 12.844), (17.657, 19.266), (19.266, 20.875)]
for cx in (5.25, 15.75, 26.25):
    for cy in (L / 2, 1.5 * L):
        slab(cx, cy, Z0 - T / 2, C_FLOOR, "floor"); slab(cx, cy, Z0 + H + T / 2, C_CEIL, "ceil")

# ---------------------------------------------------------------- холл
prop(2165, 4.0, 5.0, 0, "lobby"); prop(2165, 5.96, 5.0, 0, "lobby")          # стойка регистратуры
actor(308, 5.47, 6.1, 180.0)                                                  # медсестра
point("med_reception", 5.47, 4.2, 0.0, "Больница")
for x in (3.4, 7.6): prop(2773, x, 3.25, 0, "lobby")                          # ограждения очереди
for x in (17.5, 21.0, 24.5):
    prop(1723, x, 1.2, 180, "lobby"); prop(1723, x, 4.0, 180, "lobby")       # ряды кресел ожидания
prop(1775, 28.25, 2.0, 270, "lobby"); prop(1776, 28.25, 3.4, 270, "lobby")    # автоматы
prop(1808, 28.45, 7.6, 270, "lobby")                                          # кулер
prop(2001, 0.65, 0.65, 0, "lobby"); prop(2001, 0.65, 8.95, 0, "lobby"); prop(948, 28.5, 8.95, 0, "lobby")
prop(2690, 15.3, L - T / 2 - 0.15, 0, "lobby", z=1.2, solid=False)            # огнетушитель на стене
actor(78, 18.0, 2.6, 0.0, ("PED", "XPRESSscratch")); actor(196, 23.2, 2.6, 0.0, ("PED", "IDLE_tired"))
point("hospital_in", 14.45, 1.2, 0.0, "Выход из больницы")
point("hospital_lobby", 11.0, 5.0, 90.0, "Холл больницы")
sign(8.0, L, 2.75, "S", "+ ALL SAINTS", 48, 0xFFFFFFFF, 0xFFC0392B)
sign(14.45, 0, 2.75, "N", "ВЫХОД", 44, 0xFFFFFFFF, 0xFF1E8449)
sign(4.97, L, 2.25, "S", "Регистратура", 34)
sign(11.0, 0, 1.9, "N", "Часы приёма\n08:00 - 20:00\nПриёмный покой\nкруглосуточно", 22, 0xFF1B2631, 0xFFF4F6F6,
     "OBJECT_MATERIAL_SIZE_256x256", 1)
for (d1, d2), txt in zip(DOORS, ["Кабинет 1\nТерапевт", "Кабинет 2\nОкулист", "Кабинет 3\nЛаборатория", "Палата №1"]):
    sign((d1 + d2) / 2, L, 2.95, "S", txt, 26, 0xFFFFFFFF, 0xFF1F618D)

# ---------------------------------------------------------------- R1 терапевт
prop(2165, 2.2, 15.0, 0, "R1"); actor(275, 2.7, 16.2, 180.0)
prop(1997, 0.65, 11.6, 0, "R1")                                               # кушетка
prop(2161, 3.6, D - T / 2 - 0.03, 0, "R1"); prop(2167, 5.4, D - T / 2 - 0.03, 0, "R1")
prop(948, 0.45, 18.8, 0, "R1")
point("med_ther", 3.9, 13.4, 0.0, "Больница")
sign(2.6, D, 2.4, "S", "Терапевт\nСмирнова Е.А.", 28, 0xFF1B2631, 0xFFFDFEFE)

# ---------------------------------------------------------------- R2 окулист
sign(9.6, D, 1.75, "S", "Ш  Б\nМ  Н  К\nЫ  М  Б  Ш\nБ  Ы  Н  К  М\nИ  Н  Ш  М  К", 28, 0xFF111111, 0xFFF5F5F5,
     "OBJECT_MATERIAL_SIZE_256x256", 1)
prop(2165, 12.25, 16.5, 270, "R2"); actor(70, 8.0, 17.5, 225.0)
prop(2163, 7.3, D - T / 2 - 0.03, 0, "R2"); prop(2001, 12.15, 18.55, 0, "R2")
point("med_eye", 9.6, 13.0, 0.0, "Больница")
sign(9.6, L, 2.6, "N", "Окулист\nКим Д.С.", 28, 0xFF1B2631, 0xFFFDFEFE)

# ---------------------------------------------------------------- R3 лаборатория
prop(2357, 16.05, 15.2, 0, "R3")
prop(2200, 13.5, D - T / 2 - 0.16, 0, "R3"); prop(2200, 15.9, D - T / 2 - 0.16, 0, "R3")
prop(2167, 18.3, D - T / 2 - 0.03, 0, "R3"); prop(948, 13.35, 10.2, 0, "R3")
actor(276, 16.05, 16.6, 180.0)
point("med_lab", 16.05, 13.6, 0.0, "Больница")
sign(16.05, L, 2.6, "N", "Лаборатория\nГарсия М.", 28, 0xFF1B2631, 0xFFFDFEFE)

# ---------------------------------------------------------------- R4 палата
for x in (21.0, 23.2, 25.4, 27.6): prop(1997, x, 18.0, 0, "R4")
prop(2146, 28.2, 12.6, 0, "R4"); prop(1808, 28.45, 10.25, 270, "R4")
actor(274, 24.3, 14.5, 200.0, ("COP_AMBIENT", "Copbrowse_loop"))
point("hospital_spawn", 22.1, 15.6, 0.0, "Палата больницы")
sign(24.3, L, 2.4, "N", "Палата №1\nНе шуметь", 28, 0xFF1B2631, 0xFFFDFEFE)

# ---------------------------------------------------------------- валидатор
errors = []
def inner(room):
    x1, x2, y1, y2 = ROOMS[room]
    return (x1 + T / 2, x2 - T / 2, y1 + T / 2, y2 - T / 2)
def overlap(a, b, eps=0.005):
    return a[0] < b[1] - eps and b[0] < a[1] - eps and a[2] < b[3] - eps and b[2] < a[3] - eps

props = [o for o in objects if o["kind"] == "prop"]
walls = [o for o in objects if o["kind"] == "wall"]
for i, o in enumerate(props):
    r = inner(o["room"])
    b = o["box"]
    if b[0] < r[0] - 0.005 or b[1] > r[1] + 0.005 or b[2] < r[2] - 0.005 or b[3] > r[3] + 0.005:
        errors.append("model %d @(%.2f,%.2f) выходит за комнату %s: %s vs %s" % (o["model"], o["x"], o["y"], o["room"],
                      tuple(round(v, 2) for v in b), tuple(round(v, 2) for v in r)))
    for w in walls:
        if overlap(b, w["box"]): errors.append("model %d @(%.2f,%.2f) в стене" % (o["model"], o["x"], o["y"]))
    for o2 in props[i + 1:]:
        if overlap(b, o2["box"]): errors.append("model %d и %d пересекаются" % (o["model"], o2["model"]))
    if o["zr"][0] < -0.01: errors.append("model %d ниже пола" % o["model"])

# сетка проходимости
G = 0.05
NX, NY = int(W / G) + 1, int(D / G) + 1
blocked = bytearray(NX * NY)
def mark(b, pad):
    x1 = max(0, int((b[0] - pad) / G)); x2 = min(NX - 1, int((b[1] + pad) / G) + 1)
    y1 = max(0, int((b[2] - pad) / G)); y2 = min(NY - 1, int((b[3] + pad) / G) + 1)
    for gx in range(x1, x2 + 1):
        for gy in range(y1, y2 + 1): blocked[gx * NY + gy] = 1
for w in walls: mark(w["box"], PR)
for o in props:
    if o.get("solid", True) and o["zr"][0] < 1.6: mark(o["box"], PR)
for a in actors: mark((a["x"] - 0.25, a["x"] + 0.25, a["y"] - 0.25, a["y"] + 0.25), PR)
def cell(x, y): return int(round(x / G)), int(round(y / G))
sx, sy = cell(*points["hospital_in"][:2])
seen = bytearray(NX * NY)
if blocked[sx * NY + sy]: errors.append("вход заблокирован")
q = deque([(sx, sy)]); seen[sx * NY + sy] = 1
while q:
    cx, cy = q.popleft()
    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        nx, ny = cx + dx, cy + dy
        if 0 <= nx < NX and 0 <= ny < NY and not seen[nx * NY + ny] and not blocked[nx * NY + ny]:
            seen[nx * NY + ny] = 1; q.append((nx, ny))
for code, (x, y, a, t) in points.items():
    gx, gy = cell(x, y)
    if blocked[gx * NY + gy]: errors.append("точка %s стоит в препятствии" % code)
    elif not seen[gx * NY + gy]: errors.append("точка %s недостижима от входа" % code)
for a in actors:  # к каждому актёру можно подойти на 1.5 м
    ok = False
    for gx in range(NX):
        if ok: break
        for gy in range(NY):
            if seen[gx * NY + gy] and math.hypot(gx * G - a["x"], gy * G - a["y"]) < 1.5: ok = True; break
    if not ok: errors.append("к актёру skin %d не подойти" % a["skin"])
    r = [k for k, v in ROOMS.items() if v[0] < a["x"] < v[1] and v[2] < a["y"] < v[3]]
    for o in props:
        b = o["box"]
        if o.get("solid", True) and b[0] - 0.2 < a["x"] < b[1] + 0.2 and b[2] - 0.2 < a["y"] < b[3] + 0.2 and o["zr"][0] < 1.6:
            errors.append("актёр skin %d внутри model %d" % (a["skin"], o["model"]))

if errors:
    print("ВАЛИДАЦИЯ НЕ ПРОЙДЕНА:"); [print("  -", e) for e in errors]; sys.exit(1)
print("валидатор: ок (%d объектов, %d табличек, %d актёров, %d точек)" % (len(objects), len(texts), len(actors), len(points)))

# ---------------------------------------------------------------- вывод Pawn
def f(v): return ("%.3f" % v).rstrip("0").rstrip(".") if "." in "%.3f" % v else "%.3f" % v
def fl(v): s = "%.3f" % v; return s
def pstr(s): return s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
out = []
out.append("// =====================================================================")
out.append("//  СГЕНЕРИРОВАНО tools/interiors/gen_interiors.py — не править руками.")
out.append("//  Больница All Saints: собственный интерьер (int %d, мир %d)." % (INT_ID, VW))
out.append("// =====================================================================")
out.append("#define GENINT_HOSP_INT %d" % INT_ID)
out.append("#define GENINT_HOSP_X %s" % fl(X0 + W / 2))
out.append("#define GENINT_HOSP_Y %s" % fl(Y0 + D / 2))
out.append("#define GENINT_HOSP_Z %s" % fl(Z0 + 1.0))
out.append("#define GENINT_DD 150.0")
out.append("// {model, x, y, z, rx, ry, rz, color(0 = без перекраски)}")
out.append("static const Float:gGenHospObj[][8] = {")
rows = []
for o in objects:
    rows.append("    {%d.0, %s, %s, %s, %s, %s, %s, %d.0}" % (o["model"], fl(X0 + o["x"]), fl(Y0 + o["y"]), fl(o["z"]),
                fl(o["rx"]), fl(o["ry"]), fl(o["rz"]), 1 if o["color"] else 0))
out.append(",\n".join(rows)); out.append("};")
out.append("static const gGenHospColor[] = {")
out.append(",\n".join("    0x%08X" % (o["color"] or 0) for o in objects)); out.append("};")
out.append("static const Float:gGenHospText[][4] = {")
out.append(",\n".join("    {%s, %s, %s, %s}" % (fl(X0 + t["x"]), fl(Y0 + t["y"]), fl(t["z"]), fl(t["rz"])) for t in texts)); out.append("};")
out.append("static const gGenHospTextStr[][] = {")
out.append(",\n".join('    "%s"' % pstr(t["text"]) for t in texts)); out.append("};")
out.append("static const gGenHospTextFmt[][4] = {  // {размер шрифта, жирный, цвет, фон}")
out.append(",\n".join("    {%d, %d, 0x%08X, 0x%08X}" % (t["fsize"], t["bold"], t["fg"], t["bg"]) for t in texts)); out.append("};")
out.append("static const gGenHospTextSize[] = {")
out.append(",\n".join("    %s" % t["size"] for t in texts)); out.append("};")
out.append("// актёры: {скин, x, y, z, угол}")
out.append("static const Float:gGenHospActor[][5] = {")
out.append(",\n".join("    {%d.0, %s, %s, %s, %s}" % (a["skin"], fl(X0 + a["x"]), fl(Y0 + a["y"]), fl(Z0 + 1.0), fl(a["a"])) for a in actors)); out.append("};")
out.append("static const gGenHospAnim[][2][16] = {")
out.append(",\n".join('    {"%s", "%s"}' % (a["anim"] or ("", "")) for a in actors)); out.append("};")
out.append("""
stock GenInt_Hospital(vw)
{
    for (new i = 0; i < sizeof gGenHospObj; i++)
    {
        new o = CreateObject(floatround(gGenHospObj[i][0]), gGenHospObj[i][1], gGenHospObj[i][2], gGenHospObj[i][3],
            gGenHospObj[i][4], gGenHospObj[i][5], gGenHospObj[i][6], GENINT_DD);
        if (o == INVALID_OBJECT_ID) { printf("[INT] Не хватило объектов для больницы (%d/%d)", i, sizeof gGenHospObj); break; }
        if (gGenHospColor[i]) SetObjectMaterial(o, 0, 10765, "airportgnd_sfse", "white", gGenHospColor[i]);
    }
    for (new i = 0; i < sizeof gGenHospText; i++)
    {
        new o = CreateObject(19483, gGenHospText[i][0], gGenHospText[i][1], gGenHospText[i][2], 0.0, 0.0, gGenHospText[i][3], GENINT_DD);
        if (o == INVALID_OBJECT_ID) break;
        SetObjectMaterialText(o, gGenHospTextStr[i], 0, gGenHospTextSize[i], "Arial", gGenHospTextFmt[i][0], gGenHospTextFmt[i][1],
            gGenHospTextFmt[i][2], gGenHospTextFmt[i][3], OBJECT_MATERIAL_TEXT_ALIGN_CENTER);
    }
    for (new i = 0; i < sizeof gGenHospActor; i++)
    {
        new a = CreateActor(floatround(gGenHospActor[i][0]), gGenHospActor[i][1], gGenHospActor[i][2], gGenHospActor[i][3], gGenHospActor[i][4]);
        if (a == INVALID_ACTOR_ID) continue;
        SetActorVirtualWorld(a, vw);
        SetActorInvulnerable(a, true);
        if (gGenHospAnim[i][0][0]) ApplyActorAnimation(a, gGenHospAnim[i][0], gGenHospAnim[i][1], 4.1, 1, 0, 0, 0, 0);
    }
    printf("[INT] Больница: %d объектов, %d табличек, %d актёров", sizeof gGenHospObj, sizeof gGenHospText, sizeof gGenHospActor);
}
""")
open(os.path.join(ROOT, "gamemode/modules/gen_interiors.inc"), "w", encoding="utf-8").write("\n".join(out))

sql = ["-- 005: больница All Saints получает собственный интерьер (int %d, мир %d)," % (INT_ID, VW),
       "-- точки сгенерированы tools/interiors/gen_interiors.py"]
for code, (x, y, a, t) in points.items():
    sql.append("INSERT OR REPLACE INTO locations VALUES ('%s',%.2f,%.2f,%.2f,%d,%d,%d,'%s');" % (code, X0 + x, Y0 + y, Z0 + 1.0, a, INT_ID, VW, t))
sql.append("ALTER TABLE houses ADD COLUMN interior INTEGER DEFAULT -1;")
open(os.path.join(ROOT, "database/migrations/005.sql"), "w", encoding="utf-8").write("\n".join(sql) + "\n")
print("записано: gamemode/modules/gen_interiors.inc, database/migrations/005.sql")
