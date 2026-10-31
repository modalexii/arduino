// clock_config.h: every visual and behaviour knob for the clock, and nothing else.
// Restyle the clock by editing values here and re-flashing. face.cpp derives every
// pixel from these constants; there are no magic numbers in the drawing code.
#pragma once
#include <stdint.h>

namespace cfg {

// ---------- Colours (0xRRGGBB) ----------
constexpr uint32_t BG_COLOR          = 0x000000;  // background (black = pixels off on AMOLED)
constexpr uint32_t HOUR_COLOR        = 0xFFFFFF;  // hour numeral
constexpr uint32_t ARC_COLOR         = 0xFF8A1E;  // minute band: nixie-tube orange
constexpr uint32_t TICK_COLOR        = 0xFFFFFF;  // 5-minute indices
constexpr uint8_t  TICK_MINOR_OPA    = 180;       // 0-255; fine 5-minute ticks
constexpr uint8_t  TICK_MAJOR_OPA    = 230;       // 15/30/45 (and 12) ticks

// ---------- Geometry (px; screen 466x466, centre 233,233) ----------
constexpr int  EDGE_MARGIN         = 4;    // glass edge -> outer edge of the minute band
constexpr int  ARC_WIDTH           = 12;   // band thickness ("medium-thick")
constexpr bool ARC_ROUND_CAPS      = true; // rounded leading end (false = square)
constexpr bool ARC_ROUND_START     = false;// rounded tail at 12 (false = flat, starting exactly at 12; Kyle)
constexpr float ARC_HIDE_BELOW_DEG = 0.5f; // band hidden until it's this long (0.5 deg = 5 s past the hour)
constexpr int  TICK_MINOR_LEN      = 12;   // measured inward from the band's outer edge
constexpr int  TICK_MINOR_WIDTH    = 2;    // stroke width of fine ticks
constexpr int  TICK_MAJOR_LEN      = 20;   // length of bold ticks
constexpr int  TICK_MAJOR_WIDTH    = 4;    // stroke width of bold ticks
constexpr bool TICK_MAJOR_AT_12    = true; // true = 12 o'clock tick is bold too (symmetry)
constexpr int  TICK_OUTER_INSET    = 0;    // >0 pulls ticks inward away from the edge
constexpr bool TICKS_ABOVE_ARC     = true; // draw ticks over the orange band

// ---------- Hour numeral ----------
constexpr int  HOUR_FONT_PX        = 260;  // tiny_ttf renders at any size: just change this
constexpr int  HOUR_X_OFFSET       = 1;    // + moves right; nudge for optical centring (Kyle: 1 px right)
constexpr int  HOUR_Y_OFFSET       = -2;   // + moves down; nudge for optical centring (Kyle: 2 px up)
constexpr int  HOUR_LETTER_SPACE   = 0;    // tighten "10", "11", "12"
constexpr bool HOUR_12H            = true; // 1-12, no leading zero

// ---------- Motion ----------
constexpr bool     SWEEP_SMOOTH       = true; // false = band jumps once per minute (6 deg)
constexpr uint32_t SWEEP_UPDATE_MS    = 1000; // ~0.4 px/s at the rim, so 1 s is plenty
constexpr uint32_t HOUR_CLEAR_MS      = 1200; // top of the hour: a black brush sweeps the old band
                                              // away clockwise from 12 in this long (0 = instantly)
constexpr uint32_t ANIM_FRAME_MS      = 16;   // frame interval while animating (16 = ~60 fps)
constexpr float    CLEAR_MIN_BACKSTEP_DEG = 1.0f; // the band getting shorter by less than this
                                              // (10 s; e.g. a clock correction) just redraws it
                                              // shorter; more (top of the hour) runs the sweep

// ---------- Brightness / AMOLED care ----------
// Dimming follows the sun (Kyle: dim somewhat 1 h after sunset, further 2 h after; full again at
// sunrise). Sunrise/sunset come from a simple formula for SUN_LAT/SUN_LON, within ~3 min of
// exact all year (checked against the astral library).
constexpr uint8_t  BRIGHTNESS_DAY     = 200;  // 0-255: sunrise (+DAY_AFTER_SUNRISE_MIN) to evening
constexpr uint8_t  BRIGHTNESS_EVENING = 100;  // 0-255: from EVENING_AFTER_SUNSET_MIN
constexpr uint8_t  BRIGHTNESS_NIGHT   = 40;   // 0-255: from NIGHT_AFTER_SUNSET_MIN until sunrise
constexpr int      EVENING_AFTER_SUNSET_MIN = 60;
constexpr int      NIGHT_AFTER_SUNSET_MIN   = 120;
constexpr int      DAY_AFTER_SUNRISE_MIN    = 0;  // e.g. 60 = stay dim until an hour after sunrise
constexpr float    SUN_LAT            = 38.9f;  // degrees north: Washington, DC
constexpr float    SUN_LON            = -77.0f; // degrees east (west is negative)
constexpr uint32_t DIM_FADE_MS        = 2000; // day <-> night fade length (0 = switch instantly)
// Pixel orbit (burn-in care): every ORBIT_PERIOD_MIN minutes the whole face moves to the next
// of 8 positions on a circle of ORBIT_PX px around the centre. 0 = off. Keep it <= 2: with
// EDGE_MARGIN 4 the band's outer edge then stays >= 2 px from the glass edge (still visible).
constexpr int      ORBIT_PX           = 0;
constexpr int      ORBIT_PERIOD_MIN   = 10;   // minutes per orbit step

// ---------- Time setting ----------
constexpr const char *AP_SSID         = "Clock";
constexpr const char *AP_PASSWORD     = "";   // "" = open; >=8 chars = WPA2
// Set-time mode starts by itself when the clock boots without a valid time (after an unplug).
// There is no touch or button trigger (Phase 5 skipped, Kyle's choice); serial W also works.
constexpr uint32_t AP_TIMEOUT_S       = 300;  // hotspot shuts off after this long with no time received...
constexpr bool     AP_STAY_UP_UNTIL_SET = true; // ...except while the clock has no time at all:
                                              // then it stays up until a phone sets it (e.g. after
                                              // a power cut while nobody was around)
constexpr const char *DEFAULT_TZ      = "EST5EDT,M3.2.0,M11.1.0"; // America/New_York
// Set-time screen (§5.5): ticks + a full orange ring that "breathes", small grey text.
constexpr uint32_t SETUP_TEXT_COLOR   = 0x808080; // "Join Wi-Fi / Clock" message
constexpr uint32_t SETUP_BREATHE_MS   = 2000;     // one breath (0.5 Hz)
constexpr uint8_t  SETUP_BREATHE_MIN  = 40;       // ring opacity at the bottom of a breath (0-255)
constexpr uint8_t  SETUP_IDLE_OPA     = 60;       // still ring while the hotspot is off
constexpr uint32_t SETUP_FRAME_MS     = 40;       // breathing frame interval (25 fps)
constexpr uint32_t SETUP_DONE_MS      = 2000;     // how long the check mark shows after a set

// ---------- Development ----------
// 0 = real time (RTC + TZ; Phase 3). N > 0 = boot into a fake clock at N x real speed
// (60 = one hour per minute) to preview the face, starting at FAKE_START_HOUR:FAKE_START_MIN.
// Serial commands switch live: "S 60" / "F 10 30 0" (fake clock), "R" (back to real time).
constexpr int FAKE_TIME_SPEED         = 0;
constexpr int FAKE_START_HOUR         = 11;   // fake clock start, 24 h
constexpr int FAKE_START_MIN          = 50;
// Chatty serial output (per-request portal log, render/animation timings, an hourly free-memory
// line). Events, errors and the replies to serial commands (incl. "?") print either way.
constexpr bool DEBUG_LOG              = false;

}  // namespace cfg
