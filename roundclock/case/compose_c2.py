"""Before/after sheet for the chosen lean-back option (C -> C2)."""
from PIL import Image, ImageDraw, ImageFont
import os
HERE = os.path.dirname(os.path.abspath(__file__))
F = "/usr/share/fonts/truetype/dejavu/"
ft = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 34); fh = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 28)
fb = ImageFont.truetype(F + "DejaVuSans.ttf", 19); fs = ImageFont.truetype(F + "DejaVuSans.ttf", 17)
fc = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 20)
ROWS = {
 "C": ("Before: C (rev 1)", ["Front rim 3.7 mm above table", "Wedge sole = visible chin",
        "48.8 W x 27.4 D x 51.3 H", "Footprint 24 x 27", "Jack under the puck"]),
 "C2": ("Now: C2 (rev 2)", ["Front rim 0.5 mm above table", "Table slices the tilted puck:",
        "  no added chin; face-on it's", "  a pure circle", "48.8 W x 31.6 D x 48.2 H",
        "Footprint 32 x 29 (more stable)", "Jack 4.1 mm further back"]),
}
VIEWS = [("front", "Front (level)"), ("faceon", "Face-on (12° down)"), ("side", "Side (right)"),
         ("side_cut", "Section (centre)"), ("rear", "Rear 3/4"), ("front34", "Front 3/4")]
T, CROP, LW, HEAD = 340, 22, 400, 120
W = LW + T*len(VIEWS) + 20; H = HEAD + 40 + T*len(ROWS) + 20
im = Image.new("RGB", (W, H), (246, 245, 241)); d = ImageDraw.Draw(im)
d.text((24, 20), "Option C, rev 2: chin minimised", font=ft, fill=(25, 25, 25))
d.text((24, 66), "Board lowered 3.2 mm, so its front rim nearly touches the table. The tilted puck's lower back is cut flat by the table "
       "and is the foot. The USB-A moved back into a deeper tail that stays inside the screen's own outline.", font=fs, fill=(60, 60, 60))
for i, (_, lab) in enumerate(VIEWS):
    w = d.textlength(lab, font=fc); d.text((LW + i*T + (T - w)/2, HEAD + 6), lab, font=fc, fill=(70, 70, 70))
for r, (o, (title, lines)) in enumerate(ROWS.items()):
    y = HEAD + 40 + r*T
    if r: d.line([(16, y), (W - 16, y)], fill=(215, 212, 205), width=2)
    d.text((24, y + 30), title, font=fh, fill=(20, 20, 20))
    for k, ln in enumerate(lines): d.text((28, y + 80 + k*32), ln, font=fb, fill=(55, 55, 55))
    for i, (v, _) in enumerate(VIEWS):
        t = Image.open(f"{HERE}/r/{o}_{v}.png").convert("RGB")
        t = t.crop((CROP, CROP, t.width - CROP, t.height - CROP)).resize((T, T), Image.LANCZOS)
        im.paste(t, (LW + i*T, y))
im.save(f"{HERE}/case_C2_rev2.png"); print(im.size)
