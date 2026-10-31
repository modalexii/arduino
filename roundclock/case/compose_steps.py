"""C2 USB-A install sequence sheet."""
from PIL import Image, ImageDraw, ImageFont
import os, textwrap
HERE = os.path.dirname(os.path.abspath(__file__))
F = "/usr/share/fonts/truetype/dejavu/"
ft = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 32); fh = ImageFont.truetype(F + "DejaVuSans-Bold.ttf", 24)
fb = ImageFont.truetype(F + "DejaVuSans.ttf", 17); fs = ImageFont.truetype(F + "DejaVuSans.ttf", 16)
STEPS = [
 ("S1_rearzoom", "S1_zoom_cut", "1  Prep the jack",
  "Straighten the pins so they point straight out the back. Fold the 2 shell prongs flat or snip them. "
  "Optional: solder thin (26-28 AWG) 5V/GND wires to the pin tips now; the slot passes a slim joint."),
 ("S2_rearzoom", "S2_zoom_cut", "2  Push in from the back",
  "Screen out. Press the jack in through the rear opening until it bottoms on the stop wall, mouth flush. "
  "Crush ribs make it a snug press fit. Plugging a cable in pushes it against the stop."),
 ("S3_intozoom", "S3_zoom_cut", "3  Bend the pins up",
  "Reach in through the open front with needle-nose pliers and bend the pins up. The bends hook the stop "
  "wall, so unplugging can't pull the jack out. Solder the wires now if you didn't in step 1."),
 ("C2_rearzoom", "C2_zoom_cut", "4  Wire up, screen in",
  "Push the wire ends onto 5V and GND (bottom header, left of centre seen from the front). Press the "
  "screen in from the front. Optional: a dab of silicone where the shell meets the opening."),
]
T, PAD = 430, 16
W = PAD + len(STEPS)*(T + PAD); TOP = 120
H = TOP + 60 + 150 + 2*T + 2*PAD + 70
im = Image.new("RGB", (W, H), (246, 245, 241)); d = ImageDraw.Draw(im)
d.text((24, 20), "C2: installing the USB-A jack (no screws, no extra parts)", font=ft, fill=(25, 25, 25))
d.text((24, 64), "The jack loads from the BACK, then its own pins lock it in. Bottom row: centre section, darker tan = cut plastic.",
       font=fs, fill=(60, 60, 60))
d.text((24, 88), "To remove it: take the screen out (push-out holes), straighten the pins, push the jack out the back.",
       font=fs, fill=(60, 60, 60))
for i, (a, b, title, body) in enumerate(STEPS):
    x = PAD + i*(T + PAD); y = TOP + 10
    d.text((x, y), title, font=fh, fill=(20, 20, 20))
    for k, ln in enumerate(textwrap.wrap(body, 44)):
        d.text((x, y + 38 + k*23), ln, font=fb, fill=(55, 55, 55))
    for j, n in enumerate((a, b)):
        t = Image.open(f"{HERE}/r/{n}.png").convert("RGB").resize((T, T), Image.LANCZOS)
        im.paste(t, (x, TOP + 60 + 150 + j*(T + PAD)))
im.save(f"{HERE}/case_C2_usb_install.png"); print(im.size)
