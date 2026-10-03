#!/usr/bin/env python3
"""Пакет «Экран загрузки Godjo»: вместо заставки SA-MP (логотипы SA-MP / EAX / nVidia / RakNet)
показывается анимированный экран загрузки Godjo Role Play.

Заставку подменяет ASI-плагин samp-custom-loading-screen (github.com/vendder5/samp-custom-loading-screen):
он рисует картинку loadscs/loading_screen.(png|gif) поверх стандартного экрана, пока игра грузится.

usage: make-loadscreen.py <custom-loading-screen.asi> <out.zip> [preview.png]
"""
import io, os, sys, zipfile, random, math, base64
from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 1280, 720
GOLD, GOLD2 = (255, 201, 77), (232, 169, 58)
FRAMES = 24

def font(size, bold=True):
    names = (["DejaVuSans-Bold.ttf", "LiberationSans-Bold.ttf", "arialbd.ttf"] if bold
             else ["DejaVuSans.ttf", "LiberationSans-Regular.ttf", "arial.ttf"])
    dirs = ["", "/usr/share/fonts/truetype/dejavu/", "/usr/share/fonts/dejavu/", "/usr/share/fonts/liberation/",
            "/usr/share/fonts/truetype/liberation/", "/usr/share/fonts/liberation-sans/", "/usr/share/fonts/truetype/msttcorefonts/", "C:/Windows/Fonts/"]
    for n in names:
        for d in dirs:
            try: return ImageFont.truetype(d + n, size)
            except OSError: pass
    return ImageFont.load_default()

def background():
    rnd = random.Random(7)
    img = Image.new("RGB", (W, H), (8, 9, 13))
    # мягкое золотое свечение сверху
    glow = Image.new("RGB", (W, H), (0, 0, 0)); gd = ImageDraw.Draw(glow)
    gd.ellipse((W * .15, -H * .55, W * .85, H * .55), fill=(70, 52, 14))
    img = Image.blend(img, glow.filter(ImageFilter.GaussianBlur(120)), .9)
    d = ImageDraw.Draw(img, "RGBA")
    # огни и силуэт города
    for _ in range(260):
        x, y, r = rnd.randint(0, W), rnd.randint(int(H * .55), H), rnd.choice([1, 1, 2, 2, 3])
        d.ellipse((x - r, y - r, x + r, y + r), fill=rnd.choice([GOLD, GOLD2, (255, 255, 255), (120, 180, 255)]) + (rnd.randint(40, 150),))
    x = 0
    while x < W:
        bw, bh = rnd.randint(30, 80), rnd.randint(60, 300)
        d.rectangle((x, H - bh, x + bw, H), fill=(5, 5, 9, 240))
        for wy in range(H - bh + 10, H - 8, 16):
            for wx in range(x + 6, x + bw - 6, 11):
                if rnd.random() < .25: d.rectangle((wx, wy, wx + 4, wy + 7), fill=GOLD + (110,))
        x += bw + rnd.randint(0, 8)
    # затемнение снизу под полосу загрузки
    shade = Image.new("L", (1, H)); [shade.putpixel((0, y), int(max(0, (y - H * .55) / (H * .45)) * 170)) for y in range(H)]
    img.paste((4, 4, 7), (0, 0, W, H), shade.resize((W, H)))
    # персонажи штата (рендер моделей GTA SA: бездомные, банды, полиция, медики, бизнес) — tools/ci/art/crew.webp
    crew_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "art", "crew.webp.b64")
    if os.path.exists(crew_path):
        crew = Image.open(io.BytesIO(base64.b64decode(open(crew_path).read()))).convert("RGBA").resize((W, H))
        rim = Image.new("RGBA", (W, H), GOLD + (0,)); rim.putalpha(crew.getchannel("A").filter(ImageFilter.GaussianBlur(14)).point(lambda v: v * .55))
        img = img.convert("RGBA"); img.alpha_composite(rim); img.alpha_composite(crew)
        fade = Image.new("L", (1, H)); [fade.putpixel((0, y), int(max(0, (y - H * .78) / (H * .22)) * 235)) for y in range(H)]
        img = img.convert("RGB"); img.paste((4, 4, 7), (0, 0, W, H), fade.resize((W, H)))
    # логотип с подсветкой
    lg = Image.new("RGBA", (W, H), (0, 0, 0, 0)); ld = ImageDraw.Draw(lg)
    f1, f2 = font(118), font(30)
    t1 = "GODJO"; w1 = ld.textlength(t1, font=f1)
    ld.text(((W - w1) / 2, 170), t1, font=f1, fill=GOLD + (255,))
    img.paste(lg.filter(ImageFilter.GaussianBlur(18)), (0, 0), lg.filter(ImageFilter.GaussianBlur(18)))
    d = ImageDraw.Draw(img, "RGBA")
    d.text(((W - w1) / 2, 170), t1, font=f1, fill=GOLD)
    t2 = "R O L E   P L A Y"; w2 = d.textlength(t2, font=f2)
    d.text(((W - w2) / 2, 312), t2, font=f2, fill=(240, 240, 245))
    d.rectangle((W / 2 - 60, 366, W / 2 + 60, 370), fill=GOLD)
    f3 = font(18, False); t3 = "Начни с самого дна — поднимись на вершину"; w3 = d.textlength(t3, font=f3)
    d.text(((W - w3) / 2, 392), t3, font=f3, fill=(170, 175, 190))
    return img

def frame(bg, k):
    img = bg.copy(); d = ImageDraw.Draw(img, "RGBA")
    # полоса загрузки с бегущим бликом
    bx, by, bw, bh = W / 2 - 300, 600, 600, 8
    d.rounded_rectangle((bx, by, bx + bw, by + bh), radius=4, fill=(255, 255, 255, 26))
    t = k / FRAMES
    seg = 180
    x0 = bx - seg + (bw + seg) * t
    for i in range(seg):
        a = math.sin(math.pi * i / seg)
        xx = x0 + i
        if bx <= xx <= bx + bw:
            d.line((xx, by + 1, xx, by + bh - 1), fill=(int(GOLD2[0] + (GOLD[0] - GOLD2[0]) * a), int(GOLD2[1] + (GOLD[1] - GOLD2[1]) * a), int(GOLD2[2] + (GOLD[2] - GOLD2[2]) * a), int(255 * a)))
    # три пульсирующие точки
    f = font(20, False); txt = "Загрузка мира San Andreas"; tw = d.textlength(txt, font=f)
    d.text((W / 2 - tw / 2 - 14, 562), txt, font=f, fill=(225, 226, 232))
    for i in range(3):
        a = (math.sin(2 * math.pi * (t - i * .15)) + 1) / 2
        cx = W / 2 + tw / 2 - 4 + i * 11
        d.ellipse((cx - 3, 574 - 3, cx + 3, 574 + 3), fill=GOLD + (int(70 + 185 * a),))
    f4 = font(14, False); t4 = "godjo role play · sa-mp 0.3.7"; w4 = d.textlength(t4, font=f4)
    d.text(((W - w4) / 2, H - 40), t4, font=f4, fill=(120, 124, 136))
    return img

def main():
    asi, out = sys.argv[1], sys.argv[2]
    bg = background()
    frames = [frame(bg, k) for k in range(FRAMES)]
    pal = frames[0].quantize(colors=255, method=Image.Quantize.MEDIANCUT)
    q = [f.quantize(palette=pal, dither=Image.Dither.NONE) for f in frames]
    buf = io.BytesIO()
    q[0].save(buf, "GIF", save_all=True, append_images=q[1:], duration=60, loop=0, optimize=False, disposal=1)
    cfg = "IMAGE_URL=\r\nFIT_MODE=FILL\r\n"
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        z.write(asi, "custom-loading-screen.asi")
        z.writestr("loadscs_config.cfg", cfg)
        z.writestr("loadscs/loading_screen.gif", buf.getvalue())
    if len(sys.argv) > 3: frames[FRAMES // 2].save(sys.argv[3])
    print(out, "gif", len(buf.getvalue()), "bytes")

if __name__ == "__main__":
    main()
