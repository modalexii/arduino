// face.h: builds and updates the clock face (ticks, minute band, hour numeral) from
// clock_config.h (CLOCK_PLAN.md §5.2-5.4).
//
// The band and ticks are drawn by ring_render.cpp into a full-screen RGB565 canvas in PSRAM;
// only the few pixels around the band's moving end are re-rendered on each update. The hour
// numeral is an LVGL label using the embedded Nixie One TTF (tiny_ttf), centred on its ink.
#pragma once
#include <stdint.h>

namespace face {

// Creates the face on the active LVGL screen. Call after display_init(). False if the
// canvas or font can't be allocated.
bool begin();

// Shows a local time (24 h hour). Cheap when nothing visible changed.
void show(int hour24, int minute, int second, int millis);

// True while an animation is running (the top-of-the-hour clearing sweep). Call show()
// at frame rate (~30 fps) until it returns false.
bool busy();

// ---- Set-time (SETUP) screen (CLOCK_PLAN.md §5.5) ----
// The numeral is hidden, the ticks stay, the band is a full ring, and a small grey message
// sits in the centre. Don't call show() while in setup. setup_enter() takes ~0.3 s (it
// pre-renders the ring with and without the band so it can blend between them cheaply).
// breathe = the ring's opacity pulses at ~0.5 Hz; else it is shown dim and still.
bool setup_enter(const char *message, bool breathe);
void setup_message(const char *message, bool breathe);
void setup_loop();    // call every loop while in setup: advances the breathing
void setup_done();    // success: an orange check mark, ring at full brightness
void setup_exit();    // back to the clock face (one full redraw); then call show()
bool in_setup();

// Pixel orbit: moves the whole face (ring, numeral, setup text) by (dx, dy) screen pixels.
// Repaints the screen once (~a full flush; no ring re-render). Don't call during busy().
void set_offset(int dx, int dy);
void offset(int &dx, int &dy);

// Serial debug: numeral ink box and fit, band angle, render timings, free heap/PSRAM.
void print_status();

}  // namespace face
