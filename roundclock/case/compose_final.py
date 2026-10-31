"""Final C2 sheet, rev 3 (USB-C breakout)."""
from PIL import Image, ImageDraw, ImageFont, ImageOps
import os, textwrap
HERE = os.path.dirname(os.path.abspath(__file__))
F = "/usr/share/fonts/truetype/dejavu/"
ft = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 32); fc = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 19)
fs = ImageFont.truetype(F + "DejaVuSans.ttf", 17); fh = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 21)
TILES = [("F_faceon", "Face-on (12° down)", 22), ("F_side", "Side", 22), ("F_rear", "Rear 3/4", 22),
         ("I_zoom_cut", "1. Slide in from the front", 0), ("F_zoom_cut", "2. Seated: USB-C flush at back", 0),
         ("K_zoom_cut", "Finger clicks into mount hole", 0),
         ("F_bottom", "Underside: 2 release holes", 0), ("F_side_cut", "Section", 22), ("P_print", "Print orientation", 0)]
STEPS = [
 "1  Solder VBUS + GND wires to the side WITHOUT the receptacle; run them toward the header-hole edge.",
 "2  Screen out. Receptacle side down, receptacle end first: push the board straight in from the front "
 "along the rails at the bottom of the cavity until both fingers click into its mounting holes.",
 "3  Wires onto the 5V / GND header pins, then press the screen in.",
 "Removal: screen out, push a paperclip up through each hole underneath to lift its finger, slide the board out the front.",
]
T, COLS = 400, 3
LW = 520
W = LW + COLS*T + 20; H = 120 + 3*(T + 34) + 10
im = Image.new("RGB", (W, H), (246, 245, 241)); d = ImageDraw.Draw(im)
d.text((20, 18), "Clock case C2, rev 3: USB-C breakout", font=ft, fill=(25, 25, 25))
d.text((20, 62), "48.8 W x 30.1 D x 48.2 H mm  |  8.3 cm³ PLA (~10 g)  |  one piece, no screws  |  glass flush, 1.25 mm wall",
       font=fs, fill=(60, 60, 60))
y = 130
d.text((20, y), "Install", font=fh, fill=(20, 20, 20)); y += 36
for st in STEPS:
    for k, ln in enumerate(textwrap.wrap(st, 46)):
        d.text((24 if k == 0 else 50, y), ln, font=fs, fill=(55, 55, 55)); y += 24
    y += 12
y += 10
d.text((20, y), "Print", font=fh, fill=(20, 20, 20)); y += 36
for ln in ["Front face down (STL is oriented).", "PLA, 0.16-0.2 mm layers, 3+ walls, 15-20% infill.",
           "Tree supports, build plate only (they reach", "  only the hidden inner back wall + pads).",
           "Elephant-foot comp. ~0.1-0.15 mm."]:
    d.text((24, y), ln, font=fs, fill=(55, 55, 55)); y += 24
for i, (n, lab, crop) in enumerate(TILES):
    x = LW + (i % COLS)*T; yy = 110 + (i // COLS)*(T + 34)
    w = d.textlength(lab, font=fc); d.text((x + (T - w)/2, yy), lab, font=fc, fill=(70, 70, 70))
    t = Image.open(f"{HERE}/r/{n}.png").convert("RGB")
    if n == "F_bottom": t = ImageOps.flip(t)
    t = t.crop((crop, crop, t.width - crop, t.height - crop)).resize((T, T), Image.LANCZOS)
    im.paste(t, (x, yy + 30))
im.save(f"{HERE}/final/clock_case_C2.png"); print(im.size)
