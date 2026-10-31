// ring_render.cpp: see ring_render.h.
//
// Conventions: pixel (px, py) covers [px, px+1) x [py, py+1); its centre is (px+0.5, py+0.5).
// Positions are taken relative to the panel centre (233.0, 233.0) with y pointing down.
// Angles are clockwise from 12 o'clock. Each shape is described by a signed distance
// (negative inside, in px); coverage = clamp(0.5 - distance, 0, 1) gives ~1 px of anti-aliasing.
#include "ring_render.h"
#include <math.h>
#include "board.h"
#include "clock_config.h"

namespace ring {
namespace {

constexpr float PI_F = 3.14159265358979f;
constexpr float DEG = PI_F / 180.0f;

// ---- Geometry, all derived from clock_config.h ----
constexpr float CX = LCD_W / 2.0f;                                  // 233.0
constexpr float CY = LCD_H / 2.0f;
constexpr float BAND_OUT = CX - cfg::EDGE_MARGIN;                   // outer edge of the band
constexpr float BAND_HALF = cfg::ARC_WIDTH / 2.0f;
constexpr float BAND_MID = BAND_OUT - BAND_HALF;                    // centre line of the band
constexpr float BRUSH_HALF = BAND_HALF + 1.0f;                      // hour-clearing brush
constexpr float TICK_OUT = CX - cfg::EDGE_MARGIN - cfg::TICK_OUTER_INSET;
constexpr int TICK_LONGEST = cfg::TICK_MAJOR_LEN > cfg::TICK_MINOR_LEN ? cfg::TICK_MAJOR_LEN
                                                                       : cfg::TICK_MINOR_LEN;
constexpr float INNER = (BAND_OUT - cfg::ARC_WIDTH) < (TICK_OUT - TICK_LONGEST)
                            ? (BAND_OUT - cfg::ARC_WIDTH)
                            : (TICK_OUT - TICK_LONGEST);
constexpr float OUTER = BAND_OUT > TICK_OUT ? BAND_OUT : TICK_OUT;
constexpr float ZONE_IN = INNER - 1.5f;   // pixels outside [ZONE_IN, ZONE_OUT] are background
constexpr float ZONE_OUT = OUTER + 1.5f;

struct Rgb { float r, g, b; };
constexpr Rgb rgb(uint32_t c) {
  return {float((c >> 16) & 0xFF), float((c >> 8) & 0xFF), float(c & 0xFF)};
}
constexpr Rgb BG = rgb(cfg::BG_COLOR);
constexpr Rgb ARC = rgb(cfg::ARC_COLOR);
constexpr Rgb TICK = rgb(cfg::TICK_COLOR);

struct Tick {
  float ux, uy;        // unit vector from the centre toward the tick
  float half_w, half_len, r_mid;
  float opa;           // 0-1
};
Tick ticks[12];
bool ticks_ready = false;

void init_ticks() {
  for (int i = 0; i < 12; i++) {
    bool major = (i % 3 == 0) && (i != 0 || cfg::TICK_MAJOR_AT_12);
    float a = i * 30.0f * DEG;
    float len = major ? cfg::TICK_MAJOR_LEN : cfg::TICK_MINOR_LEN;
    Tick &t = ticks[i];
    t.ux = sinf(a);
    t.uy = -cosf(a);
    t.half_w = (major ? cfg::TICK_MAJOR_WIDTH : cfg::TICK_MINOR_WIDTH) / 2.0f;
    t.half_len = len / 2.0f;
    t.r_mid = TICK_OUT - len / 2.0f;
    t.opa = (major ? cfg::TICK_MAJOR_OPA : cfg::TICK_MINOR_OPA) / 255.0f;
  }
  ticks_ready = true;
}

inline float coverage(float sd) {
  if (sd >= 0.5f) return 0.0f;
  if (sd <= -0.5f) return 1.0f;
  return 0.5f - sd;
}

// Signed distance to an axis-aligned box of half extents (hx, hy) centred on the origin.
inline float box_sd(float px, float py, float hx, float hy) {
  float qx = fabsf(px) - hx, qy = fabsf(py) - hy;
  float ox = qx > 0 ? qx : 0, oy = qy > 0 ? qy : 0;
  float inside = qx > qy ? qx : qy;
  return sqrtf(ox * ox + oy * oy) + (inside < 0 ? inside : 0);
}

// Distance from p to the ray leaving the centre in direction (ux, uy).
inline float ray_dist(float x, float y, float d, float ux, float uy) {
  float along = x * ux + y * uy;
  if (along <= 0) return d;
  return fabsf(x * -uy + y * ux);
}

// Per-render band parameters.
struct Band {
  bool visible, full;
  float a_rad;          // sweep in radians
  float half;           // half width (px)
  bool round_start;     // round tail at 12; else flat and nothing drawn behind 12
  float ex, ey;         // unit vector toward the leading edge
  float opa;            // 0-1
};

// always_visible: draw it however short (the clearing brush), else hide it below the threshold.
Band make_band(float deg, uint8_t opa, bool always_visible = false, float half = BAND_HALF,
               bool round_start = cfg::ARC_ROUND_START) {
  Band b{};
  b.visible = (always_visible ? deg >= 0.0f : deg >= cfg::ARC_HIDE_BELOW_DEG) && opa > 0;
  b.full = deg >= 360.0f;
  b.a_rad = deg * DEG;
  b.half = half;
  b.round_start = round_start;
  b.ex = sinf(b.a_rad);
  b.ey = -cosf(b.a_rad);
  b.opa = opa / 255.0f;
  return b;
}

constexpr float FAR = 1e6f;  // signed distance meaning "nowhere near"

// Signed distance from p (relative to the centre, d = |p|, theta = clockwise angle from 12)
// to band b.
inline float band_sd(const Band &b, float x, float y, float d, float theta) {
  if (!b.visible) return FAR;
  float sd = fabsf(d - BAND_MID) - b.half;  // the full annulus
  if (!b.full) {
    // Clip the annulus to the wedge [0, a] between the 12 o'clock ray and the leading ray.
    float ds = ray_dist(x, y, d, 0.0f, -1.0f);
    float de = ray_dist(x, y, d, b.ex, b.ey);
    float edge = ds < de ? ds : de;
    float wedge = theta <= b.a_rad ? -edge : edge;
    if (wedge > sd) sd = wedge;
    if (cfg::ARC_ROUND_CAPS) {
      float qx = x - b.ex * BAND_MID, qy = y - b.ey * BAND_MID;
      float ce = sqrtf(qx * qx + qy * qy) - b.half;  // leading cap
      // With a flat tail, a short band's leading cap mustn't poke out behind 12 either:
      // clip it to the right of the 12 o'clock line while the band is under 90 deg.
      if (!b.round_start && b.a_rad < PI_F / 2 && -x > ce) ce = -x;
      if (ce < sd) sd = ce;
      if (b.round_start) {
        float sx = x, sy = y + BAND_MID;  // tail cap centre: (0, -BAND_MID)
        float cs = sqrtf(sx * sx + sy * sy) - b.half;
        if (cs < sd) sd = cs;
      }
    }
  }
  return sd;
}

inline float tick_cov(float x, float y, float theta) {
  int k = int(theta * (6.0f / PI_F) + 0.5f) % 12;  // nearest tick (30 deg apart)
  const Tick &t = ticks[k];
  float radial = x * t.ux + y * t.uy - t.r_mid;
  float across = x * -t.uy + y * t.ux;
  return coverage(box_sd(across, radial, t.half_w, t.half_len)) * t.opa;
}

inline void mix(Rgb &c, const Rgb &top, float a) {
  c.r += (top.r - c.r) * a;
  c.g += (top.g - c.g) * a;
  c.b += (top.b - c.b) * a;
}

inline uint16_t pack565(const Rgb &c) {
  int r = int(c.r + 0.5f), g = int(c.g + 0.5f), b = int(c.b + 0.5f);
  return uint16_t(((r * 31 + 127) / 255) << 11 | ((g * 63 + 127) / 255) << 5 | ((b * 31 + 127) / 255));
}

inline int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

}  // namespace

float inner_radius() { return INNER; }

void render(uint16_t *fb, int fb_w, int x1, int y1, int x2, int y2, const BandState &bands,
            uint8_t band_opa) {
  if (!ticks_ready) init_ticks();
  x1 = clampi(x1, 0, LCD_W - 1); x2 = clampi(x2, 0, LCD_W - 1);
  y1 = clampi(y1, 0, LCD_H - 1); y2 = clampi(y2, 0, LCD_H - 1);
  const Band band = make_band(bands.deg, band_opa);
  const bool erasing = bands.old_deg != BAND_HIDDEN;
  const Band old_band = make_band(erasing ? bands.old_deg : BAND_HIDDEN, band_opa);
  // The brush is 1 px wider each side than the band so it leaves no anti-aliased fringe.
  // Its tail is always round (with round caps): touching down at 12 it also removes the old
  // band's leading cap, which overhangs 12 at the end of the hour.
  const Band brush = make_band(erasing ? bands.erase_deg : BAND_HIDDEN, 255, true, BRUSH_HALF,
                               cfg::ARC_ROUND_CAPS);
  const bool any_band = band.visible || old_band.visible;
  const uint16_t bg565 = pack565(BG);
  const float zin2 = ZONE_IN * ZONE_IN, zout2 = ZONE_OUT * ZONE_OUT;

  for (int py = y1; py <= y2; py++) {
    float y = py + 0.5f - CY;
    uint16_t *row = fb + py * fb_w;
    for (int px = x1; px <= x2; px++) {
      float x = px + 0.5f - CX;
      float d2 = x * x + y * y;
      if (d2 < zin2 || d2 > zout2) {
        row[px] = bg565;
        continue;
      }
      float d = sqrtf(d2);
      float theta = atan2f(x, -y);
      if (theta < 0) theta += 2 * PI_F;
      float a_band = 0.0f;
      if (any_band) {
        float sd = band_sd(band, x, y, d, theta);
        if (old_band.visible) {
          // What's left of the old band: outside the brush stroke.
          float rest = band_sd(old_band, x, y, d, theta);
          float stroke = band_sd(brush, x, y, d, theta);
          if (-stroke > rest) rest = -stroke;
          if (rest < sd) sd = rest;
        }
        a_band = coverage(sd) * band.opa;
      }
      float a_tick = tick_cov(x, y, theta);
      Rgb c = BG;
      if (cfg::TICKS_ABOVE_ARC) {
        mix(c, ARC, a_band);
        mix(c, TICK, a_tick);
      } else {
        mix(c, TICK, a_tick);
        mix(c, ARC, a_band);
      }
      row[px] = pack565(c);
    }
  }
}

void band_change_bbox(float from_deg, float to_deg, int &x1, int &y1, int &x2, int &y2) {
  float lo = from_deg < to_deg ? from_deg : to_deg;
  float hi = from_deg < to_deg ? to_deg : from_deg;
  // A hidden band counts as length 0: showing or hiding it changes the region from 12 to hi.
  if (lo < cfg::ARC_HIDE_BELOW_DEG) lo = 0.0f;
  if (hi < 0.0f) hi = 0.0f;
  if (hi > 360.0f) hi = 360.0f;
  if (hi - lo > 90.0f) {
    x1 = y1 = 0;
    x2 = LCD_W - 1;
    y2 = LCD_H - 1;
    return;
  }
  // Sample the band's centre line between the two ends; each sample covers a disc of
  // the band's half width plus anti-aliasing margin. Samples are <= 2 deg (~8 px) apart.
  const float pad = BRUSH_HALF + 2.0f;
  int steps = int((hi - lo) / 2.0f) + 1;
  float fx1 = 1e9f, fy1 = 1e9f, fx2 = -1e9f, fy2 = -1e9f;
  for (int i = 0; i <= steps; i++) {
    float a = (lo + (hi - lo) * i / steps) * DEG;
    float cx = CX + sinf(a) * BAND_MID, cy = CY - cosf(a) * BAND_MID;
    if (cx - pad < fx1) fx1 = cx - pad;
    if (cy - pad < fy1) fy1 = cy - pad;
    if (cx + pad > fx2) fx2 = cx + pad;
    if (cy + pad > fy2) fy2 = cy + pad;
  }
  x1 = clampi(int(floorf(fx1)), 0, LCD_W - 1);
  y1 = clampi(int(floorf(fy1)), 0, LCD_H - 1);
  x2 = clampi(int(ceilf(fx2)), 0, LCD_W - 1);
  y2 = clampi(int(ceilf(fy2)), 0, LCD_H - 1);
}

}  // namespace ring
