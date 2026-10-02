#!/usr/bin/env python3
"""Генерирует пакет «Текстуры Godjo»: свои экраны загрузки (loadscs.txd) для GTA SA.

Пишет TXD (RenderWare 3.6.0.3, платформа D3D9, X8R8G8B8) без сторонних утилит.
Результат кладётся в modloader/godjo_textures/txd/loadscs.txd — Modloader подменяет
оригинал «на лету», файлы игры не трогаются.

usage: make-textures.py <out.zip>
"""
import io, os, struct, sys, zipfile, math, random
from PIL import Image, ImageDraw, ImageFilter, ImageFont

RW = 0x1803FFFF
W = H = 512
GOLD, GOLD2 = (244, 196, 48), (255, 145, 0)

TIPS = [
    ("Добро пожаловать", "Godjo Role Play — твоя история начинается здесь"),
    ("Автосалон", "Более 75 новых автомобилей: от Lada Priora до Bugatti Chiron"),
    ("Работа", "Таксист, водитель автобуса, дальнобойщик — ищи в /gps"),
    ("Права", "Сдай экзамен в автошколе, чтобы не получить штраф"),
    ("Отыгрыш", "Используй /me, /do и /try — это RP, а не DM"),
    ("Анимации", "Команда /anims — сесть, лечь, станцевать, помахать"),
    ("Дома и бизнесы", "Копи на своё жильё — /help расскажет как"),
    ("Погода", "В штате теперь меняется погода и время суток"),
    ("Новая карта", "Площадь Godjo у станции Unity — встречаемся там"),
    ("Ремень", "Пристегнись: /belt — меньше урона в ДТП"),
    ("Администрация", "Вопрос? Пиши /report — помогут"),
    ("Лаунчер", "Сборку можно настроить во вкладке «Сборка»"),
    ("Игра в кости", "/dice и /coin — решай споры честно"),
    ("Сообщество", "Следи за новостями в лаунчере и соцсетях"),
    ("Godjo RP", "Играй честно. Уважай других. Наслаждайся."),
]

def font(size, bold=True):
    names = (["DejaVuSans-Bold.ttf", "LiberationSans-Bold.ttf", "arialbd.ttf"] if bold
             else ["DejaVuSans.ttf", "LiberationSans-Regular.ttf", "arial.ttf"])
    dirs = ["", "/usr/share/fonts/truetype/dejavu/", "/usr/share/fonts/dejavu/", "/usr/share/fonts/liberation/",
            "/usr/share/fonts/truetype/liberation/", "/usr/share/fonts/liberation-sans/", "C:/Windows/Fonts/"]
    for n in names:
        for d in dirs:
            try: return ImageFont.truetype(d + n, size)
            except OSError: pass
    return ImageFont.load_default()

def wrap(draw, text, f, width):
    words, lines, cur = text.split(), [], ""
    for w in words:
        t = (cur + " " + w).strip()
        if draw.textlength(t, font=f) <= width: cur = t
        else: lines.append(cur); cur = w
    if cur: lines.append(cur)
    return lines

def screen(i, title, text):
    rnd = random.Random(1000 + i)
    img = Image.new("RGB", (W, H))
    px = img.load()
    hue = [(18, 16, 30), (12, 22, 34), (30, 14, 18), (14, 26, 22)][i % 4]
    for y in range(H):
        for x in range(W):
            k = (x + y) / (W + H)
            px[x, y] = tuple(int(c * (1.4 - k) ) for c in hue)
    d = ImageDraw.Draw(img, "RGBA")
    # огни города
    for _ in range(140):
        x, y, r = rnd.randint(0, W), rnd.randint(int(H * .45), H), rnd.randint(1, 3)
        c = rnd.choice([GOLD, GOLD2, (255, 255, 255), (120, 180, 255)])
        d.ellipse((x - r, y - r, x + r, y + r), fill=c + (rnd.randint(60, 200),))
    # силуэт города
    x = 0
    while x < W:
        bw, bh = rnd.randint(18, 46), rnd.randint(40, 200)
        d.rectangle((x, H - bh, x + bw, H), fill=(6, 6, 10, 235))
        for wy in range(H - bh + 8, H - 6, 12):
            for wx in range(x + 4, x + bw - 4, 8):
                if rnd.random() < .3: d.rectangle((wx, wy, wx + 3, wy + 5), fill=GOLD + (150,))
        x += bw + rnd.randint(0, 6)
    # золотые полосы
    for k in range(3):
        o = 80 + k * 22
        d.polygon([(W - o - 160, 0), (W - o - 140, 0), (W - o - 380, H), (W - o - 400, H)], fill=GOLD + (40 - k * 10,))
    glow = Image.new("RGBA", (W, H), (0, 0, 0, 0)); gd = ImageDraw.Draw(glow)
    gd.text((34, 36), "GODJO", font=font(82), fill=GOLD2 + (255,))
    img.paste(glow.filter(ImageFilter.GaussianBlur(10)), (0, 0), glow.filter(ImageFilter.GaussianBlur(10)))
    d = ImageDraw.Draw(img, "RGBA")
    d.text((34, 36), "GODJO", font=font(82), fill=GOLD)
    d.text((40, 126), "ROLE  PLAY", font=font(30), fill=(240, 240, 245))
    d.rectangle((40, 172, 140, 176), fill=GOLD)
    # плашка с подсказкой
    d.rounded_rectangle((28, 300, W - 28, 420), radius=14, fill=(10, 10, 14, 200), outline=GOLD + (120,), width=2)
    d.text((48, 312), title.upper(), font=font(22), fill=GOLD)
    y = 344
    for line in wrap(d, text, font(18, False), W - 100)[:3]:
        d.text((48, y), line, font=font(18, False), fill=(225, 225, 232)); y += 24
    d.text((W - 120, H - 30), f"{i + 1:02d} / {len(TIPS)}", font=font(14, False), fill=(200, 200, 210, 180))
    return img

def chunk(t, data): return struct.pack("<III", t, len(data), RW) + data

def native(name, img):
    img = img.convert("RGB")
    r, g, b = img.split()
    a = Image.new("L", img.size, 255)
    bgra = Image.merge("RGBA", (b, g, r, a)).tobytes()   # D3DFMT_X8R8G8B8 = B,G,R,X в памяти
    s = struct.pack("<IBBH", 9, 0x02, 0x11, 0)
    s += name.encode().ljust(32, b"\0") + b"".ljust(32, b"\0")
    s += struct.pack("<II", 0x0600, 22)                   # raster 888, D3DFMT_X8R8G8B8
    s += struct.pack("<HHBBBB", img.width, img.height, 32, 1, 4, 0)
    s += struct.pack("<I", len(bgra)) + bgra
    return chunk(0x15, chunk(0x01, s) + chunk(0x03, b""))

def txd(textures):
    body = chunk(0x01, struct.pack("<HH", len(textures), 0))
    for n, im in textures: body += native(n, im)
    return chunk(0x16, body + chunk(0x03, b""))

def main():
    out = sys.argv[1]
    tex = [(f"loadsc{i}", screen(i, *TIPS[i])) for i in range(15)]
    title = screen(0, *TIPS[0])
    tex += [("title_pc_EU", title), ("title_pc_US", title)]
    data = txd(tex)
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        z.writestr("modloader/godjo_textures/txd/loadscs.txd", data)
        z.writestr("modloader/godjo_textures/readme.txt", "Godjo Role Play — экраны загрузки. Отключаются в лаунчере (вкладка «Сборка»).\r\n")
    if os.environ.get("PREVIEW"):
        tex[1][1].save(os.environ["PREVIEW"])
    print(out, len(data), "bytes txd,", os.path.getsize(out), "bytes zip")

if __name__ == "__main__":
    main()
