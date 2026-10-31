// face.cpp: see face.h.
#include "face.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include "board.h"
#include "clock_config.h"
#include "fonts/nixie_one_ttf.h"
#include "ring_render.h"

namespace face {
namespace {

uint16_t *fb = nullptr;          // full-screen RGB565 canvas buffer (PSRAM)
lv_obj_t *canvas = nullptr;
lv_obj_t *hour_label = nullptr;
lv_font_t *hour_font = nullptr;

ring::BandState bands;           // what the canvas currently shows
uint32_t clear_start_ms = 0;     // when the current hour-clearing sweep began
int shown_hour = -1;
int ox = 0, oy = 0;              // pixel-orbit offset of the whole face (set_offset)
int hour_x = 0, hour_y = 0;      // numeral position before the offset

struct Box { float x1, y1, x2, y2; };  // continuous coordinates, x2/y2 exclusive
Box ink{};                             // numeral ink, relative to the label's top-left

uint32_t last_render_us = 0, max_render_us = 0, full_render_us = 0;
uint32_t renders = 0;

// Band length in degrees for a time (CLOCK_PLAN.md §5.2): 3600 s of the hour -> 360 deg.
float band_angle(int minute, int second, int millis) {
  if (!cfg::SWEEP_SMOOTH) return minute * 6.0f;
  return (minute * 60 + second + millis / 1000.0f) * 0.1f;
}

bool band_visible(float deg) { return deg >= cfg::ARC_HIDE_BELOW_DEG; }

bool clearing() { return bands.old_deg != ring::BAND_HIDDEN; }

// Marks canvas pixels (framebuffer coordinates) for redraw. lv_obj_invalidate_area() takes
// screen coordinates, so add the orbit offset.
void invalidate_fb(int x1, int y1, int x2, int y2) {
  lv_area_t a = {x1 + ox, y1 + oy, x2 + ox, y2 + oy};
  lv_obj_invalidate_area(canvas, &a);
}

void redraw(int x1, int y1, int x2, int y2) {
  uint32_t t0 = micros();
  ring::render(fb, LCD_W, x1, y1, x2, y2, bands);
  last_render_us = micros() - t0;
  bool full = (x2 - x1 + 1) * (y2 - y1 + 1) >= LCD_W * LCD_H;
  if (full) full_render_us = last_render_us;
  else if (last_render_us > max_render_us) max_render_us = last_render_us;
  renders++;
  invalidate_fb(x1, y1, x2, y2);
}

// Ink bounding box of txt as the label will draw it (mirrors lv_draw_label's glyph
// placement). tiny_ttf reports each glyph box 1 px wider and taller than stb_truetype's
// ink box (it adds 1 to an exclusive bound), hence the -1s.
Box measure_ink(const char *txt) {
  const int32_t line_h = lv_font_get_line_height(hour_font);
  const int32_t base = hour_font->base_line;
  Box b{1e9f, 1e9f, -1e9f, -1e9f};
  int32_t x = 0;
  for (const char *p = txt; *p; ++p) {
    lv_font_glyph_dsc_t g;
    if (!lv_font_get_glyph_dsc(hour_font, &g, (uint32_t)p[0], (uint32_t)p[1])) continue;
    float gx1 = x + g.ofs_x, gx2 = gx1 + g.box_w - 1;
    float gy1 = (line_h - base) - g.box_h - g.ofs_y, gy2 = gy1 + g.box_h - 1;
    if (gx1 < b.x1) b.x1 = gx1;
    if (gy1 < b.y1) b.y1 = gy1;
    if (gx2 > b.x2) b.x2 = gx2;
    if (gy2 > b.y2) b.y2 = gy2;
    x += g.adv_w + cfg::HOUR_LETTER_SPACE;
  }
  return b;
}

// Distance from the panel centre to the ink box's farthest corner, as placed on screen.
float ink_reach() {
  lv_obj_update_layout(hour_label);
  float lx = lv_obj_get_x(hour_label) - ox, ly = lv_obj_get_y(hour_label) - oy;
  float c = LCD_W / 2.0f, worst = 0;
  float xs[2] = {lx + ink.x1, lx + ink.x2}, ys[2] = {ly + ink.y1, ly + ink.y2};
  for (float x : xs)
    for (float y : ys) worst = fmaxf(worst, hypotf(x - c, y - c));
  return worst;
}

void set_hour(int h) {
  char txt[4];
  snprintf(txt, sizeof txt, "%d", h);
  lv_label_set_text(hour_label, txt);
  ink = measure_ink(txt);
  // Put the centre of the ink (not of the line box) on the panel centre.
  float cx = (ink.x1 + ink.x2) / 2.0f, cy = (ink.y1 + ink.y2) / 2.0f;
  int x = (int)lroundf(LCD_W / 2.0f - cx);
  x += cfg::HOUR_X_OFFSET;
  int y = (int)lroundf(LCD_H / 2.0f - cy) + cfg::HOUR_Y_OFFSET;
  hour_x = x, hour_y = y;
  lv_obj_set_pos(hour_label, x + ox, y + oy);
  shown_hour = h;
}

// ---- Set-time screen ----
// The ring is blended between two pre-rendered versions (band hidden / full ring), only at the
// pixels where they differ, and only the strips around the ring are invalidated, so a
// breathing frame costs well under a millisecond of blending plus the flush.
struct BlendPx { uint32_t idx; uint16_t off, on; };
BlendPx *blend = nullptr;
uint32_t blend_n = 0;
constexpr int STRIP_GROUPS = 8;              // row groups; up to 2 rects each (< LV_INV_BUF_SIZE)
lv_area_t strips[STRIP_GROUPS * 2];
int strip_n = 0;
bool setup_ = false, breathe = false;
uint8_t ring_opa = 0;
uint32_t setup_t0 = 0, last_breath = 0;
lv_obj_t *msg_label = nullptr, *check_label = nullptr;

inline uint16_t lerp565(uint16_t a, uint16_t b, uint32_t t) {  // t 0-255
  int ar = a >> 11, ag = (a >> 5) & 63, ab = a & 31;
  int br = b >> 11, bg = (b >> 5) & 63, bb = b & 31;
  int r = ar + (((br - ar) * (int)t + 127) / 255);
  int g = ag + (((bg - ag) * (int)t + 127) / 255);
  int bl = ab + (((bb - ab) * (int)t + 127) / 255);
  return (uint16_t)((r << 11) | (g << 5) | bl);
}

bool build_blend() {
  if (blend) return true;
  const size_t px = (size_t)LCD_W * LCD_H;
  uint16_t *off = (uint16_t *)heap_caps_malloc(px * 2, MALLOC_CAP_SPIRAM);
  if (!off) return false;
  ring::BandState none, full;
  full.deg = 360.0f;
  ring::render(off, LCD_W, 0, 0, LCD_W - 1, LCD_H - 1, none);
  ring::render(fb, LCD_W, 0, 0, LCD_W - 1, LCD_H - 1, full);
  uint32_t n = 0;
  for (size_t i = 0; i < px; i++) n += off[i] != fb[i];
  blend = (BlendPx *)heap_caps_malloc(n * sizeof(BlendPx), MALLOC_CAP_SPIRAM);
  if (!blend) {
    heap_caps_free(off);
    return false;
  }
  // Strips: per group of rows, the bounding box of the differing pixels left and right of
  // the centre (merged into one when they meet, e.g. at the top and bottom of the ring).
  int lx1[STRIP_GROUPS], lx2[STRIP_GROUPS], rx1[STRIP_GROUPS], rx2[STRIP_GROUPS];
  int gy1[STRIP_GROUPS], gy2[STRIP_GROUPS];
  for (int g = 0; g < STRIP_GROUPS; g++) {
    lx1[g] = rx1[g] = gy1[g] = INT32_MAX;
    lx2[g] = rx2[g] = gy2[g] = -1;
  }
  const int half = LCD_W / 2;
  blend_n = 0;
  for (size_t i = 0; i < px; i++) {
    if (off[i] == fb[i]) continue;
    blend[blend_n++] = {(uint32_t)i, off[i], fb[i]};
    int x = i % LCD_W, y = i / LCD_W, g = y * STRIP_GROUPS / LCD_H;
    if (y < gy1[g]) gy1[g] = y;
    if (y > gy2[g]) gy2[g] = y;
    if (x < half) {
      if (x < lx1[g]) lx1[g] = x;
      if (x > lx2[g]) lx2[g] = x;
    } else {
      if (x < rx1[g]) rx1[g] = x;
      if (x > rx2[g]) rx2[g] = x;
    }
  }
  heap_caps_free(off);
  strip_n = 0;
  for (int g = 0; g < STRIP_GROUPS; g++) {
    if (gy2[g] < 0) continue;
    bool l = lx2[g] >= 0, r = rx2[g] >= 0;
    if (l && r && lx2[g] + 1 >= rx1[g]) {
      strips[strip_n++] = {lx1[g], gy1[g], rx2[g], gy2[g]};
    } else {
      if (l) strips[strip_n++] = {lx1[g], gy1[g], lx2[g], gy2[g]};
      if (r) strips[strip_n++] = {rx1[g], gy1[g], rx2[g], gy2[g]};
    }
  }
  return true;
}

void set_ring_opa(uint8_t opa) {
  if (opa == ring_opa) return;
  ring_opa = opa;
  for (uint32_t i = 0; i < blend_n; i++) fb[blend[i].idx] = lerp565(blend[i].off, blend[i].on, opa);
  for (int i = 0; i < strip_n; i++)
    invalidate_fb(strips[i].x1, strips[i].y1, strips[i].x2, strips[i].y2);
}

uint8_t breath_opa(uint32_t t) {
  float ph = (t % cfg::SETUP_BREATHE_MS) / (float)cfg::SETUP_BREATHE_MS;
  float k = 0.5f - 0.5f * cosf(ph * 2.0f * (float)M_PI);  // 0 -> 1 -> 0, starting dim
  return (uint8_t)lroundf(cfg::SETUP_BREATHE_MIN + (255 - cfg::SETUP_BREATHE_MIN) * k);
}

}  // namespace

bool begin() {
  lv_obj_t *scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, lv_color_hex(cfg::BG_COLOR), 0);
  lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  fb = (uint16_t *)heap_caps_malloc(LCD_W * LCD_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
  if (!fb) {
    Serial.println("face: canvas allocation in PSRAM failed");
    return false;
  }
  canvas = lv_canvas_create(scr);
  lv_canvas_set_buffer(canvas, fb, LCD_W, LCD_H, LV_COLOR_FORMAT_RGB565);
  lv_obj_set_pos(canvas, 0, 0);
  redraw(0, 0, LCD_W - 1, LCD_H - 1);

  size_t psram_before = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  hour_font = lv_tiny_ttf_create_data(nixie_one_ttf, nixie_one_ttf_len, cfg::HOUR_FONT_PX);
  if (!hour_font) {
    Serial.println("face: tiny_ttf font creation failed");
    return false;
  }
  if (cfg::DEBUG_LOG) Serial.printf("face: font %d px, line height %d, PSRAM used by font setup %u bytes\n",
                cfg::HOUR_FONT_PX, (int)lv_font_get_line_height(hour_font),
                (unsigned)(psram_before - heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));

  hour_label = lv_label_create(scr);  // created after the canvas, so drawn on top of it
  lv_obj_set_style_text_font(hour_label, hour_font, 0);
  lv_obj_set_style_text_color(hour_label, lv_color_hex(cfg::HOUR_COLOR), 0);
  lv_obj_set_style_text_letter_space(hour_label, cfg::HOUR_LETTER_SPACE, 0);
  lv_label_set_text(hour_label, "");
  return true;
}

void show(int hour24, int minute, int second, int millis) {
  int h = hour24;
  if (cfg::HOUR_12H) {
    h = hour24 % 12;
    if (h == 0) h = 12;
  }
  if (h != shown_hour) set_hour(h);

  const float deg = band_angle(minute, second, millis);
  ring::BandState next = bands;
  next.deg = deg;

  // The band got shorter (top of the hour, or a jump back in time): clear the old band with
  // a black brush sweeping clockwise from 12 while the new band grows underneath it. A tiny
  // step back (a clock correction or re-set) just redraws the band shorter.
  bool started = false, restarted = false;
  if (band_visible(bands.deg) && deg < bands.deg - cfg::CLEAR_MIN_BACKSTEP_DEG) {
    restarted = clearing();  // a second wrap mid-sweep: the older remains just vanish
    started = true;
    next.old_deg = bands.deg;
    next.erase_deg = 0.0f;
    clear_start_ms = ::millis();
  }
  if (next.old_deg != ring::BAND_HIDDEN) {
    uint32_t t = ::millis() - clear_start_ms;
    next.erase_deg = (cfg::HOUR_CLEAR_MS == 0 || t >= cfg::HOUR_CLEAR_MS)
                         ? 360.0f
                         : t * 360.0f / cfg::HOUR_CLEAR_MS;
  }

  // Areas whose pixels can change, worked out from the old and new state.
  int r[2][4], n = 0;
  float from = started ? ring::BAND_HIDDEN : bands.deg;  // a new hour's band starts from nothing
  if ((band_visible(from) || band_visible(deg)) && fabsf(deg - from) >= 0.001f) {
    ring::band_change_bbox(from, deg, r[n][0], r[n][1], r[n][2], r[n][3]);
    n++;
  }
  if (next.old_deg != ring::BAND_HIDDEN && (started || next.erase_deg != bands.erase_deg)) {
    ring::band_change_bbox(started ? 0.0f : bands.erase_deg, next.erase_deg, r[n][0], r[n][1],
                           r[n][2], r[n][3]);
    n++;
  }
  bands = next;
  if (restarted) {
    redraw(0, 0, LCD_W - 1, LCD_H - 1);
  } else {
    for (int i = 0; i < n; i++) redraw(r[i][0], r[i][1], r[i][2], r[i][3]);
  }
  // Sweep finished: the old band is fully painted over, so dropping it changes no pixels.
  if (clearing() && bands.erase_deg >= 360.0f) bands.old_deg = ring::BAND_HIDDEN;
}

bool busy() { return clearing(); }

bool setup_enter(const char *message, bool breathe_) {
  if (!setup_) {
    uint32_t t0 = millis();
    if (!build_blend()) {
      Serial.println("face: setup screen: PSRAM allocation failed");
      return false;
    }
    lv_obj_t *scr = lv_screen_active();
    if (!msg_label) {
      msg_label = lv_label_create(scr);
      lv_obj_set_style_text_font(msg_label, &lv_font_montserrat_28, 0);
      lv_obj_set_style_text_color(msg_label, lv_color_hex(cfg::SETUP_TEXT_COLOR), 0);
      lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_style_text_line_space(msg_label, 6, 0);
      check_label = lv_label_create(scr);
      lv_obj_set_style_text_font(check_label, &lv_font_montserrat_48, 0);
      lv_obj_set_style_text_color(check_label, lv_color_hex(cfg::ARC_COLOR), 0);
      lv_label_set_text(check_label, LV_SYMBOL_OK);
      lv_obj_align(check_label, LV_ALIGN_CENTER, ox, oy);
    }
    lv_obj_add_flag(hour_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(check_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(msg_label, LV_OBJ_FLAG_HIDDEN);
    // fb currently holds the full ring (from build_blend) or an earlier blend: set every
    // blended pixel explicitly, and redraw the whole screen once.
    ring_opa = 0;
    for (uint32_t i = 0; i < blend_n; i++) fb[blend[i].idx] = blend[i].off;
    lv_obj_invalidate(canvas);
    setup_ = true;
    if (cfg::DEBUG_LOG) Serial.printf("face: setup screen in %u ms (%u ring pixels, %d strips)\n",
                  (unsigned)(millis() - t0), (unsigned)blend_n, strip_n);
  }
  setup_message(message, breathe_);
  return true;
}

void setup_message(const char *message, bool breathe_) {
  if (!setup_) return;
  lv_label_set_text(msg_label, message);
  lv_obj_align(msg_label, LV_ALIGN_CENTER, ox, oy);
  lv_obj_remove_flag(msg_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(check_label, LV_OBJ_FLAG_HIDDEN);
  breathe = breathe_;
  setup_t0 = millis();
  last_breath = 0;
  set_ring_opa(breathe ? cfg::SETUP_BREATHE_MIN : cfg::SETUP_IDLE_OPA);
}

void setup_loop() {
  if (!setup_ || !breathe) return;
  uint32_t now = millis();
  if (now - last_breath < cfg::SETUP_FRAME_MS) return;
  last_breath = now;
  set_ring_opa(breath_opa(now - setup_t0));
}

void setup_done() {
  if (!setup_) return;
  breathe = false;
  lv_obj_add_flag(msg_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(check_label, LV_OBJ_FLAG_HIDDEN);
  set_ring_opa(255);
}

void setup_exit() {
  if (!setup_) return;
  setup_ = false;
  lv_obj_add_flag(msg_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(check_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(hour_label, LV_OBJ_FLAG_HIDDEN);
  // Start the face from scratch: no band (the next show() draws it), numeral re-laid out.
  bands = ring::BandState();
  redraw(0, 0, LCD_W - 1, LCD_H - 1);
  shown_hour = -1;
}

bool in_setup() { return setup_; }

void set_offset(int dx, int dy) {
  if (dx == ox && dy == oy) return;
  ox = dx, oy = dy;
  // Moving an object invalidates its old and new areas, so the whole face repaints from the
  // canvas (no ring re-render); the strip it uncovers at the edge shows the black screen.
  lv_obj_set_pos(canvas, ox, oy);
  lv_obj_set_pos(hour_label, hour_x + ox, hour_y + oy);
  if (msg_label) {
    lv_obj_align(msg_label, LV_ALIGN_CENTER, ox, oy);
    lv_obj_align(check_label, LV_ALIGN_CENTER, ox, oy);
  }
}

void offset(int &dx, int &dy) { dx = ox, dy = oy; }

void print_status() {
  if (setup_) Serial.printf("face: SETUP screen, ring opacity %u%s\n", ring_opa, breathe ? " (breathing)" : "");
  Serial.printf("face: hour %d, band %.2f deg%s%s, orbit offset (%d,%d)\n", shown_hour, bands.deg,
                band_visible(bands.deg) ? "" : " (hidden)", clearing() ? ", clearing old band" : "",
                ox, oy);
  if (hour_label) {
    float reach = ink_reach();
    Serial.printf("  numeral ink %.0f x %.0f px at (%d,%d), farthest corner %.1f px from centre; "
                  "ring inner edge %.1f px -> %s\n",
                  ink.x2 - ink.x1, ink.y2 - ink.y1, (int)lv_obj_get_x(hour_label),
                  (int)lv_obj_get_y(hour_label), reach, ring::inner_radius(),
                  reach < ring::inner_radius() ? "fits" : "TOO BIG");
  }
  Serial.printf("  ring renders: %u, last %u us, max partial %u us, full %u us\n", (unsigned)renders,
                (unsigned)last_render_us, (unsigned)max_render_us, (unsigned)full_render_us);
  Serial.printf("  free heap: internal %u KB, PSRAM %u KB\n",
                (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
}

}  // namespace face
