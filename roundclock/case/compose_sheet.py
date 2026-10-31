"""Compose r/<opt>_<view>.png tiles into one labelled comparison sheet."""
from PIL import Image, ImageDraw, ImageFont
import os
HERE = os.path.dirname(os.path.abspath(__file__))
F = "/usr/share/fonts/truetype/dejavu/"
f_title = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 34)
f_h = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 30)
f_b = ImageFont.truetype(F + "DejaVuSans.ttf", 19)
f_s = ImageFont.truetype(F + "DejaVuSans.ttf", 17)
f_col = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 20)

OPTS = {
 "A": ("A  Puck + hidden tail",
       ["48.8 OD x 14.8 deep puck; a keel-shaped",
        "tail carries the USB-A back to 27 mm",
        "Front: pure circle (small foot below)",
        "Footprint 22 x 27 mm",
        "+ thinnest-looking from the front/side",
        "- tail adds a 'step' in the profile"]),
 "B": ("B  Deep can",
       ["One plain 48.8 x 27 mm cylinder;",
        "jack in a boss at the bottom of the back",
        "Front: pure circle, 0.5 mm flat on base",
        "Footprint 10 x 27 mm (narrowest)",
        "+ simplest, cleanest object",
        "- deepest-looking; tips over most easily"]),
 "C": ("C  Lean-back wedge (12°)",
       ["Thin puck tilted 12° back on a wedge",
        "sole; jack in the sole, out the back",
        "Front: circle on a small chin",
        "Footprint 24 x 27 mm",
        "+ easier to read on a desk, stable",
        "- chin under the screen; more complex"]),
 "D": ("D  Puck on a plinth",
       ["Thin puck sitting in an 8 mm base;",
        "jack lies in the base, out the back",
        "Front: circle + visible base",
        "Footprint 28 x 27 mm (most stable)",
        "+ very stable, jack fully enclosed",
        "- +6.7 mm height, base is visible"]),
}
VIEWS = [("front", "Front"), ("side", "Side (right)"), ("side_cut", "Section (centre)"),
         ("rear", "Rear 3/4"), ("front34", "Front 3/4")]
T = 360            # tile size on the sheet
CROP = 22          # crop margin from the 620 px renders
LW = 470           # left text column
HEAD = 150
W = LW + T*len(VIEWS) + 20
H = HEAD + 40 + T*len(OPTS) + 20
sheet = Image.new("RGB", (W, H), (246, 245, 241))
d = ImageDraw.Draw(sheet)
d.text((24, 20), "Round AMOLED clock case: 4 concepts", font=f_title, fill=(25, 25, 25))
d.text((24, 66), "All: flush glass, no lip, 1.25 mm wall (48.8 mm OD). Screen goes in from the front and stops on 4 pegs at the "
       "standoffs; held by friction + silicone dots.", font=f_s, fill=(60, 60, 60))
d.text((24, 90), "Push-out holes behind the pegs for service. USB-A slides in from inside: rear lip stops pull-out, a printed "
       "latch/silicone stops push-in. Screw-free.", font=f_s, fill=(60, 60, 60))
d.text((24, 114), "Grey/white = USB-A jack, blue = PCB, black = glass. Dimensions are first-guess and parametric; "
       "the fit-test coupon comes next.", font=f_s, fill=(60, 60, 60))
for i, (_, lab) in enumerate(VIEWS):
    x = LW + i*T
    w = d.textlength(lab, font=f_col)
    d.text((x + (T - w)/2, HEAD + 6), lab, font=f_col, fill=(70, 70, 70))
for r, (o, (title, lines)) in enumerate(OPTS.items()):
    y = HEAD + 40 + r*T
    if r: d.line([(16, y), (W - 16, y)], fill=(215, 212, 205), width=2)
    d.text((24, y + 30), title, font=f_h, fill=(20, 20, 20))
    for k, ln in enumerate(lines):
        col = (30, 110, 60) if ln.startswith("+") else (160, 50, 40) if ln.startswith("-") else (55, 55, 55)
        d.text((28, y + 84 + k*34), ln, font=f_b, fill=col)
    for i, (v, _) in enumerate(VIEWS):
        im = Image.open(f"{HERE}/r/{o}_{v}.png").convert("RGB")
        im = im.crop((CROP, CROP, im.width - CROP, im.height - CROP)).resize((T, T), Image.LANCZOS)
        sheet.paste(im, (LW + i*T, y))
sheet.save(f"{HERE}/case_concepts_ABCD.png")
print(sheet.size)
