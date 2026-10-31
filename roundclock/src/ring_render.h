// ring_render.h: software renderer for the minute band and the 12 ticks (CLOCK_PLAN.md §5.2-5.3).
//
// Everything is drawn with float geometry around the true panel centre (233.0, 233.0) and
// anti-aliased from signed distances, so the band's leading edge moves continuously (no
// 1-degree steps) and the ticks are symmetric. Plain C++, no Arduino/LVGL, so it can also be
// built on a PC to preview the face (tools/host_sim).
#pragma once
#include <stdint.h>

namespace ring {

// Band lengths are in degrees clockwise from 12 o'clock. Below cfg::ARC_HIDE_BELOW_DEG a
// band is hidden; 360 or more draws a full ring.
constexpr float BAND_HIDDEN = -1.0f;

// What the band layer shows. At the top of the hour the previous hour's band is cleared by a
// black "brush" that sweeps clockwise from 12: old_deg is that band's length and erase_deg is
// how far the brush has got (it paints over [0, erase_deg] with the band's cap shape). The new
// hour's band (deg) is drawn on top, so the brush never hides it.
struct BandState {
  float deg = BAND_HIDDEN;      // this hour's band
  float old_deg = BAND_HIDDEN;  // band being cleared; BAND_HIDDEN = no clear in progress
  float erase_deg = 0.0f;       // clearing brush position
};

// Renders the inclusive rectangle (x1,y1)-(x2,y2) of the face (background, band, ticks)
// into an RGB565 frame buffer of width fb_w. band_opa scales the band's opacity (0-255).
void render(uint16_t *fb, int fb_w, int x1, int y1, int x2, int y2, const BandState &bands,
            uint8_t band_opa = 255);

// Screen rectangle covering every pixel that can change when the end of a band (or of the
// clearing brush) moves from from_deg to to_deg (either order). A hidden band counts as
// length 0. Spans over 90 deg, or reaching 360, give the whole panel. Clipped to the panel.
void band_change_bbox(float from_deg, float to_deg, int &x1, int &y1, int &x2, int &y2);

// Innermost radius (from the centre) that the band or ticks reach. The hour numeral
// must stay inside this.
float inner_radius();

}  // namespace ring
