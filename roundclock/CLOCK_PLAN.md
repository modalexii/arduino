# Minimal Clock: Project Plan

A plan for a future Claude instance (or Kyle) to execute. Read `BOARD_NOTES.md` first: it has the board, pin map, PlatformIO setup and the Windows build fixes. This file covers only the clock.

Status: **All phases done** (2026-10-29; Phase 6 verified on the board by Kyle). Left: a passive 48 h soak check (§8, last item) and the burn-in review after a week of use (§1). **Phase 5 skipped** by Kyle: set-time mode is entered only by unplugging (no touch or button trigger). Update the phase checklist and progress log in §9 as phases land.

### Start here (handoff for the next instance)

- **Where things are:** the project lives on Kyle's PC (`inky`) at `Desktop\claude\amoled143`. Through the remote shell it is `$HOME/mnt/claude/amoled143`. Edit files there in place, or edit a copy in the cloud container and copy it back. `BOARD_NOTES.md` → "Compile-checking from a cloud agent", step 7, has the tarball method.
- **Who flashes:** Kyle builds and uploads from VS Code + PlatformIO. The remote shell has no `pio` and no USB access.
  - Each phase ends with a hand-off: what to flash, the serial commands to type, the expected output, and exactly what Kyle should report back (paste the serial output, describe or photograph the screen).
  - He replies with results; record them here and in `BOARD_NOTES.md`.
- **Compile-check before handing off:** you can build in the cloud container. The recipe and proxy workarounds are in `BOARD_NOTES.md`, under "Compile-checking from a cloud agent". Expect to redo the setup: the container from the last session is probably gone. Phases 0–2 all built clean that way before Kyle flashed them.
- **Preview the face without the board:** `tools/host_sim/` runs `face.cpp` with real LVGL on a PC, checks consistency and dumps frames to PNG (see `BOARD_NOTES.md`). Run it after any face change; it must print "all consistency checks passed".
- **Remote-shell limits:** it can't delete files (`rm` fails). `.pio/` holds leftover transfer tarballs (`snapshot.tgz`, `phase2.tgz`, `p2b/c/d.tgz`), which are harmless.
- **Phase 6 additions (see the Phase 6 log entries):** `dimmer::` in `dimmer.cpp/.h` (brightness follows the sun: day / evening / night, with a fade; `B n` / `B`; sun maths in `dimmer.h`, used from `main.cpp: sun_phase()`); `face::set_offset()` + `orbit_step()` in `main.cpp` (pixel orbit, off by default; `O dx dy` previews it); `cfg::DEBUG_LOG` gates chatty serial output; `cfg::AP_STAY_UP_UNTIL_SET` keeps the hotspot up while there's no valid time. Tests: `tools/host_sim/dim_test.cpp`, and `sim` now checks the orbit.
- **Phase 4 additions (see the Phase 4 log entry for details):** `W` serial command starts/cancels set-time mode; boot with an invalid RTC goes straight into it. `portal::` in `timeset_portal.cpp`; parsing + TZ choice in `portal_logic.h` (PC-tested by `tools/host_sim/portal_test.cpp`); setup screen in `face::setup_*`; `tools/gen_tz_table.py` builds `tz_table.h` from `tools/tzdata/`.
- **Current code state (end of Phase 3):**
  - `#define CALIBRATE 0` in `src/main.cpp`: the clock face runs. Set it to 1 to get the Phase 1 calibration screen back (`calibrate.cpp`).
  - The face shows **real local time** (`FAKE_TIME_SPEED = 0`). `tk::` in `timekeeping.cpp` does RTC (UTC) → system clock at boot (aligned on an RTC seconds edge), TZ from NVS (`clock`/`tz`, else `DEFAULT_TZ`), and an hourly re-discipline (slew with `adjtime()` if < 2 s off, else step). If the RTC is invalid at boot, the face shows 12:00 and serial nags every minute.
  - Serial commands: `T <unix_utc> [posix_tz]` set time (s or ms), `Z [posix_tz]` set/show TZ, `R` back to real time, `F h m [s]` / `F` / `S n` fake clock (dev override), `B n` brightness, `?` status (time, TZ, RTC, last re-discipline, last set, face).
  - `ring_render.cpp`: band + ticks, drawn by our own float/anti-aliased renderer into a full-screen RGB565 canvas in PSRAM. `ring::BandState {deg, old_deg, erase_deg}` describes the band, including the top-of-the-hour clearing brush.
  - `face.cpp`: canvas + tiny_ttf hour label (ink-centred plus config offsets). `face::show(h24, m, s, ms)` only re-renders what changed. `face::busy()` is true during the 1.2 s clearing sweep.
  - `main.cpp: animate()`: while `face::busy()`, renders a frame every `ANIM_FRAME_MS` and pushes it with `lv_refr_now()`, then prints `hour clear: ...` stats. **Keep this pacing in Phase 3.** Leaving the frames to `lv_timer_handler()` made the sweep jerky.
  - Already written and reusable:
    - `board.h`: all pins and I2C addresses.
    - `display_init(bool with_lvgl)` and `display_set_brightness()`.
    - `ClockCO5300 *gfx`: an `Arduino_CO5300` subclass with `setOffsets()`.
    - `rtc::read/write/present` (raw PCF85063, 24 h, BCD; `write` clears the OS flag).
    - `touch::begin/read/present` (FT3168).
    - `face::begin/show/busy/print_status`, `ring::render/band_change_bbox`.
    - `tools/embed_ttf.py`: always use it to embed a font (it works around an LVGL tiny_ttf bug; see `BOARD_NOTES.md`).
    - `tk::begin/valid/local_now/utc_ms/loop/set_utc_ms/set_tz/tz/print_status`. Phase 4's portal should call `tk::set_tz()` then `tk::set_utc_ms()`.
  - Not built, by choice: long-press and BOOT button triggers (Phase 5 skipped). `input.cpp` only has the raw touch read used by `calibrate.cpp`; the clock never polls touch.
- **Look and feel settled by Kyle (don't change without asking):**
  - Flat band start exactly at 12 (`ARC_ROUND_START = false`), with a round leading end.
  - Bold 12 o'clock tick.
  - Nixie One at 260 px.
  - Numeral nudged `HOUR_X_OFFSET = 1`, `HOUR_Y_OFFSET = -2`.
  - Hour clear: a black brush sweeping clockwise over `HOUR_CLEAR_MS = 1200`, with a smooth sweep.
  - All other `clock_config.h` values are as in §4.
- **Phase 3 plan (§5.6, §6)** (done in code; kept for reference):
  1. Fill in `timekeeping.cpp/.h`: the RTC holds UTC; boot validity check (OS flag or year < 2025 → invalid); `settimeofday()` from the RTC; TZ from NVS (`Preferences`, namespace `clock`), else `cfg::DEFAULT_TZ`; hourly re-discipline from the RTC; serial `T <unix_utc> [posix_tz]` and `?`.
  2. Make `FAKE_TIME_SPEED = 0` mean real time: get local time from `gettimeofday()` + `localtime_r()` and pass `tv_usec/1000` as the millis to `face::show()`. Keep `F`/`S` as dev overrides (any `F`/`S` switches to the fake clock until reset, or until a new command such as `R` returns to real time). Set `FAKE_TIME_SPEED` back to 0 in the config.
  3. Until Phase 4 there's no set-time portal. If the RTC is invalid at boot, show the face at the RTC's time anyway (or 12:00) and print a clear serial message telling Kyle to use `T`. Phase 5 adds the SETUP screen.
  4. Hand-off checks for Kyle: set the time via `T` (give him the exact command with the current epoch, e.g. from https://www.epochconverter.com or a PowerShell one-liner `[DateTimeOffset]::UtcNow.ToUnixTimeSeconds()`); check it survives RESET; check the US fall-back DST change by setting 2026-11-01 05:59:30 UTC (= 01:59:30 EDT) and watching the clock go 1:59 → 1:00 (the band jumps back, so the clearing sweep runs; that's fine); check unplug → RTC invalid message.
- **Open item still to confirm with Kyle** (§1): burn-in settings, after a week of use.
- **Working with Kyle:** keep messages short. Give step-by-step test instructions, and say plainly what you need back. He likes seeing simulator previews of visual changes.

---

## 1. What we're building

A round 466×466 AMOLED wall/desk clock showing:

- **Hour:** one large white numeral, centered, 12-hour format, no leading zero (`1`–`12`; 10–12 are two digits).
- **Minutes:** a medium-thick **nixie-tube orange** (`0xFF8A1E`, the colour already used in `src/main.cpp`) band traced around the screen edge. It starts at 12 o'clock and grows clockwise as if an invisible minute hand were painting it. It is empty at :00, a full circle just before the hour, then resets.
  - **Smooth sweep:** the band grows continuously with seconds, not in 6° steps (Kyle's choice).
- **Indices:** very fine white ticks every 5 minutes, with slightly bolder ticks at 15/30/45.
- **Nothing else:** no seconds, date, colon, logo or decoration in normal operation.
- **Easy to restyle:** every visual parameter lives in one config header (§4).

### Decisions already made (answered by Kyle)

| Question | Answer | Consequence |
|---|---|---|
| Phone used to set time | Both / either (iPhone and Android) | Use a **temporary Wi-Fi hotspot with a captive portal**. iPhone Safari has no Web Bluetooth, so BLE would need an app. |
| Hour format | 12-hour | `1`–`12`; no AM/PM indicator |
| Power | Always on USB | No sleep or battery work. Time is lost when unplugged (no RTC backup cell), so the clock must go straight into set-time mode on boot when the RTC is invalid. |
| Minute band motion | Smooth sweep | Needs the sub-degree workaround in §5.2 |

### Open items to confirm with Kyle (use the defaults if he's unavailable)

1. **12 o'clock tick:** the spec says bolder ticks at 15/30/45. Default here: 12 o'clock is **also** bold, for symmetry (`TICK_MAJOR_AT_12 = true`). One flag flips it.
2. **Hour font:** default is **Nixie One** (`NixieOne-Regular.ttf`, already in the project), in white. It's thin. If it looks too light at large size, try a bolder face; changing it means swapping one TTF.
3. **Burn-in:** the ticks never move, so they're the main AMOLED burn-in risk. Defaults: ticks at ~70% white and night dimming on. Pixel-orbit is available but off. Confirm with Kyle after a week of use.

---

## 2. Setting the time (the minimal, no-extra-hardware approach)

### Chosen method: on-demand setup hotspot + captive portal

1. **Enter set-time mode**, any one of:
   - automatically at boot if the RTC has no valid time (the PCF85063 oscillator-stop flag is set, or the year is < 2025). This is the normal case after unplugging.
   - ~~long-press the screen for ~3 s~~ / ~~press the BOOT button~~: **dropped** (Phase 5 skipped, Kyle's choice). Unplug and replug instead; serial `W` also works for development.
2. The clock starts a Wi-Fi access point named **`Clock`** (open by default; a WPA2 password can be set in the config). The screen shows the minimal setup screen (§5.5).
3. Kyle joins `Clock` on his phone. A DNS server answers every hostname with the clock's IP, and the OS connectivity checks get redirected, so the phone's **"Sign in to network" sheet opens by itself** (iOS CNA and Android both do this).
4. The page served by the clock **auto-sends the phone's time on load**, so there's nothing to tap. It sends `Date.now()` (UTC ms) and `Intl.DateTimeFormat().resolvedOptions().timeZone` (for example `America/New_York`), plus `getTimezoneOffset()` as a fallback. A big **"Set time"** button is there too, for re-sending.
5. The clock writes UTC to the RTC, saves the timezone (as a POSIX TZ string) in NVS, replies "✓ Time set to 3:47 PM", then **shuts Wi-Fi off completely** after ~3 s.
6. **Timeout:** if nothing is received within 5 minutes, Wi-Fi turns off. The clock shows the time if it has one; otherwise it stays on the setup screen and retries the hotspot on the next trigger.

All in, that's: power up (or long-press), join "Clock" on the phone, done. No app, no internet, no permanent Wi-Fi credentials stored.

**DST** is handled on-device. The RTC stores UTC, and the ESP32's `localtime_r()` with a POSIX TZ string (for example `EST5EDT,M3.2.0,M11.1.0`) applies daylight saving automatically, so **no resync is needed when clocks change**.

**Timezone mapping:** the page sends an IANA name, and the firmware maps it to a POSIX string with an embedded table (generate `src/tz_table.h` from the MIT-licensed `nayarsystems/posix_tz_db` `zones.csv`; about 20–30 KB of flash, which is nothing here). If the zone is unknown, use a fixed offset from `getTimezoneOffset()` (no DST) and log a warning. If no timezone has ever been received, use `DEFAULT_TZ` from the config (Kyle is in `America/New_York`).

**Accuracy:** one HTTP round trip on a local AP is a few tens of ms, far below what a minute display needs.

### Why not BLE (documented so nobody re-litigates it)

- **Web Bluetooth** (a web page that writes the time to a GATT characteristic) works in Chrome on Android, Windows and macOS, but **not on iPhone** (Safari/WebKit has no Web Bluetooth). The page would also need hosting over HTTPS.
- **The iOS Current Time Service (CTS)** can in principle give the time to a bonded BLE peripheral, but iOS only exposes it reliably to devices paired through an app or with special advertising. That's fragile and hard to debug.
- **nRF Connect** (a manual GATT write) works on both platforms but isn't "simple".
- Possible stretch goal: add a BLE characteristic as a *second* path for Android/desktop Chrome (§7). It isn't needed.

### Developer fallback (build it anyway; useful for testing)

USB serial command at 115200: `T <unix_utc_seconds> [posix_tz]` sets the time, and `?` prints RTC, system time, TZ and the RTC drift estimate.

---

## 3. Hardware facts this plan relies on (verify in Phase 1)

| Part | Bus/pin | Notes |
|---|---|---|
| CO5300 AMOLED | QSPI 9–14, RST 21, EN 42 | Working, see `BOARD_NOTES.md`. **The column offset of 6 hasn't been visually verified**, and that matters for a perfectly centred ring. |
| PCF85063 RTC | I2C SDA 47 / SCL 48, expected `0x51` | Registers: `0x00` Ctrl1, `0x01` Ctrl2, `0x02` Offset, `0x04` Seconds (bit 7 = OS, oscillator-stopped flag), `0x05` Min, `0x06` Hour, `0x07` Day, `0x08` Weekday, `0x09` Month, `0x0A` Year. BCD. Use 24 h mode (Ctrl1 bit 1 = 0). |
| FT3168 touch | same I2C, expected `0x38` | For long-press only: read reg `0x02` (TD_STATUS); a low nibble > 0 means touched. Poll every ~50 ms. No interrupt pin known, so polling is fine. |
| BOOT button | GPIO0, active low | Use `INPUT_PULLUP` and debounce. |
| Brightness | `Arduino_CO5300::setBrightness(uint8_t)` | Confirmed to exist in the installed GFX library (DCS 0x51). |

---

## 4. Code layout

Replace the current test sketch. Keep it for reference by moving `src/main.cpp` to `extras/lvgl_font_test.cpp`, because PlatformIO compiles everything in `src/`.

```
amoled143/
  CLOCK_PLAN.md            <- this file
  BOARD_NOTES.md
  platformio.ini
  include/
    lv_conf.h              <- add LV_USE_TINY_TTF etc. (§5.1)
    clock_config.h         <- *** every visual/behaviour knob, nothing else ***
  src/
    main.cpp               <- setup(), loop(), mode state machine (CLOCK / SETUP); CALIBRATE switch
    board.h                <- (added in Phase 1) all pins, I2C addresses, panel offsets
    calibrate.cpp/.h       <- (added in Phase 1) bring-up/calibration firmware with serial commands
    display.cpp/.h         <- panel power, Arduino_GFX (ClockCO5300 subclass), LVGL init, flush + even-pixel rounder
    face.cpp/.h            <- builds and updates the clock face from clock_config.h
    ring_render.cpp/.h     <- (added in Phase 2) float/AA software renderer for the band + ticks
    timekeeping.cpp/.h     <- PCF85063 driver, system time, TZ, NVS, RTC re-discipline
    timeset_portal.cpp/.h  <- SoftAP, DNSServer, WebServer, captive-portal endpoints
    portal_page.h          <- the HTML/JS page as a raw string literal (no external assets!)
    tz_table.h             <- IANA -> POSIX map (generated)
    input.cpp/.h           <- touch long-press + BOOT button
    fonts/nixie_one_ttf.h  <- the TTF embedded as a byte array (or use board_build.embed_files)
  tools/
    gen_tz_table.py        <- builds tz_table.h from posix_tz_db zones.csv
    embed_ttf.py           <- (Phase 2) subsets + embeds the hour TTF -> src/fonts/*.h
    host_sim/              <- (Phase 2) runs face.cpp with real LVGL on a PC, dumps PNG frames
  extras/                  <- not compiled
    lvgl_font_test.cpp     <- the original LVGL/Nixie test sketch
    nixie_one_120.c        <- its 120 px bitmap font
```

### `include/clock_config.h` (starting values; Kyle tweaks these)

Use `constexpr` in a namespace (type-safe, readable errors), with comments on every line:

```cpp
#pragma once
#include <stdint.h>
namespace cfg {
// ---------- Colours (0xRRGGBB) ----------
constexpr uint32_t BG_COLOR          = 0x000000;
constexpr uint32_t HOUR_COLOR        = 0xFFFFFF;
constexpr uint32_t ARC_COLOR         = 0xFF8A1E;  // nixie-tube orange
constexpr uint32_t TICK_COLOR        = 0xFFFFFF;
constexpr uint8_t  TICK_MINOR_OPA    = 180;       // 0-255; fine 5-minute ticks
constexpr uint8_t  TICK_MAJOR_OPA    = 230;       // 15/30/45 (and 12) ticks

// ---------- Geometry (px; screen 466x466, centre 233,233) ----------
constexpr int  EDGE_MARGIN         = 4;    // glass edge -> outer edge of the minute band
constexpr int  ARC_WIDTH           = 12;   // "medium-thick"
constexpr bool ARC_ROUND_CAPS      = true; // rounded leading/trailing ends
constexpr int  TICK_MINOR_LEN      = 12;   // measured inward from the band's outer edge
constexpr int  TICK_MINOR_WIDTH    = 2;
constexpr int  TICK_MAJOR_LEN      = 20;
constexpr int  TICK_MAJOR_WIDTH    = 4;
constexpr bool TICK_MAJOR_AT_12    = true;
constexpr int  TICK_OUTER_INSET    = 0;    // >0 pulls ticks inward away from the edge
constexpr bool TICKS_ABOVE_ARC     = true; // draw ticks over the orange band

// ---------- Hour numeral ----------
constexpr int  HOUR_FONT_PX        = 260;  // tiny_ttf renders at any size: just change this
constexpr int  HOUR_Y_OFFSET       = 0;    // + moves down; nudge for optical centring
constexpr int  HOUR_LETTER_SPACE   = 0;    // tighten "10", "11", "12"
constexpr bool HOUR_12H            = true;

// ---------- Motion ----------
constexpr bool     SWEEP_SMOOTH       = true;
constexpr uint32_t SWEEP_UPDATE_MS    = 1000; // ~0.4 px/s at the rim, so 1 s is plenty

// ---------- Brightness / AMOLED care ----------
constexpr uint8_t BRIGHTNESS_DAY      = 200;  // 0-255
constexpr uint8_t BRIGHTNESS_NIGHT    = 40;
constexpr int     NIGHT_START_HOUR    = 22;   // local, 24 h
constexpr int     NIGHT_END_HOUR      = 7;
constexpr int     ORBIT_PX            = 0;    // 0 = off; else shift whole face up to N px
constexpr int     ORBIT_PERIOD_MIN    = 10;

// ---------- Time setting ----------
constexpr const char *AP_SSID         = "Clock";
constexpr const char *AP_PASSWORD     = "";   // "" = open; >=8 chars = WPA2
constexpr uint32_t AP_TIMEOUT_S       = 300;
constexpr uint32_t LONGPRESS_MS       = 3000;
constexpr bool     BOOT_BTN_TRIGGER   = true;
constexpr const char *DEFAULT_TZ      = "EST5EDT,M3.2.0,M11.1.0"; // America/New_York

// ---------- Development ----------
constexpr int FAKE_TIME_SPEED         = 0;    // >0 = fake clock at N x speed to preview the face
}
```

Rule: **`face.cpp` derives every pixel from these constants.** No magic numbers in drawing code, so Kyle can restyle by editing one file and re-flashing.

---

## 5. Implementation details

### 5.1 LVGL config additions (`include/lv_conf.h`)

```c
#define LV_USE_TINY_TTF 1          // render the hour numeral at any size from the TTF
#define LV_TINY_TTF_FILE_SUPPORT 0 // fonts come from memory, not the filesystem
#define LV_USE_FLOAT 1             // float coordinates/angles where LVGL supports them
```

- Load the font with `lv_tiny_ttf_create_data(nixie_ttf, nixie_ttf_len, cfg::HOUR_FONT_PX)`.
- Glyphs at ~260 px are about 40 KB each in A8, so make sure LVGL's glyph cache lands in PSRAM. `LV_STDLIB_CLIB` malloc already allows this, and Arduino-ESP32 with `BOARD_HAS_PSRAM` sends large mallocs to PSRAM. Verify by printing `heap_caps_get_free_size(MALLOC_CAP_SPIRAM)` before and after the first render.
- **Fallback** if tiny_ttf misbehaves: pre-generate a digits-only bitmap font with `lv_font_conv`, as was done for `nixie_one_120.c`: `--font NixieOne-Regular.ttf -r 0x30-0x39 --size 260 --bpp 4 --format lvgl --lv-font-name nixie_one_260`. Put the exact command in a comment so size changes are reproducible.

### 5.2 Minute band, with a smooth sweep despite LVGL's integer-degree arcs

**Known limitation, verified in source:** `lvgl/src/draw/sw/lv_draw_sw_arc.c` (v9.2) casts `start_angle`/`end_angle` to `int32_t`. Arcs therefore only grow in **1° steps = 10 s ≈ 4 px jumps** at the rim, and `LV_USE_FLOAT` doesn't fix that.

**Approach A (do this first): "brush tip"**

- An `lv_arc` (background arc hidden, indicator = orange, width `ARC_WIDTH`, `rotation = 270` so 0° is 12 o'clock, square caps) draws from 0° to `floor(angle)`.
- A separate filled circle object, diameter `ARC_WIDTH`, in `ARC_COLOR` with `radius = LV_RADIUS_CIRCLE`, is the **brush tip**. Position it at the exact float angle on the band's centre-line radius, rounded to the nearest pixel. It hides the 0–1° gap at the arc's end and moves about 1 px every 2.5 s, which reads as smooth.
- A second identical circle at 0° gives a rounded tail when `ARC_ROUND_CAPS` is set.
- `angle = (minute*60 + second + ms/1000.0) / 3600.0 * 360.0`
- When `angle < 0.5°` (the first ~5 s of each hour), hide the arc and the tip so no stray dot appears at :00.
- With `SWEEP_SMOOTH = false`, use `angle = minute * 6` and update only on the minute change.

**Approach B (only if A looks lumpy): custom anti-aliased ring**

- Use a full-screen transparent object with an `LV_EVENT_DRAW_MAIN` handler.
- For the invalidated area only, compute coverage per pixel: radial AA from distance to centre, angular AA from `atan2` against the float end angle, plus a round cap.
- Invalidate only the bounding box of the tip each update. That's cheap: a ~30×30 px area per second.

### 5.3 Ticks

- Draw all 12 ticks in one `LV_EVENT_DRAW_MAIN` callback on a transparent full-screen object, using `lv_draw_line` with anti-aliasing and float endpoints.
- Endpoints run from `r_outer = 233 - EDGE_MARGIN - TICK_OUTER_INSET` inward by the tick length, along angle `i*30°` measured clockwise from 12.
- Major ticks are `i % 3 == 0`, including i = 0 when `TICK_MAJOR_AT_12` is set.
- Object order sets the layering: create the tick layer after the arc and brush-tip objects when `TICKS_ABOVE_ARC`, otherwise before them.
- The tick layer never needs invalidating after the first draw. LVGL redraws it only where the arc invalidates.

### 5.4 Hour numeral

- Use an `lv_label` with the tiny_ttf font, `HOUR_COLOR` and `lv_obj_center()` plus `HOUR_Y_OFFSET`.
- Font metrics include ascender and descender space, so digits can look low. Compute the glyph's bounding box (`lv_font_get_glyph_dsc`) and centre on the *ink*, not the line box.
- Update the text only when the hour changes. For 12-hour time, `h = tm_hour % 12; if (h == 0) h = 12;`.
- Check that `"12"` fits inside the ring's inner edge (inner radius ≈ 233 − 4 − 20 = 209 px). At 260 px Nixie One it should fit easily; assert it in a debug print.

### 5.5 Setup-mode screen (the only time anything else is shown)

- Ticks stay visible.
- The orange band slowly "breathes": a full ring whose opacity pulses at ~0.5 Hz.
- Centre: small text in Montserrat 28, grey: `Join Wi-Fi` / `Clock`.
- On success: show `✓` in orange for ~2 s, then go back to the face. (Montserrat has no ✓, so draw it as two lines or use LVGL's `LV_SYMBOL_OK`.)

### 5.6 Timekeeping

- **Source of truth:** the PCF85063 holds **UTC**.
- **On boot:** read the RTC. If the OS flag is set or the year is < 2025, the time is invalid and the clock enters SETUP. Otherwise call `settimeofday()` from the RTC value, then `setenv("TZ", posix_from_nvs_or_default); tzset();`.
- **Display:** read time from `gettimeofday()` + `localtime_r()`. This gives the millisecond resolution the smooth sweep needs.
- **Re-discipline:** once an hour, at a seconds rollover, re-read the RTC and `settimeofday()` so the ESP32's own clock can't drift away from it.
- **On time set:** write the RTC (also clears the OS flag), `settimeofday()`, save TZ to NVS (`Preferences`, namespace `clock`).
- **Stretch (§7):** auto-calibrate RTC drift. On each set, compare the RTC with the phone time, compute ppm since the last set, and write the PCF85063 offset register (`0x02`, mode 0 ≈ 4.34 ppm per LSB). Store the last-set epoch in NVS.

### 5.7 Set-time portal details

- `WiFi.mode(WIFI_AP); WiFi.softAP(AP_SSID, AP_PASSWORD_or_null);`. The default IP is 192.168.4.1.
- `DNSServer` on port 53 answers `*` with the AP IP.
- `WebServer` routes:
  - `/` serves `portal_page.h`, a tiny self-contained HTML/JS page. **No external assets: the phone has no internet on this AP.**
  - `POST /set` takes the JSON `{"utc_ms":..., "tz":"America/New_York", "offset_min":240}`, validates it (year 2025–2099), applies it, and replies with the local time string.
  - Captive-portal probes redirect to `http://192.168.4.1/`. Cover iOS/macOS `/hotspot-detect.html`, Android `/generate_204` and `/gen_204`, and Windows `/connecttest.txt` and `/ncsi.txt`. Also add an `onNotFound` catch-all that redirects.
- The page JS auto-POSTs on `DOMContentLoaded`, shows the result in large text, and has a fallback button.
- Run `dnsServer.processNextRequest()` and `server.handleClient()` from `loop()` while in SETUP mode, alongside `lv_timer_handler()`.
- To exit: `server.stop(); dnsServer.stop(); WiFi.softAPdisconnect(true); WiFi.mode(WIFI_OFF);`. **Wi-Fi is off in normal operation.**
- If the iOS sheet doesn't pop up, Kyle can open `http://192.168.4.1` (or any `http://` address) in Safari, and the DNS catch-all lands there too.

### 5.8 Main loop and state machine

```
BOOT -> (RTC valid?) -> CLOCK : SETUP
CLOCK --(long-press 3 s | BOOT btn)--> SETUP
SETUP --(time received)--> show ✓ 2 s -> CLOCK
SETUP --(timeout, time valid)--> CLOCK
SETUP --(timeout, time invalid)--> SETUP_IDLE (Wi-Fi off, setup screen, wait for trigger)
```

`loop()`:

- `lv_timer_handler()`
- input polling every 50 ms
- face update every `SWEEP_UPDATE_MS`
- brightness check every minute
- portal servicing when in SETUP
- `delay(5)`

---

## 6. Phases (do them in order; each ends with something Kyle can flash and see)

**Build/flash note:** Kyle builds and flashes from VS Code + PlatformIO on Windows (`inky`). An agent working through the remote shell can edit files but probably can't reach the USB port. Check whether `pio` is available in the shell. If not, hand each phase to Kyle with "build & upload" instructions and the expected serial output and visual result.

### Phase 0: Housekeeping
- Move `src/main.cpp` to `extras/lvgl_font_test.cpp`. Keep `nixie_one_120.c` in `extras/` too, or delete it if unused.
- Create `include/clock_config.h` (§4) and the empty module files.
- Lift the display/LVGL glue into `display.cpp`, unchanged (flush, even-pixel rounder, PSRAM buffers).

### Phase 1: Hardware bring-up and calibration
- **I2C scan** on SDA 47/SCL 48. Confirm `0x51` (RTC) and `0x38` (touch) and record the results in `BOARD_NOTES.md`.
- **Calibration screen** (a temporary `#define CALIBRATE`):
  - 1 px white circles at r = 232, 230 and 226, plus a centre crosshair.
  - Confirm the ring is concentric with the glass and nothing clips. If it's shifted horizontally, adjust the CO5300 column offset (currently 6; try 0–8).
  - Choose `EDGE_MARGIN` from what's visible. **Record the verified offset and visible radius in `BOARD_NOTES.md`.**
- **RTC smoke test:** write a known time, read it back over serial, power-cycle and check the OS flag behaviour.
- **Touch smoke test:** print the touch count while pressing.
- **Brightness test:** step `setBrightness` 255, 128 and 32.

### Phase 2: The face (fake time)
- Implement `face.cpp`: ticks, arc with brush tip, hour numeral, all driven by `clock_config.h`.
- Set `FAKE_TIME_SPEED = 60` (one hour per minute) to review the whole sweep, the hour change and the 12→1 rollover quickly. Also add a serial command `F h m s` to freeze on a given time, for tuning.
- Check: smooth sweep with no visible stepping, no dot at :00, ticks crisp, "12" fits, numeral optically centred.
- **Show Kyle photos/video here and tune the config before going on.**

### Phase 3: Timekeeping
- `timekeeping.cpp`: PCF85063 raw I2C driver (~80 lines; no library needed. SensorLib's `SensorPCF85063` is an acceptable alternative), UTC in RTC, TZ in NVS, hourly re-discipline, serial `T` and `?` commands.
- Check: set via serial; the clock survives a reset (not an unplug); DST is correct when you set a time just before 2 AM on 2026-11-01 (US fall back).

### Phase 4: Set-time portal
- `timeset_portal.cpp`, `portal_page.h`, `tools/gen_tz_table.py` and `tz_table.h`.
- Check on **both an iPhone and an Android phone**: the sheet auto-opens, the time is set without tapping, Wi-Fi shuts off (the `Clock` SSID disappears), and the timezone is stored and survives a reset.
- Check the 5-minute timeout and the invalid-JSON rejection.

### Phase 5: Triggers and state machine
- `input.cpp`: long-press (3 s continuous touch, debounced) and the BOOT button. Make sure a long-press doesn't re-trigger while still held after exiting SETUP.
- The boot-with-invalid-RTC path goes straight to SETUP.

### Phase 6: Polish
- Night dimming (`BRIGHTNESS_DAY/NIGHT`, local hours) with a gentle fade over ~2 s.
- Optional orbit (`ORBIT_PX`). Off by default; if enabled, check the ring still clears the glass edge.
- Remove the debug prints, or put them behind `#define CLK_DEBUG`. For release, also add `-DCORE_DEBUG_LEVEL=0` to hide the core's log line for the FT3168's periodic I2C NAK (see `BOARD_NOTES.md`).
- Update `BOARD_NOTES.md` with what was learned, and tick the checklist below.

## 7. Stretch ideas (only if Kyle asks)
- RTC drift auto-calibration (§5.6).
- A second set-time path over BLE for Android/desktop Chrome (Web Bluetooth page + one GATT characteristic).
- Show a tiny orange dot in the centre for a few seconds after the hour changes (would break "no other info": ask first).
- Ambient-based dimming is impossible (no light sensor). Schedule-based dimming is what we have.

## 8. Acceptance criteria
- [x] Normal operation shows only: the hour numeral, the orange minute band and 12 ticks, on black.
- [x] Band starts at 12 and grows clockwise smoothly (no visible 4 px jumps), empty at :00, near-full at :59.
- [x] Ticks: 5-minute fine, 15/30/45 (and 12 if enabled) bolder; crisp, anti-aliased, concentric with the glass.
- [x] Hour 1–12, no leading zero, centred on the ink.
- [x] Changing any value in `clock_config.h` and re-flashing changes the look, with no other code edits.
- [x] From unplugged: plug in, join "Clock" on iPhone **or** Android, and the time is correct within 1 s without typing anything.
- [x] Wi-Fi is off whenever not in set-time mode.
- [x] ~~Long-press 3 s or the BOOT button opens set-time mode at any time.~~ Dropped (Phase 5 skipped): unplugging opens set-time mode.
- [x] Brightness follows the sun: 100 from 1 h after sunset, 40 from 2 h after, 200 again at sunrise, each with a smooth ~2 s fade.
- [x] DST transitions happen automatically.
- [ ] Runs 48 h with no glitches, freezes or memory growth (print free heap/PSRAM hourly in debug builds).

## 9. Progress log

Phase checklist:
- [x] Phase 0: Housekeeping (2026-10-28; flashed and verified)
- [x] Phase 1: Hardware bring-up and calibration (2026-10-28; results in `BOARD_NOTES.md`)
- [x] Phase 2: The face (fake time) (2026-10-28; four rounds of tweaks, approved on the board)
- [x] Phase 3: Timekeeping (2026-10-28; verified on the board)
- [x] Phase 4: Set-time portal (2026-10-28; Android + iPhone verified)
- [x] ~~Phase 5: Triggers and state machine~~ (skipped 2026-10-29: Kyle only wants set-time mode via unplugging; the boot-with-invalid-RTC path was already done in Phase 4)
- [x] Phase 6: Polish (2026-10-29; sun-based dimming and orbit verified on the board; orbit left off)

Log:
- 2026-10-28: Plan written. Requirements and decisions captured above. No code yet.
- 2026-10-28: **Phase 0 done.** Old test sketch moved to `extras/lvgl_font_test.cpp` (with `extras/nixie_one_120.c`). Added `include/clock_config.h` (§4 values verbatim), `src/display.cpp/.h` (flush, even-pixel rounder and PSRAM buffers lifted unchanged; pins and the column offset are now named constants in `display.h`; they moved to `src/board.h` in Phase 1), stub headers/sources for `face`, `timekeeping`, `timeset_portal`, `input`, `portal_page.h`, `tz_table.h`, `fonts/nixie_one_ttf.h`, and a stub `tools/gen_tz_table.py`. New `src/main.cpp` boots the display and shows a grey "Clock: phase 0" label. Compile-checked with pioarduino (Arduino core 3.3.12, GFX 1.6.9, LVGL 9.2.2): builds clean, RAM 7.7%, flash 9.8%. Flashed and verified by Kyle: serial shows the banner, 16 MB flash, 8192 KB PSRAM and "Display up."; the screen shows "Clock: phase 0". `pio` isn't on the remote shell's PATH, so Kyle builds and uploads from VS Code.
- 2026-10-28: **Phase 1 firmware written** (compiles clean; not yet run). `#define CALIBRATE 1` in `main.cpp` runs `src/calibrate.cpp`: boot-time I2C scan, RTC read and touch check, then a calibration screen (red/green/white 1 px rings at 0/2/6 px in from the edge, measured from the true centre 233.0 between pixels 232 and 233, plus a 2 px crosshair). Serial commands change the column/row offset live (`c`/`r`, via a `ClockCO5300` subclass in `display.h`), toggle a solid-disk view (`v`), set or test brightness (`b`/`bt`), and read or write the RTC (`t`/`w`). Touch DOWN/UP events print automatically. New `src/board.h` holds all pins and I2C addresses. The raw PCF85063 driver (`timekeeping.cpp`) and FT3168 read (`input.cpp`) are written so Phases 3 and 5 can reuse them. Added `monitor_echo = yes` to `platformio.ini`.
- 2026-10-28: **Phase 1 results from Kyle:**
  - I2C devices at 0x38, 0x51, 0x6B, plus an unexplained ACK at 0x7E.
  - Column offset 6 and row offset 0 look centred.
  - The outermost pixel ring is hidden; the ring 2 px in is visible, so `EDGE_MARGIN = 4` stands.
  - Touch works, with coordinates matching the display.
  - All three brightness levels are distinct.
  - The RTC survives RESET. After an unplug it reads 2000-01-01 with OS=1.
  - The touch chip NAKed polls every few seconds when idle. Fixed with keep-active writes (0x86=0, 0xA5=0); waiting on Kyle to confirm. Details are in `BOARD_NOTES.md`.
- 2026-10-28: **Phase 1 done.** The keep-active writes didn't stop the touch NAKs: exactly one failed read about every 13.7 s (4 of 1200 polls a minute), which looks like an internal cycle of the chip. Accepted as benign: failed reads count as "no change", and the core log line gets silenced in Phase 6. `CALIBRATE` stays available in `main.cpp` for re-checking the panel.
- 2026-10-28: **Phase 2 code written** (compiles clean: RAM 7.8%, flash 10.0%; not yet run on the board). Changes from §5, and why:
  - **Band and ticks use Approach B, not A** (§5.2). `src/ring_render.cpp` draws the band and all 12 ticks in software into a full-screen RGB565 `lv_canvas` (434 KB, PSRAM), anti-aliased from signed distances around the true centre 233.0. The band's end follows the exact float angle, so the sweep is continuous with either cap style and needs no brush-tip objects. Each update re-renders only the bounding box of the change (~19×18 px per second at 1x, under 1.5 ms on a PC) and invalidates just that area. A full redraw (hour change, show/hide) is ~5 ms on a PC.
  - `LV_USE_FLOAT` was not enabled; it isn't needed with the custom renderer.
  - **Hour numeral:** `lv_label` with `lv_tiny_ttf_create_data()` at `HOUR_FONT_PX`, positioned so the centre of the digits' ink box lands on the panel centre (`face.cpp: measure_ink`). At 260 px, "12" is 222×185 px; the farthest box corner is 145 px from centre vs. the ring's inner edge at 209 px.
  - **tiny_ttf bug found (LVGL 9.2.2):** `stbtt_KernTableCheck()` in `stb_truetype_htcw.h` forgets to add the GPOS offset when reading the lookup list, so with Nixie One it read a lookup count of 47338 and walked ~160 KB past the font data (ASan caught it in the PC sim; on the ESP32 it could crash or randomly enable kerning). Fix: `tools/embed_ttf.py` subsets the font to digits + space and drops GPOS/GSUB/GDEF/kern (needs `pip install fonttools`). The embedded font is now 2.7 KB. Always regenerate the header with that tool when swapping the hour font.
  - New config knobs: `ARC_HIDE_BELOW_DEG` (0.5), `FAKE_START_HOUR/MIN` (11:50, so the 11→12 and 12→1 changes come up within ~70 s at 60x). `FAKE_TIME_SPEED` is 60 for review; until Phase 3, 0 behaves as 1x.
  - Face updates every `SWEEP_UPDATE_MS` (1 s) at 1x, and every 33 ms when fast-forwarding.
  - Serial: `F h m [s]` freeze, `F` resume, `S n` speed, `B n` brightness, `?` status (numeral fit, render times, free heap/PSRAM).
  - Verified in `tools/host_sim` (real LVGL 9.2.2 on a PC, ASan clean): sample frames look right; ink centred within 0.5 px; a 200-step incremental sweep is byte-identical to a fresh full frame.
- 2026-10-28: **Phase 2 on the board.** Kyle: "Looks great." His `?` at 60x: band 270°, "2" ink 124×185 at (157,86), fits; free heap 314 KB internal, 7437 KB PSRAM. **On-device render times:** partial ring updates ≤ 2 ms (~3.5 µs/pixel, about 30× slower than the PC), a full ring redraw 156 ms. Avoid full redraws in normal running.
- 2026-10-28: **Phase 2 round 2 (Kyle's requests), compile/sim-checked, not yet flashed:**
  - **Top-of-the-hour clear:** Kyle disliked the old behaviour. At the wrap, the band was dropped with one 156 ms full redraw, which LVGL flushed in strips and so looked like a top-to-bottom wipe. Now a black "brush" sweeps clockwise from 12 over `HOUR_CLEAR_MS` (1000 ms, linear) and paints the old band away. The brush has the band's cap shape, so with round caps the cut is concave, like real paint, and it's 1 px wider each side so no anti-aliased orange fringe is left. The new hour's band is drawn on top and keeps growing underneath. `ring::BandState {deg, old_deg, erase_deg}` describes this. `face::busy()` is true during the sweep, and `main.cpp` then updates every 33 ms. Each frame re-renders only the brush's swept segment, and no full redraw happens at the wrap any more. Also triggered by any jump back in time (`F` to an earlier minute). A second wrap mid-sweep (only possible at very high `S`) restarts it with one full redraw.
  - `band_change_bbox` now treats a hidden band as length 0, so the band appearing at :00:05 also re-renders only its small area, not the whole ring.
  - **Numeral nudge:** new `HOUR_X_OFFSET = 1`, `HOUR_Y_OFFSET = -1` (1 px right, 1 px up, as Kyle asked).
  - `tools/host_sim` now drives `millis()` from a simulated clock and checks every frame of the clearing sweep (1x and 60x) against a fresh full render of the expected state: 0 mismatches. It also writes `clear1_*.raw` / `clear60_*.raw` frames every 200 ms from the wrap.
- 2026-10-28: **Round 2 confirmed on the board.** Kyle: sweep speed (1000 ms) is good, keep the bold 12 tick, Nixie One is heavy enough.
- 2026-10-28: **Phase 2 round 3, compile/sim-checked, not yet flashed:**
  - **Flat start at 12:** Kyle didn't like the round tail sticking out behind the 12 tick. New `ARC_ROUND_START = false`: the band starts flat, exactly on the 12 o'clock line (hidden under the right half of the 4 px tick). The leading end stays round (`ARC_ROUND_CAPS`). While the band is under 90°, its leading cap is also clipped at the 12 line, so in the first ~15 s of the hour it can't poke out behind 12 either. The clearing brush keeps a round tail, so touching down at 12 also removes the old band's leading cap where it overhangs 12 at :59.
  - **Numeral:** `HOUR_Y_OFFSET = -2` (one more px up; x unchanged at +1).
  - Sim: all consistency checks still pass; crops of the 12 o'clock area at 0.6°, 1.5°, 12° and 359.8° show nothing behind the tick.
- 2026-10-28: **Round 3 confirmed** by Kyle ("Ok good"). He then reported the hour-clear sweep looks chunky/jerky.
- 2026-10-28: **Phase 2 round 4 (smoother sweep), compile/sim-checked, not yet flashed:**
  - Cause: at 1000 ms per turn and ~30 fps, each frame jumped ~12° (~47 px). On top of that, `face::show()` ran on its own 33 ms interval while LVGL refreshed on its own 33 ms timer. The two drifted against each other, so some frames carried one step and some two.
  - Fix: while `face::busy()`, `main.cpp: animate()` renders a frame every `ANIM_FRAME_MS` (16 ms, ~60 fps) and pushes it straight to the panel with `lv_refr_now()`, so each frame shows exactly the state computed for it. `lv_timer_handler()` isn't called during the sweep, and the loop sleeps 1 ms instead of 5.
  - `HOUR_CLEAR_MS` 1000 → 1200 (Kyle allowed slowing it a little). That's ~4.8° (~19 px) per frame. The worst-case re-render area per frame is 1120 px, roughly 4–6 ms at the board's measured ~3.5–5 µs/px, which fits in 16 ms.
  - Serial now prints a line at the end of each sweep: `hour clear: N frames in M ms (F fps); first frame X ms, then max Y ms`. The first frame includes drawing the new hour's numeral. If fps is well under 60, the per-pixel renderer (`atan2f`, `sqrtf` per pixel) is the next thing to optimise.
- 2026-10-28: **Round 4 confirmed; Phase 2 done.** Kyle: "Looks pretty good, no changes." Board stats: `hour clear: 48 frames in 1217 ms (39 fps); first frame 81 ms, then max 21 ms`. Frames take up to ~21 ms (render + flush), so it runs at ~39 fps rather than 60; Kyle is happy with that. If it ever needs to be smoother, optimise `ring::render`'s per-pixel maths (replace `atan2f` with cross-product wedge tests and a cheap angle approximation for the tick index). The 81 ms first frame is drawing the new hour numeral at 260 px.
- 2026-10-28: **Phase 3 code written** (compiles clean: RAM 7.9%, flash 10.6%; no warnings in project code; `host_sim` all checks pass; not yet run on the board).
  - `timekeeping.cpp` gains `tk::` (API in `timekeeping.h`). Boot: RTC invalid if it doesn't answer, the OS flag is set or the year < 2025. Otherwise it waits (≤ 1.5 s) for the RTC's seconds to tick and sets the system clock on that edge.
  - Hourly re-discipline is non-blocking: `tk::loop()` (called from `main.cpp` only when not animating) polls the RTC every ~5 ms until its seconds change, then compares with the system clock. Under 2 s off: `adjtime()` slews it (the IDF slews at ~1/64, so 50 ms takes ~3 s); more: `settimeofday()`. A poll gap > 25 ms (loop was busy) restarts the edge wait. The first check runs 15 s after a `T`, so `?` soon shows the measured system-minus-RTC offset.
  - Setting the RTC: second S is written at system time S + 0.5 s, because (per the PCF85063 datasheet, as I read it) the first increment comes ~0.5 s after the STOP bit is released. **Verify on the board:** the first re-discipline after `T` should report an offset within ~±20 ms. If it says ~±500 ms, change `RTC_WRITE_PHASE_MS` to 0.
  - `T` also prints how far off the clock was and the ppm since the last set (NVS `clock`/`set_at`). That gives a rough RTC drift figure for free (it includes the typing delay).
  - `main.cpp`: sources REAL / FAKE. In REAL mode the face updates on each wall-clock `SWEEP_UPDATE_MS` boundary, so the hour changes right at :00.000. `F`/`S` switch to FAKE (starting from the current time), `R` goes back. Serial line buffer is now 96 chars (long TZ strings).
  - `face.cpp`: the band getting shorter by less than the new `cfg::CLEAR_MIN_BACKSTEP_DEG` (1° = 10 s) now just redraws it shorter, instead of starting the hour-clear sweep. Before, any step back over 10 ms would have swept (a re-discipline step or a re-set). `host_sim` checks this.
  - Checked on the PC: the date maths matches `gmtime` from 2025 to 2099, and the TZ rule gives 2026-11-01 01:59:59 EDT → 01:00:00 EST at unix 1793512800.
- 2026-10-28: **Phase 3 done.** Kyle: set via `T`, survives RESET, US fall-back (1:59:59 EDT → 1:00:00 EST, with the clearing sweep) and the unplug → "TIME NOT SET" / 12:00 path all work as expected. First re-discipline 15 s after `T`: system clock minus RTC = **+3 ms** (edge within 6 ms), which confirms `RTC_WRITE_PHASE_MS = 500` (the PCF85063 ticks ~0.5 s after STOP is released). Free memory: 314 KB internal, 7609 KB PSRAM. Fixed afterwards (trivial, not re-flashed): `?` printed the ms after the zone ("EDT.258"); now "20:15:21.258 EDT".
- 2026-10-28: **Phase 4 code written** (compiles clean: RAM 15.4%, flash 22.6%, the Wi-Fi stack is most of the jump; no warnings in project code; `host_sim` all checks pass; `portal_test` passes under ASan/UBSan; not yet run on the board).
  - `timeset_portal.cpp` (`portal::start/stop/active/loop/print_status`): open AP `Clock` at 192.168.4.1, `DNSServer` answering `*`, `WebServer` with `/` (page), `POST /set`, and redirects (302 to `http://192.168.4.1/`) for `/hotspot-detect.html`, `/library/test/success.html`, `/generate_204`, `/gen_204`, `/connecttest.txt`, `/ncsi.txt`, `/redirect`, `/canonical.html`, `/success.txt` and `onNotFound`. Every request is logged to serial with its Host header (useful if a phone's sheet doesn't pop). `WiFi.persistent(false)`. After a set, Wi-Fi stays up 3 s (`LINGER_MS`) so the reply arrives, then `WIFI_OFF`. No set within `AP_TIMEOUT_S` (300 s): Wi-Fi off, `TIMEOUT` event.
  - `/set` order: parse → range check (2025–2099) → `tk::set_tz()` → `tk::set_utc_ms()` → reply `{"ok":true,"local":"8:21 PM","tz":"..."}`. Bad JSON / missing `utc_ms` → 400 `{"ok":false,"error":...}`. Zone: `tz_table.h` lookup, else fixed offset from `offset_min` (e.g. `<-04>4`, no DST, serial WARNING), else TZ unchanged.
  - `tz_table.h`: 461 zones from posix_tz_db + 126 old names from the IANA `backward` file (e.g. `Asia/Calcutta`, `US/Eastern`, `Europe/Kiev`, which some phones still report), sorted for binary search, ~23 KB source. Sources live in `tools/tzdata/`; rerun `python3 tools/gen_tz_table.py` (`--download` to refresh).
  - `portal_page.h`: black page, orange ring, auto-POSTs on load via XHR (`utc_ms`, IANA `tz`, `offset_min`), big result text, "Set time" retry button. Checked in headless Chromium with iPhone emulation against a mock server (success and error paths).
  - Setup screen (`face::setup_enter/message/loop/done/exit/in_setup`): numeral hidden, ticks stay, full ring breathing at 0.5 Hz (opacity 40→255), grey Montserrat 28 `Join Wi-Fi / Clock`; orange `LV_SYMBOL_OK` (Montserrat 48, newly enabled in `lv_conf.h`) for 2 s after a set; `Time not set` with a dim still ring when the hotspot timed out without a valid time. Breathing blends two pre-rendered rings at only the ~18k differing pixels (PSRAM list) and invalidates 14 strips around the ring, so frames are cheap. New `clock_config.h` knobs: `SETUP_TEXT_COLOR`, `SETUP_BREATHE_MS`, `SETUP_BREATHE_MIN`, `SETUP_IDLE_OPA`, `SETUP_FRAME_MS`, `SETUP_DONE_MS`. Sim preview: `Claude outputs/phase4_setup_screen_sim.png`.
  - `main.cpp`: modes CLOCK / SETUP / DONE / SETUP_IDLE (§5.8). Boot with invalid RTC → SETUP (pulled forward from Phase 5 because the Phase 4 test needs it). Serial `W` starts set-time mode, `W` again cancels; a serial `T` while in setup also exits it. `?` now prints the mode and portal status. A portal set also switches a fake clock back to real time. Loop sleeps 1 ms while Wi-Fi is up.
- 2026-10-28: **Phase 4, first board run (Android, via `W`): works.** Android probed `/generate_204` (host connectivitycheck.gstatic.com) → redirect → page → auto `POST /set` with `America/New_York`, table TZ applied, time set, `mode: CLOCK` after 2 s, `Wi-Fi off` after 3 s. The clock had been 2.4 s behind the phone (left over from the typed `T` in Phase 3). After RESET (assumed to be step 4; the counters were back to 0): TZ "from NVS", RTC OS flag 0, free memory 283 KB internal / 7646 KB PSRAM. Small fixes afterwards (compiled clean, not yet flashed): (1) the ppm drift figure is only printed when the last set was ≥ 1 day ago (it said -67861 ppm after minutes); (2) `shutdown()` no longer calls `WiFi.mode(WIFI_OFF)` after `softAPdisconnect(true)` had already switched Wi-Fi off, which logged `E wifi_init_default: netstack cb reg failed with 12308` (0x3014 = ESP_ERR_WIFI_STOP_STATE; harmless); (3) the boot banner prints the reset reason (`esp_reset_reason()`), so a crash can be told apart from a RESET press. Still to test: iPhone, unplug → auto setup, 5-minute timeout, bad request.
- 2026-10-28: **Phase 4 done.** Kyle: iPhone works (sheet opens and sets the time without a tap), `W` works. The bad-request test was skipped: the page auto-sets the time as soon as the phone joins, and Wi-Fi goes off 3 s later, so there's no time to open `/set` by hand. The parsing and rejection paths are covered by `tools/host_sim/portal_test.cpp`. The reboot seen after the Android test was most likely a RESET press (Kyle isn't sure); the new reset-reason line in the boot banner will show it if it happens again. Not explicitly reported: the unplug → auto-setup path and the 5-minute timeout. Re-check both during Phase 5 testing.
- 2026-10-29: **Phase 5 skipped.** Kyle: "I would prefer only to enter time set mode via unplugging. I don't need any touch functions right now." Removed `LONGPRESS_MS` and `BOOT_BTN_TRIGGER` from `clock_config.h`. The clock never polls touch (only `calibrate.cpp` does), so the FT3168's periodic NAK log line can't appear in normal running.
- 2026-10-29: **Phase 6 code written** (compiles clean: RAM 15.4%, flash 22.6%; no warnings in project code; `host_sim` all consistency checks pass; `dim_test` and `portal_test` pass; not yet run on the board).
  - **Night dimming:** new `dimmer.cpp/.h`. Once a second `main.cpp` passes the local hour (fake clock's hour when on `F`/`S`; -1 = time unknown → day) to `dimmer::schedule()`. Night = `NIGHT_START_HOUR` (22) to `NIGHT_END_HOUR` (7), wrapping midnight. A change of state fades over the new `DIM_FADE_MS` (2000 ms), stepping every 20 ms, interpolated in a square-root (gamma 2) space so the dim end doesn't rush (200 → 40: 200, 178, 158, 139, 121, 105, 89, 75, 62, 50, 40). Boot applies the scheduled level with no fade, before the first frame is drawn. `dimmer::loop()` runs at the top of `loop()`, so the fade keeps going during the hour-clear sweep (22:00 and 07:00 are both tops of the hour, so they always coincide). Serial: `B n` = manual level, held until the next day/night switch; `B` = back to the schedule; `?` shows the brightness state.
  - **Pixel orbit:** `ORBIT_PX` (still 0 = off) / `ORBIT_PERIOD_MIN`. `face::set_offset(dx, dy)` moves the canvas and labels; partial invalidations are shifted to match (`lv_obj_invalidate_area()` takes screen coordinates, checked in LVGL's source). A move repaints the screen from the canvas (no ring re-render). Positions: 8 points on a circle of `ORBIT_PX`, one step per period (millis-based), only in CLOCK mode and never mid-sweep. Config comment: keep it ≤ 2 px so the band stays ≥ 2 px from the glass edge. Serial `O dx dy` (or `O` = 0 0) previews an offset on the board. `host_sim` checks a (7, −5) offset: whole frame = shifted frame, 100 partial updates, the setup screen's breathing strips, and back to (0, 0); a mutation test (offset left out of the invalidation) fails it as it should.
  - **Debug output:** new `cfg::DEBUG_LOG` (false). Gated: per-request portal log, request bodies, "phone(s) connected", hour-clear frame stats, font/setup-screen timings, "Face up", and a new hourly `health:` free-memory line (for the 48 h soak test). Events, errors and replies to serial commands still print; `?` still shows everything.
  - **`-DCORE_DEBUG_LEVEL=0` not added**, a deliberate change from the plan: the clock never polls touch now, so the NAK log line it was meant to hide can't happen, and keeping the core's error-level logs (the default, level 1) helps if something ever goes wrong.
  - **Hotspot while the time isn't set:** new `cfg::AP_STAY_UP_UNTIL_SET` (true). With no valid time, the 5-minute timeout no longer applies: the hotspot stays up until a phone sets the time. Reason: unplugging is the only trigger now, so after a power cut with nobody around, the clock would otherwise sit at "Time not set" until someone replugs it. The timeout still applies when the clock already has a time (serial `W`). The idle screen (only reachable if Wi-Fi fails, or with the knob off) now says `Time not set / Replug to retry`.
- 2026-10-29: **Phase 6 board test 1.** Kyle: the fade looks good. His `?` shows the time set from his phone at 18:02 after a replug (portal: 7 requests, 1 set, zone from the table), so the unplug → setup → phone path works. Free memory 259 KB internal, 7463 KB PSRAM. Not reported: the `O` edge check, and whether the hotspot outlasted 5 minutes.
- 2026-10-29: **Dimming now follows the sun** (Kyle: he's near Washington, DC; "dims somewhat 1 hour after sunset, then dims further 2 hours after"; rounding is fine). Compile-checked and `dim_test`-checked; not yet flashed.
  - Three phases: DAY (`BRIGHTNESS_DAY` 200) from sunrise + `DAY_AFTER_SUNRISE_MIN` (0); EVENING (new `BRIGHTNESS_EVENING` 100, about halfway between 200 and 40 in the fade's gamma-2 space) from sunset + `EVENING_AFTER_SUNSET_MIN` (60); NIGHT (`BRIGHTNESS_NIGHT` 40) from sunset + `NIGHT_AFTER_SUNSET_MIN` (120) to sunrise. Morning wasn't specified, so it goes straight back to full at sunrise (one knob to delay it). `NIGHT_START_HOUR` / `NIGHT_END_HOUR` are gone.
  - `dimmer::sun_times_utc(yday, SUN_LAT, SUN_LON)`: solar declination + equation-of-time formula with the standard −0.833° horizon, ~10 lines of float maths. Against the `astral` Python library for DC over all of 2026, it's within 3.2 min (sunrise) and 3.1 min (sunset). `main.cpp: sun_today()` converts to local minutes using the current UTC offset (local minus UTC time of day), so DST is handled. On the fake clock the real date is used with the fake time of day; with no valid time the phase is DAY.
  - Assumes the switch times don't cross midnight: in DC the latest night start is 22:37 EDT (late June), checked in `dim_test`.
  - `?` now prints today's times, e.g. `sun today: rise 7:32, set 18:10 -> 100 at 19:10, 40 at 20:10, 200 at 7:32`.
- 2026-10-29: **Phase 6 done.** Kyle: sun-based dimming looks good (levels 200 / 100 / 40 kept). Orbit edge check (`O ±2 0`, `O 0 ±2`) looks good: the band stays visible at a 2 px shift. Orbit stays **off** (`ORBIT_PX = 0`) by his choice. Acceptance criteria ticked except the 48 h run. Still open: (1) 48 h soak: compare the free-memory line of `?` now vs. after two days (259 KB internal / 7463 KB PSRAM at the start of the Phase 6 test); (2) burn-in settings (tick opacity 180/230, orbit off) to review with Kyle after a week of use; (3) not explicitly seen: the hotspot outlasting 5 min with no time set (`AP_STAY_UP_UNTIL_SET`).

