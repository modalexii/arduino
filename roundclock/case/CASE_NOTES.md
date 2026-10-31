# Clock Case: Design Notes

Handoff notes for the 3D-printed case. This folder holds only case files. Don't touch the firmware (`src/`, `include/`, and so on) from case work.

Status (2026-10-30): **Rev 3: power is now a USB-C breakout board** (Kyle swapped from the bare USB-A).
- **Files:** `clock_case.scad` is the source and `clock_case_C2.stl` is the print-ready export. `clock_case_C2.png` shows the renders, and the C2 USB-C section below has the details.
- **USB-A version:** `clock_case_usba.scad` / `clock_case_C2_usba.stl`, which Kyle printed and said "worked well".
- **Not yet measured:** the breakout's dimensions come from a listing that warns of ±1–2 mm error, so ask Kyle to caliper them (list in that section).

Previous status (2026-10-29): **The final design is built.**
- **Files:** `clock_case.scad` is the source and `clock_case_C2.stl` is the print-ready export; `clock_case_C2.png` shows the renders.
- **Fit test:** Kyle skipped the test print because his printer is well calibrated.
- **Not yet measured:** the part dimensions are still the assumed values (see "Measurements to ask Kyle for"). If his calipers disagree, change the parameters at the top of `clock_case.scad` and re-export.
- **Old concepts:** `amoled_case_concepts.scad` is history only. It holds concepts A to D and C2, and was used to make the earlier renders.

## C2 rev 3: USB-C breakout (2026-10-30), current `clock_case.scad`

**The part:** a red 22 × 13 mm USB-C breakout PCB.
- **Pins:** six THT holes along one 22 mm edge (CC2, D+, D−, CC1, GND, VBUS), and 5.1 k CC pull-downs, so a USB-C charger will supply 5 V.
- **Receptacle:** on the opposite edge, sticking out about 1 mm past the PCB (the listing says 14 vs 13).
- **Mounting holes:** two of about Ø3 at the receptacle-end corners, 16.3 mm apart. Total height about 5.
- **Dimensions are guesses:** all of these come from the listing photo (±1–2 mm), and they're parameters at the top of the .scad (`bw bl bt c_w c_h c_l c_over mh_d mh_dx mh_z`).

Kyle asked for it to slide in **from the front, before the screen**, with the print catching the PCB and the receptacle flush at the back. How the design does that:
- **Insertion path:** the board slides along the **puck axis**, not world-horizontal, receptacle side down. It rides just above the two lower stop pads (`y0` = lower pad top + 0.85), so it goes straight in through the open front.
  - The receptacle (|x| ≤ 4.5) hangs between the pads, which sit at |x| ≥ 9.
  - Why not lower: a 22 mm board can't run along the bottom of the 46 mm bore. Its corners hit the curved wall, and the lower pads block it.
- **Board position:**
  - The PCB front edge (header holes) is flush with the cavity's inner back face (`Z_f = ceil_z`), so the 3 mm wire gap behind the module header is kept.
  - The PCB back edge stops on the back wall. The receptacle passes through an opening with 0.2 clearance per side and a 0.3 lead-in, and finishes flush with the back face (`Z_out = Z_b − c_over`). That wall is only `c_over` (1.0) thick around the opening.
- **Rails:** the PCB slot is `bw + 2×0.3` wide and `bt + 2×0.1` thick.
  - Receptacle and parts channel below it: |x| ≤ 10, `c_h + 0.3` tall.
  - Wire channel above it: |x| ≤ 7.2, 1.6 tall, open to the cavity. Wires are soldered on the non-receptacle side before inserting.
- **Catch:** two spring fingers (2 × 1 mm, about 7.8 mm long) sit above the PCB at x = ±mh_dx/2, rooted at the front and free at the back.
  - Each has a 0.6 mm detent bump: a 45° ramp toward the front and a steep face toward the back. The bump drops into the mounting hole and stops the plug pushing the board forward.
  - Unplugging pulls the board against the back wall.
  - Flex room above each finger is 0.8. The finger strain at 0.6 mm deflection is about 1.5 %, which is fine for PLA.
- **Release:**
  - Two Ø1.6 holes run straight up from the hidden underside, through the parts channel, to the mounting holes.
  - Push a paperclip up to lift each bump, then slide the board out the front.
- **Lower push-out holes:** moved 0.8 mm outward to Ø2.0, so they clear the pocket. They still land on the standoffs.
- **Tail shape:** the tail is now the screen's own cylinder (r = R − 0.2) carried straight back along the puck axis.
  - It is clipped to |x| ≤ 14.5 and to `y_top` = PCB top + 0.1 + finger + flex gap + 1.2. Its back face is perpendicular to the axis, so the receptacle points 12° down.
  - In print orientation it's a vertical extrusion, with clean round push-out hole exits on its back face.
- **Checks:**
  - Watertight, one body, 8.3 cm³. No overlap with the module or the board when seated.
  - **Insertion sweep** (board stepped 0–32 mm along the axis): the only contact is the two detent bumps over the last 0.5–2.5 mm, as intended.
  - **Overhangs face-down:** the inner back wall (about 1420 mm², hidden), the stop pads (about 90 mm²), and the pocket's end wall (narrow slots, short bridges).
  - **Thin spots:** only on the finger and bump (0.57) and the mouth lead-in.
- **Receptacle opening (rev 3.1, 2026-10-30):** at Kyle's request, the opening is a rounded rectangle with a 0.8 corner radius (`c_r`). It is centred on the receptacle, with clearance raised to 0.35 per side (`c_fit`) for board tolerance, and a rounded 0.3 lead-in on the outside. The `mouth` view in `render_views.py` looks straight at it.
- **Size:** 48.8 W × 30.1 D × 48.2 H.
- **Renders:** `F_*` is assembled, `I_*` has the board 9 mm out, `K_*` is the finger section, made with `-D cut_x=-8.15` and the `CUT_X` env var for `render_views.py`. The `zoom` and `bottom` views are in `render_views.py`.

**Measurements to ask Kyle for:**
1. PCB width, length and thickness.
2. Receptacle width, height and overhang past the PCB edge.
3. Mounting hole Ø, spacing, and distance from the receptacle-end edge.
4. Anything sticking up on the non-receptacle side apart from the header solder joints.

## Final design: `clock_case.scad` (C2), USB-A version, now `clock_case_usba.scad`

Everything is in one file with no includes.

**Modes:**
- `mode="print"` (the default) gives the case only, front face on the bed. Export with `openscad -o clock_case_C2.stl clock_case.scad`; it takes about 10 s.
- `mode="assembly"` gives the case standing on the table, with the board and jack, for renders.
  - `part`, `cut`, `show_module`, `jack_dz` and `pins_bent` work as they do in the concept file.
  - Render with `export_final.sh F <part> <cut>`, then `render_views.py F <view>`, then `compose_final.py`.

**Changes from the concept C2:**
- **Standoff stops are wall corbels.** Each flat Ø5 pad is joined to the side wall, so no stop is a floating island when printed face-down.
  - The contact face stays flat at `-stack`. An earlier 45° hull version slanted the pad and would have pushed the screen 0.7 mm proud. That bug was caught in the clearance check.
- **Front-rim chamfers:**
  - Outer 0.3, plus an inner 0.2 lead-in that also gives elephant-foot relief.
  - That leaves a 0.75 mm ring on the bed, 142 mm² of contact.
- **Tail top over the jack:** 2.0 mm thick.
- **Lead-in at the jack mouth:** 0.5.
- **Tail flanks:** kept curved, following the screen's own cylinder.
  - A flat-flank version (x = ±13 / ±14.5) was tried and dropped. At ±13 it left a 0.2 mm wall next to the bottom push-out holes, and it showed as a block from the level front.

**Checks (2026-10-29):**
- Watertight, one body, 10.3 cm³ (about 12 g solid).
- No overlap with the module or the jack, apart from the intended crush ribs (3.8 mm³).
- The standoff tips touch the pads with a 0.00 gap.
- **Overhangs (face-down):** only the inner back wall of the cavity (about 1500 mm², a 46 mm ceiling, hidden) and the stop pads (about 90 mm²).
- **Thin spots under 0.6 mm:** only where the tail's back-bottom corners make edges with the table cut, and where the bottom push-out holes exit the sloped tail top at a shallow angle. Both are cosmetic.

**Print settings recommended to Kyle:**
- **Orientation:** front face down; the STL is already oriented.
- **Layers:** PLA, 0.16–0.2 mm layers.
- **Walls:** at least 3 walls, so the 1.25 mm rim prints solid. 15–20 % infill.
- **Supports:** tree supports, **build plate only**. They rise through the open front and touch only the hidden inner back wall and the stop pads.
  - Without supports, a 46 mm bridge will sag a little. That's harmless, because there's a 3 mm wire gap behind the standoffs.
- **Bed adhesion:** elephant-foot compensation of about 0.1–0.15. Add an outer brim of 3–4 mm only if bed adhesion is marginal, because the bed contact is a thin ring.
- **Seam:** put the seam at the back.

**Assembly order:**
1. Jack first (back-loaded, pins bent; see the C2 USB section).
2. Wires onto the header.
3. Optionally, a dot of silicone on 2 of the pads.
4. Press the screen in from the front.

**Service:** remove the screen with a 2 mm rod through the push-out holes. The bottom two holes exit through the tail top.

## Requirements (from Kyle)

- **Board:** Waveshare ESP32-S3-Touch-AMOLED-1.43. The glass is Ø46.0 mm; see `BOARD_NOTES.md`.
- **Bezel:** ultra-compact. The most important goal is the least extra bezel around the screen.
- **Shape:** a round case with a flat bottom, so it stands upright on a table.
- **Screen orientation:** the screen goes in from the **front**. "Up" is with the on-board USB-C on the **left**, seen from the front. The USB-C doesn't need to be reachable.
- **Power:** a **bare** right-angle through-hole USB-A female jack (not on a PCB), captured by the case. It exits the **back, near the bottom**.
  - Kyle solders the wires to the jack and plugs them onto the board's 5V/GND header pins.
  - He'll use **low-profile wire ends**: a bare crimp with heat shrink, or bent pins. Don't plan for full Dupont housings.
- **Servicing:** the case must open for service, but it doesn't need to be easy.
  - Silicone is fine.
  - No screws: he has none for the board standoffs and few heat-set inserts.
- **Printing:** FDM, 0.4 nozzle, **PLA**.
- **Retention:** flush glass with no lip (his choice over a snap-on bezel ring).
- **Workflow:** Kyle has calipers and is happy to print a test-fit coupon first.

## Geometry assumptions (verify; all are parameters in the .scad)

| Item | Value | Source |
|---|---|---|
| Glass OD | 46.0 | Waveshare drawing |
| Glass front → PCB back | 6.2 | drawing |
| Standoffs | M2, H4, so their tips are 10.2 behind the glass front | drawing (M2.00-H4) |
| Standoff centres (front view, USB-C left) | (11.6, ±15.75), (-11.5, ±16.25) | from the drawing, rotated; **measure** |
| PCB / frame OD behind the glass | ~44 | estimated from the photo; **measure** |
| USB-A jack shell | 13.1 W × 5.7 H × 14 L, pin zone 2.5 | typical AF 90° THT part; **measure** |
| Bore | 46.3 (0.15 radial clearance) | PLA fit, to tune with the coupon |
| Wall | 1.25 (3 lines), giving **48.8 OD** | |

**Board rotation:** the board is turned 90° from Waveshare's back-view photo. Seen from behind, the photo's **left** header (the one with 5V, GND, and BAT) ends up along the **bottom**, right next to the USB-A jack. The wire run is short.

## Shared concept features

1. **Front:** the glass sits flush with the case's front rim, and there is a 0.4 mm chamfer on the outer edge.
2. **Back stop:** the module stops on 4 pegs (Ø5.5) that touch the 4 brass standoffs.
3. **Retention:** friction plus a few dots of silicone.
4. **Service:** Ø2.4 push-out holes go through the pegs. Push on the standoffs with a 2 mm hex key to pop the screen out the front.
5. **Wire room:** 3 mm behind the standoff tips (`wire_gap`).
6. **USB-A jack:**
   - It slides into its pocket from inside the case before the screen goes in.
   - A **0.8 mm rear lip** around the mouth (opening 12.6 × 5.1, which clears a plug) stops it pulling out when you unplug.
   - A printed latch or a silicone dab stops it pushing in when you plug in.
   - The jack is flipped so its pins point **up** toward the board header.
   - Its two shell prongs are assumed to be bent flat or clipped.
7. **Print orientation:** print **face-down**, with the front rim on the bed, for the cleanest flush edge. The jack pocket is then vertical and needs no supports.

## The four concepts (`opt` in the .scad)

| Opt | Name | Size (W × D × H, mm) | Footprint | Notes |
|---|---|---|---|---|
| A | Puck + hidden tail | 48.8 × 27 × 48.3 | 22 × 27 | 14.8 mm puck. A keel-shaped tail carries the jack back. The front is a pure circle with a small foot. |
| B | Deep can | 48.8 × 27 × 48.3 | 10 × 27 | Plain cylinder with a 0.5 mm flat. Simplest shape, but the least stable. |
| C | Lean-back wedge, 12° | 48.8 × 27.4 × 51.3 | 24 × 27 | Puck tilted back on a wedge sole that holds the jack. Has a small chin. |
| D | Puck on a plinth | 48.8 × 27.2 × 55.0 | 28 × 27 | 8 mm base holds the jack lying flat. Most stable, but the base is visible. |

**Depth driver:** total depth is about 27 mm in every option. It comes from the module stack (10.2) plus the jack (14 + 2.5 pins) plus the lip. There's no room for the jack beside the board inside a 48.8 mm circle. A shorter jack (10 mm-body versions exist) would save about 4 mm.

## C2: the chosen design (rev 2, 2026-10-29)

Kyle asked to shrink C's chin by lowering the board, moving the USB back, and making the tail deeper. What changed:

- **Board lowered by 3.2 mm:** the front-bottom rim now sits only `chin = 0.5` mm above the table.
  - The table plane slices the tilted puck's lower back flat, and that slice is the foot.
  - The cavity floor is clipped at `cav_clip = 0.4` mm above the rim pivot, so the floor is at least 0.9 mm thick. The module's lowest point is about 0.76 mm above the pivot with the assumed 44 mm PCB. A larger PCB eats into that, so **measure the PCB OD**.
- **Tail stays hidden face-on:** the tail is clipped to the screen's own cylinder (r = R − 0.2) extended back along the screen axis. Looking at the screen face-on (12° down), the case is a pure circle with no chin at all.
  - From dead level you still see about 3 mm of the cylinder's underside. That comes from the tilt itself, not from a chin.
- **USB-A moved 4.1 mm back:**
  - The jack's insulator end sits just behind the tilted back face (`zjfC2`), and its mouth is at `zmC2` = −31.6.
  - A wire channel runs from the pins forward and up into the cavity.
  - The bottom-peg push-out holes exit through the tail top.
- **Size:** 48.8 W × 31.6 D × 48.2 H (was 27.4 D × 51.3 H). Footprint 32 × 29 (was 24 × 27).
- **Checks:** watertight. No overlap with the module or the jack in the model.
- **Gotcha:** in the tilted frame, OpenSCAD 2021 made a non-manifold STL where the front chamfer cone was tangent to the outer wall. The chamfer cone now starts 0.1 mm outside the wall, so the two surfaces cross cleanly.

### C2 USB-A install: back-loading with pin hooks (added after Kyle asked "how do I get it in?")

Rev 2's first version had no way in. The pocket sat behind a wall at the bottom-rear of the cavity, and the jack is too long to tilt in through the front. It was fixed by loading the jack from the back. The sheet is `case_C2_usb_install.png` (from `compose_steps.py`).

1. **Prep:** straighten the jack's pins so they point straight out the back. Fold the 2 shell prongs flat or clip them.
2. **Insert:** with the screen out, push the jack in through the open rear pocket (`jack_cut_back`) until it hits a 1.0 mm stop wall, so its mouth is flush with the back. Plugging in a cable pushes it against that wall.
3. **Lock:** the pins pass through a 10.5 × 3.2 slot into the wire channel. Bend them up with pliers through the open front. The bends hook the stop wall, so unplugging can't pull the jack out.
4. **Wire:** solder 26–28 AWG wires (before step 2 if the joint is slim, otherwise after bending). Push the wire ends onto 5V/GND and press the screen in.

Details:
- **Fit:** 0.15 mm clearance plus crush ribs (r 0.3, so 0.15 mm interference) on both sides and the floor. The pocket depth allows the insulator to stand 0.7 mm proud of the shell (`jins`).
- **Removal:** screen out, straighten the pins, push the jack out the back.
- **Preview:** `export2.sh` and the `jack_dz` / `pins_bent` / `show_module` parameters make the step renders. `render_views.py` views `zoom`, `rearzoom`, and `intozoom` frame the pocket. Section views add a darker cap on the cut plane (needs `shapely` and `rtree`).
- **Model check:** the only overlap with the jack is the intended crush-rib interference (about 3.8 mm³).

**Known concept-level clash:** in B and D, the jack pins overlap the pocket's front wall by about 0.2–0.7 mm³. Fix this in the final design.

## Measurements to ask Kyle for

1. **USB-A jack:**
   - Shell width, height, and length.
   - Where the pins exit, how long they are, and the pin count. (He said "2 through-hole prongs": confirm whether it's a 2-pin power-only jack or 4 pins plus 2 shell legs.)
2. **PCB:** OD, and the tallest part on the back (is anything taller than the 4 mm standoffs?).
3. **Header pins:** do pins stick out of the 2×7 back header, how far, and are they male or female?
4. **Standoffs:** centre-to-centre spacing, both pairs.
5. **Glass:** OD (confirm 46.0) and the glass-plus-panel thickness.

## Files

- **`amoled_case_concepts.scad`:** the parametric concept model, built with OpenSCAD 2021.01.
  - `-D opt="A"`: which concept to build.
  - `part="case"`: export only the case.
  - `display=false`: print-ready orientation (front at z=0, body toward -z; flip it face-down to print).
  - `cut=true`: section view.
- **`export.sh`, `render_views.py`, `compose_sheet.py`:** the render pipeline.
  1. `export.sh OPT PART CUT` writes the STLs to `stl/`.
  2. `render_views.py OPT VIEW [cut]` renders with pyrender/EGL in the cloud container to `r/`.
  3. `compose_sheet.py` builds the comparison sheet.
- **`case_concepts_ABCD.png`:** the comparison sheet sent to Kyle.
- **`case_C2_rev2.png` / `compose_c2.py`:** the C → C2 before/after sheet. `render_views.py` also has a `faceon` view, which looks perpendicular to the 12° screen.

## Decision log

- **2026-10-29:** Kyle's answers: PLA on a 0.4 nozzle, low-profile wire ends, flush glass with no lip, calipers plus a test print. Four concepts rendered.
- **2026-10-29:** Kyle picked C and asked to shrink the chin. C2 built (see above). Then Kyle asked how the USB jack gets in: changed it to back-loading with pin hooks (see the C2 USB section). Waiting for his measurements before the fit coupon and the final STL.
- **2026-10-29:** Kyle said "let's build it" and skipped the test prints. Built the final `clock_case.scad` / `clock_case_C2.stl` with the assumed dimensions; the measurements are still unconfirmed.
- **2026-10-30:** Kyle swapped to a USB-C breakout and asked for front insertion with the print catching the PCB. Built rev 3 (see the section above). The breakout dimensions still need calipers.
