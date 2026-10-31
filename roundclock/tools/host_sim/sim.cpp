// sim.cpp: runs face.cpp + ring_render.cpp with real LVGL on a PC and dumps frames, so the
// face can be checked without the board.
//   make LVGL=<path to lvgl 9.2.2>          (in the cloud container: ../../.pio/libdeps/amoled143/lvgl)
//   ./sim                 -> a.raw ... f.raw (sample times), g_inc.raw (after a 200 s sweep),
//                            clear1_*.raw / clear60_*.raw (top-of-the-hour clearing sweep at 1x
//                            and 60x), plus consistency checks (must print "0 mismatches"),
//                            incl. a small backward clock step (redraw only, no sweep)
//                            setup_breathe.raw / setup_idle.raw / setup_done.raw (set-time
//                            screen, Phase 4) and its checks (blend at 255 = full ring; back
//                            to the face afterwards = a fresh render)
//                            orbit.raw (face moved 7 px right, 5 px up; Phase 6) and its checks
//   ./sim 13 10 50 x.raw  -> one frame at 13:10:50
//   python3 to_png.py a.raw a.png
// Frames are 466x466 little-endian RGB565. g_inc.raw must be byte-identical to "./sim 13 10 50".
#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include "board.h"
#include "clock_config.h"
#include "face.h"
#include "ring_render.h"

static std::vector<uint16_t> screen(LCD_W * LCD_H);
static uint32_t lv_ms = 0;  // LVGL's tick, separate from the face's millis()

static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *px) {
  uint16_t *p = (uint16_t *)px;
  for (int y = a->y1; y <= a->y2; y++)
    for (int x = a->x1; x <= a->x2; x++) screen[y * LCD_W + x] = *p++;
  lv_display_flush_ready(d);
}
static void rounder(lv_event_t *e) {
  lv_area_t *a = (lv_area_t *)lv_event_get_param(e);
  a->x1 &= ~1; a->y1 &= ~1; a->x2 |= 1; a->y2 |= 1;
}
static void dump(const char *fn) {
  FILE *f = fopen(fn, "wb");
  fwrite(screen.data(), 2, screen.size(), f);
  fclose(f);
}
static void run() {
  for (int i = 0; i < 3; i++) { lv_ms += 40; lv_timer_handler(); }
}
static void show_ms(uint64_t day_ms) {
  uint32_t t = day_ms / 1000;
  face::show(t / 3600, t / 60 % 60, t % 60, day_ms % 1000);
}
static float angle(uint64_t day_ms) { return (day_ms % 3600000) / 10000.0f; }

// Compares the ring zone (outside the numeral) of the screen with a fresh full render of st.
static int check_ring(const ring::BandState &st) {
  static std::vector<uint16_t> ref(LCD_W * LCD_H);
  ring::render(ref.data(), LCD_W, 0, 0, LCD_W - 1, LCD_H - 1, st);
  int bad = 0;
  for (int y = 0; y < LCD_H; y++)
    for (int x = 0; x < LCD_W; x++) {
      float dx = x + 0.5f - LCD_W / 2.0f, dy = y + 0.5f - LCD_H / 2.0f;
      if (dx * dx + dy * dy > 195.0f * 195.0f && screen[y * LCD_W + x] != ref[y * LCD_W + x]) bad++;
    }
  return bad;
}

// Set-time screen (Phase 4): enter, breathe, idle, done, exit.
static int setup_test() {
  int bad = 0;
  face::show(13, 20, 30, 0);
  run();
  if (!face::setup_enter("Join Wi-Fi\nClock", true)) return 1;
  // Mid-breath (half a period in = brightest), then a quarter in.
  for (uint32_t t = 0; t <= cfg::SETUP_BREATHE_MS / 4; t += cfg::SETUP_FRAME_MS) {
    sim_now_ms += cfg::SETUP_FRAME_MS;
    face::setup_loop();
    run();
  }
  dump("setup_breathe.raw");
  face::setup_message("Time not set", false);
  run();
  dump("setup_idle.raw");
  face::setup_done();
  run();
  dump("setup_done.raw");
  ring::BandState full;
  full.deg = 360.0f;
  int b1 = check_ring(full);
  printf("setup done: ring vs full render: %d mismatches\n", b1);
  face::print_status();
  face::setup_exit();
  face::show(13, 21, 0, 0);
  run();
  ring::BandState st;
  st.deg = 21 * 6.0f;
  int b2 = check_ring(st);
  printf("setup exit: face vs fresh render: %d mismatches, busy %d\n", b2, face::busy());
  return b1 + b2 + (face::busy() ? 1 : 0);
}

// Pixel orbit (Phase 6). Ring zone of the screen vs a fresh full render shifted by (dx, dy);
// pixels the shift uncovers must be background.
static int check_ring_off(const ring::BandState &st, int dx, int dy) {
  static std::vector<uint16_t> ref(LCD_W * LCD_H);
  ring::render(ref.data(), LCD_W, 0, 0, LCD_W - 1, LCD_H - 1, st);
  int bad = 0;
  for (int y = 0; y < LCD_H; y++)
    for (int x = 0; x < LCD_W; x++) {
      int sx = x - dx, sy = y - dy;
      float ex = sx + 0.5f - LCD_W / 2.0f, ey = sy + 0.5f - LCD_H / 2.0f;
      if (ex * ex + ey * ey <= 195.0f * 195.0f) continue;
      bool out = sx < 0 || sy < 0 || sx >= LCD_W || sy >= LCD_H;
      uint16_t want = out ? 0 : ref[sy * LCD_W + sx];
      if (screen[y * LCD_W + x] != want) bad++;
    }
  return bad;
}

static int orbit_test() {
  const int dx = 7, dy = -5;  // bigger than the update boxes' margins, so a missing offset shows
  int bad = 0;
  face::show(14, 10, 0, 0);
  while (face::busy()) { sim_now_ms += 33; face::show(14, 10, 0, 0); }
  run();
  std::vector<uint16_t> a = screen;  // offset 0
  face::set_offset(dx, dy);
  run();
  // Whole screen (numeral too) = the unshifted frame moved by (dx, dy); uncovered = black.
  int whole = 0;
  for (int y = 0; y < LCD_H; y++)
    for (int x = 0; x < LCD_W; x++) {
      int sx = x - dx, sy = y - dy;
      bool out = sx < 0 || sy < 0 || sx >= LCD_W || sy >= LCD_H;
      if (screen[y * LCD_W + x] != (out ? 0 : a[sy * LCD_W + sx])) whole++;
    }
  dump("orbit.raw");
  // Partial band updates after the move must land shifted too.
  for (int i = 1; i <= 100; i++) { face::show(14, 10 + i / 60, i % 60, 0); run(); }
  ring::BandState st;
  st.deg = (600 + 100) * 0.1f;
  int inc = check_ring_off(st, dx, dy);
  // Setup screen under the offset: breathing strips, then the full ring.
  face::setup_enter("Join Wi-Fi\nClock", true);
  for (int i = 0; i < 10; i++) { sim_now_ms += cfg::SETUP_FRAME_MS; face::setup_loop(); run(); }
  face::setup_done();
  run();
  ring::BandState full;
  full.deg = 360.0f;
  int setup = check_ring_off(full, dx, dy);
  face::setup_exit();
  face::set_offset(0, 0);
  face::show(14, 12, 0, 0);
  run();
  st.deg = 72.0f;
  int back = check_ring(st);
  printf("orbit (%d,%d): whole frame %d, after 100 updates %d, setup ring %d, back at (0,0) %d "
         "mismatches\n", dx, dy, whole, inc, setup, back);
  bad = whole + inc + setup + back;
  return bad;
}

// Runs the clock through the top of the hour at `speed`, frames every 33 ms, checking every
// frame against the expected band state and dumping a few.
static int clear_test(int speed) {
  uint64_t day_ms = 12ULL * 3600000 + 59 * 60000 + 59000;  // 12:59:59
  show_ms(day_ms); run();
  float old_deg = -1, last_deg = angle(day_ms);
  uint32_t wrap_at = 0;
  int mismatches = 0, frame = 0;
  for (uint32_t elapsed = 0; elapsed <= 2600; elapsed += 33, frame++) {
    sim_now_ms += 33;
    day_ms += 33ULL * speed;
    float deg = angle(day_ms);
    if (deg < last_deg && old_deg < 0) { old_deg = last_deg; wrap_at = sim_now_ms; }
    show_ms(day_ms); run();
    ring::BandState st;
    st.deg = deg;
    if (old_deg >= 0) {
      float e = cfg::HOUR_CLEAR_MS ? (sim_now_ms - wrap_at) * 360.0f / cfg::HOUR_CLEAR_MS : 360;
      if (e < 360) { st.old_deg = old_deg; st.erase_deg = e; }
    }
    mismatches += check_ring(st);
    last_deg = deg;
    static int shot;  // a frame every ~200 ms from the wrap: clearN_00 ... clearN_05
    if (old_deg < 0) shot = 0;
    else if (shot < 6 && sim_now_ms - wrap_at >= shot * 200u) {
      char fn[32];
      snprintf(fn, sizeof fn, "clear%d_%02d.raw", speed, shot++);
      dump(fn);
    }
  }
  printf("clear sweep at %dx: %d frames, %d mismatches, busy at end: %d\n", speed, frame, mismatches,
         face::busy());
  return mismatches;
}

int main(int argc, char **argv) {
  lv_init();
  lv_tick_set_cb([]() -> uint32_t { return lv_ms; });
  lv_display_t *d = lv_display_create(LCD_W, LCD_H);
  size_t bytes = LCD_W * (LCD_H / 10) * 2;
  void *b1 = malloc(bytes), *b2 = malloc(bytes);
  lv_display_set_buffers(d, b1, b2, bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(d, flush);
  lv_display_add_event_cb(d, rounder, LV_EVENT_INVALIDATE_AREA, NULL);
  if (!face::begin()) return 1;
  if (argc == 5) {
    face::show(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), 0);
    run();
    dump(argv[4]);
    return 0;
  }
  struct { int h, m, s; const char *fn; } shots[] = {{11, 50, 0, "a.raw"}, {12, 0, 3, "b.raw"},
      {12, 0, 8, "c.raw"}, {13, 7, 30, "d.raw"}, {22, 45, 0, "e.raw"}, {9, 59, 59, "f.raw"}};
  for (auto &s : shots) {
    face::show(s.h, s.m, s.s, 0);
    // a backwards jump starts a clearing sweep; let it finish before dumping
    for (int i = 0; i < 100 && face::busy(); i++) { sim_now_ms += 33; face::show(s.h, s.m, s.s, 0); }
    run();
    dump(s.fn);
    face::print_status();
  }
  // Incremental consistency: sweep from 13:07:30 in 1 s steps; must equal a fresh full frame.
  face::show(13, 7, 30, 0);
  while (face::busy()) { sim_now_ms += 33; face::show(13, 7, 30, 0); }
  run();
  for (int i = 1; i <= 200; i++) {
    face::show(13, 7 + (30 + i) / 60, (30 + i) % 60, 0);
    run();
  }
  dump("g_inc.raw");
  int bad = clear_test(1) + clear_test(60);
  // A small step back (clock correction / re-set, < CLEAR_MIN_BACKSTEP_DEG) must just redraw
  // the band shorter, with no clearing sweep.
  face::show(13, 20, 30, 0);
  while (face::busy()) { sim_now_ms += 33; face::show(13, 20, 30, 0); }
  run();
  face::show(13, 20, 27, 400);  // 2.6 s earlier = 0.26 deg
  bool swept = face::busy();
  run();
  ring::BandState st;
  st.deg = (20 * 60 + 27.4f) * 0.1f;
  int back = check_ring(st);
  printf("small step back: %s, %d mismatches\n", swept ? "SWEEP STARTED (bad)" : "no sweep", back);
  bad += back + (swept ? 1 : 0);
  bad += setup_test();
  bad += orbit_test();
  printf("%s\n", bad ? "CONSISTENCY FAILURES" : "all consistency checks passed");
  return bad ? 2 : 0;
}
