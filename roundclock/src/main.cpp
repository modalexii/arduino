// main.cpp: setup(), loop() and the CLOCK / SETUP mode state machine (CLOCK_PLAN.md §5.8).
//
// The face shows real local time (RTC holds UTC; TZ from NVS; see timekeeping.h). If the RTC
// has no valid time at boot (after an unplug), the clock goes straight into set-time mode: the
// "Clock" hotspot + captive portal (timeset_portal.h) and the setup screen. That's the only
// way in besides serial W (Kyle's choice: no touch/button trigger; Phase 5 skipped).
// Brightness follows the sun (day / evening / night) with a fade (dimmer.h); optional pixel orbit.
// Serial commands (115200):
//   W                         start set-time mode (hotspot); W again cancels it
//   T <unix_utc> [posix_tz]   set the time (seconds or ms since 1970, UTC), and optionally the TZ
//   Z [posix_tz]              set (or show) the time zone, e.g. Z EST5EDT,M3.2.0,M11.1.0
//   R                         back to real time (after F / S)
//   F h m [s]   freeze a fake clock at h:m:s (24 h)  F        resume the fake clock
//   S n         fake clock at n x speed              B n      brightness 0-255 (until the next
//                                                     B        scheduled change); B = schedule
//   O dx dy     move the face by dx, dy px (preview the orbit / check the glass edge); O = 0 0
//   ?           status (time, RTC, TZ, numeral fit, render times, free memory)
// With CALIBRATE set, runs the Phase 1 bring-up/calibration screen (calibrate.cpp) instead.
#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>
#include "board.h"
#include "clock_config.h"
#include "display.h"
#include "calibrate.h"
#include "dimmer.h"
#include "face.h"
#include "timekeeping.h"
#include "timeset_portal.h"

#define CALIBRATE 0  // 1 = Phase 1 bring-up/calibration screen instead of the clock

namespace {

bool display_ok = false;

// ---- Mode state machine (CLOCK_PLAN.md §5.8) ----
// CLOCK: the face. SETUP: hotspot up, setup screen breathing. DONE: check mark after a set,
// then back to CLOCK. SETUP_IDLE: hotspot timed out with no valid time: still setup screen,
// Wi-Fi off. Only reached if Wi-Fi failed, or with cfg::AP_STAY_UP_UNTIL_SET off; the way out
// is unplugging (or RESET), which reboots into setup again; serial W also works.
enum class Mode { CLOCK, SETUP, DONE, SETUP_IDLE } mode = Mode::CLOCK;
uint32_t done_at = 0;
const char *mode_name() {
  switch (mode) {
    case Mode::CLOCK: return "CLOCK";
    case Mode::SETUP: return "SETUP (hotspot on)";
    case Mode::DONE: return "DONE (check mark)";
    default: return "SETUP_IDLE (hotspot off, time not set)";
  }
}

// ---- Time sources ----
// REAL: tk (RTC + TZ). FAKE: a dev clock for previewing the face (F / S commands, or
// cfg::FAKE_TIME_SPEED > 0 at boot); "R" goes back to REAL.
enum class Src { REAL, FAKE } src = Src::REAL;

constexpr uint64_t DAY_MS = 24ULL * 3600 * 1000;
uint64_t fake_ms = 0;  // ms since local midnight
int speed = 1;
bool frozen = false;
uint32_t last_ms = 0;
uint32_t last_face = 0;
int64_t last_bucket = -1;      // REAL: utc_ms / SWEEP_UPDATE_MS of the last face update
uint32_t last_nag = 0;         // REAL, time not set: last serial reminder
constexpr uint32_t NAG_MS = 60UL * 1000;
String line;

void fake_advance() {
  uint32_t now = millis();
  if (!frozen) fake_ms = (fake_ms + (uint64_t)(now - last_ms) * speed) % DAY_MS;
  last_ms = now;
}

void fake_set(int h, int m, int s) { fake_ms = ((uint64_t)h * 3600 + m * 60 + s) * 1000; }

void use_fake() {
  if (src == Src::FAKE) return;
  // Start the fake clock where the face is now, so switching doesn't jump.
  struct tm t;
  int ms;
  if (tk::local_now(t, ms)) fake_ms = ((uint64_t)t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec) * 1000 + ms;
  else fake_set(12, 0, 0);
  last_ms = millis();
  src = Src::FAKE;
  Serial.println("(fake clock; R = back to real time)");
}

void print_time() {
  if (src == Src::REAL) {
    Serial.println("source: REAL time");
    return;
  }
  uint32_t t = fake_ms / 1000;
  Serial.printf("source: FAKE clock %02u:%02u:%02u.%03u, %s, speed %dx (R = real time)\n",
                (unsigned)(t / 3600), (unsigned)(t / 60 % 60), (unsigned)(t % 60),
                (unsigned)(fake_ms % 1000), frozen ? "FROZEN" : "running", speed);
}

// The local time to show: real, fake, or 12:00:00 while the real time isn't known.
void current_time(int &h, int &m, int &s, int &ms) {
  if (src == Src::FAKE) {
    uint32_t t = fake_ms / 1000;
    h = t / 3600, m = t / 60 % 60, s = t % 60, ms = fake_ms % 1000;
    return;
  }
  struct tm t;
  if (tk::local_now(t, ms)) {
    h = t.tm_hour, m = t.tm_min, s = t.tm_sec;
  } else {
    h = 12, m = 0, s = 0, ms = 0;
  }
}

// ---- Brightness schedule: follows the sun (dimmer.h) ----
// Today's sunrise/sunset in local minutes, from the real date (also on the fake clock, whose
// time of day is used instead of the real one). False while the time isn't known.
bool sun_today(int &rise, int &set, int &now) {
  struct tm t;
  int ms;
  if (!tk::local_now(t, ms)) return false;
  int local_min = t.tm_hour * 60 + t.tm_min;
  int utc_min = (int)((tk::utc_ms() / 60000) % 1440);
  int off = dimmer::wrap_day(local_min - utc_min + 720) - 720;  // e.g. -240 in EDT
  dimmer::sun_times_utc(t.tm_yday, cfg::SUN_LAT, cfg::SUN_LON, rise, set);
  rise = dimmer::wrap_day(rise + off);
  set = dimmer::wrap_day(set + off);
  now = src == Src::FAKE ? (int)(fake_ms / 60000) : local_min;
  return true;
}

int sun_phase() {
  int rise, set, now;
  if (!sun_today(rise, set, now)) return -1;
  return dimmer::phase_at(now, rise, set, cfg::DAY_AFTER_SUNRISE_MIN, cfg::EVENING_AFTER_SUNSET_MIN,
                          cfg::NIGHT_AFTER_SUNSET_MIN);
}

void print_sun() {
  int rise, set, now;
  if (!sun_today(rise, set, now)) {
    Serial.println("  sun: time not set (day brightness)");
    return;
  }
  auto hm = [](int m) { return String(m / 60) + ":" + (m % 60 < 10 ? "0" : "") + String(m % 60); };
  int e = set + cfg::EVENING_AFTER_SUNSET_MIN, n = set + cfg::NIGHT_AFTER_SUNSET_MIN;
  int d = rise + cfg::DAY_AFTER_SUNRISE_MIN;
  Serial.printf("  sun today: rise %s, set %s -> %u at %s, %u at %s, %u at %s\n", hm(rise).c_str(),
                hm(set).c_str(), cfg::BRIGHTNESS_EVENING, hm(e).c_str(), cfg::BRIGHTNESS_NIGHT,
                hm(n).c_str(), cfg::BRIGHTNESS_DAY, hm(d).c_str());
}

// ---- Pixel orbit (cfg::ORBIT_PX) ----
uint32_t orbit_k = UINT32_MAX;
void orbit_step() {
  if (cfg::ORBIT_PX <= 0) return;
  uint32_t k = millis() / (cfg::ORBIT_PERIOD_MIN * 60000UL) % 8;
  if (k == orbit_k) return;
  orbit_k = k;
  float a = k * (float)M_PI / 4;
  face::set_offset((int)lroundf(cfg::ORBIT_PX * cosf(a)), (int)lroundf(cfg::ORBIT_PX * sinf(a)));
}

// Once a second: the brightness schedule. Hourly (DEBUG_LOG): free memory, for soak tests.
uint32_t last_sched = 0, last_health = 0;
void housekeeping() {
  uint32_t now = millis();
  if (now - last_sched >= 1000) {
    last_sched = now;
    dimmer::schedule(sun_phase());
  }
  if (cfg::DEBUG_LOG && now - last_health >= 3600UL * 1000) {
    last_health = now;
    Serial.printf("health: up %u h, free internal %u KB (min %u KB), PSRAM %u KB\n",
                  (unsigned)(now / 3600000), (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                  (unsigned)(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) / 1024),
                  (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
  }
}

void update_face() {
  if (face::in_setup()) return;
  int h, m, s, ms;
  current_time(h, m, s, ms);
  face::show(h, m, s, ms);
  last_face = millis();
  last_bucket = tk::utc_ms() / cfg::SWEEP_UPDATE_MS;
}

// FAKE: refresh every SWEEP_UPDATE_MS at 1x; ~30 fps when fast-forwarding so the sweep still
// looks continuous. REAL: on each SWEEP_UPDATE_MS boundary of the wall clock, so the hour
// changes right at :00.000.
bool face_due() {
  if (src == Src::REAL) return tk::utc_ms() / cfg::SWEEP_UPDATE_MS != last_bucket;
  uint32_t ms = cfg::SWEEP_UPDATE_MS;
  if (!frozen && speed > 1) {
    ms = cfg::SWEEP_UPDATE_MS / speed;
    if (ms < 33) ms = 33;
  }
  return millis() - last_face >= ms;
}

// ---- Animation pacing (hour-clearing sweep) ----
// Each frame renders the face for "now" and pushes it to the panel at once (lv_refr_now), so
// what's shown always matches the time it was computed for, at a steady ANIM_FRAME_MS.
// (Leaving it to lv_timer_handler's own 33 ms refresh timer made frames uneven: the two
// timers drift against each other, so some frames carried one step and some two.)
uint32_t anim_start = 0, anim_frames = 0, anim_max_ms = 0, anim_first_ms = 0;
bool anim_running = false;

void animate_frame() {
  uint32_t t0 = millis();
  update_face();
  lv_refr_now(NULL);
  uint32_t dt = millis() - t0;
  if (anim_frames == 0) anim_first_ms = dt;  // includes the new hour numeral's glyphs
  else if (dt > anim_max_ms) anim_max_ms = dt;
  anim_frames++;
}

void animate() {
  if (millis() - last_face < cfg::ANIM_FRAME_MS) {
    delay(1);
    return;
  }
  animate_frame();
  if (!face::busy() && cfg::DEBUG_LOG) {
    uint32_t total = millis() - anim_start;
    Serial.printf("hour clear: %u frames in %u ms (%.0f fps); first frame %u ms, then max %u ms\n",
                  (unsigned)anim_frames, (unsigned)total, anim_frames * 1000.0f / (total ? total : 1),
                  (unsigned)anim_first_ms, (unsigned)anim_max_ms);
  }
}

void leave_setup() {
  face::setup_exit();
  mode = Mode::CLOCK;
  Serial.println("mode: CLOCK");
  update_face();
}

void enter_setup() {
  static String msg = String("Join Wi-Fi\n") + cfg::AP_SSID;
  if (!face::setup_enter(msg.c_str(), true)) return;
  if (!portal::start()) {
    face::setup_message("Wi-Fi failed\nReplug to retry", false);
    mode = Mode::SETUP_IDLE;
    return;
  }
  mode = Mode::SETUP;
  Serial.printf("mode: SETUP. On the phone, join Wi-Fi \"%s\"; the set-time page should open by "
                "itself (else browse to http://192.168.4.1). W = cancel.\n", cfg::AP_SSID);
}

void cancel_setup() {
  portal::stop();
  if (tk::valid()) {
    leave_setup();
  } else {
    face::setup_message("Time not set\nReplug to retry", false);
    mode = Mode::SETUP_IDLE;
    Serial.println("mode: SETUP_IDLE (time not set; replug, RESET or W to start the hotspot again)");
  }
}

void poll_portal() {
  if (!portal::active()) return;
  portal::Event e = portal::loop();
  if (e == portal::Event::TIME_SET) {
    if (src == Src::FAKE) {
      src = Src::REAL;
      Serial.println("(fake clock off: real time)");
    }
    if (mode == Mode::SETUP || mode == Mode::SETUP_IDLE) {
      face::setup_done();
      mode = Mode::DONE;
      done_at = millis();
    }
  } else if (e == portal::Event::TIMEOUT && mode == Mode::SETUP) {
    cancel_setup();
  }
}

void handle(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  char c = toupper(cmd[0]);
  String arg = cmd.substring(1);
  arg.trim();
  int a = 0, b = 0, d = 0;
  int n = sscanf(arg.c_str(), "%d %d %d", &a, &b, &d);
  if (c == 'T') {
    char *end = nullptr;
    long long v = strtoll(arg.c_str(), &end, 10);
    String tz = end ? String(end) : String();
    tz.trim();
    if (end == arg.c_str() || v <= 0) {
      Serial.println("usage: T <unix_utc_seconds or ms> [posix_tz]");
      Serial.println("  PowerShell: [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()");
      return;
    }
    int64_t ms = v > 100000000000LL ? (int64_t)v : (int64_t)v * 1000;  // > 1e11 = already ms
    if (tz.length() && !tk::set_tz(tz.c_str())) return;
    if (!tk::set_utc_ms(ms)) return;
    if (src == Src::FAKE) Serial.println("(still on the fake clock; R = real time)");
    if (mode != Mode::CLOCK) {
      portal::stop();
      leave_setup();
    }
  } else if (c == 'W') {
    if (mode == Mode::SETUP) cancel_setup();
    else if (mode != Mode::DONE) enter_setup();
  } else if (c == 'Z') {
    if (arg.length() == 0) Serial.printf("TZ %s\n", tk::tz());
    else tk::set_tz(arg.c_str());
  } else if (c == 'R') {
    src = Src::REAL;
    frozen = false;
    print_time();
    if (!tk::valid()) Serial.println("time not set yet: showing 12:00. Use T <unix_utc>.");
  } else if (c == 'F') {
    use_fake();
    if (arg.length() == 0) {
      frozen = false;
    } else if (n >= 2 && a >= 0 && a <= 23 && b >= 0 && b <= 59 && (n < 3 || (d >= 0 && d <= 59))) {
      fake_set(a, b, n == 3 ? d : 0);
      frozen = true;
    } else {
      Serial.println("usage: F h m [s]   (freeze)   or   F   (resume)");
      return;
    }
    print_time();
  } else if (c == 'S') {
    if (n != 1 || a < 1 || a > 3600) {
      Serial.println("usage: S n   (1-3600; 1 = real time, 60 = an hour per minute)");
      return;
    }
    use_fake();
    speed = a;
    frozen = false;
    print_time();
  } else if (c == 'B') {
    if (arg.length() == 0) {
      dimmer::automatic();
    } else if (n != 1 || a < 0 || a > 255) {
      Serial.println("usage: B n   (0-255; held until the next scheduled change)   or   B   (schedule)");
      return;
    } else {
      dimmer::manual(a);
    }
    dimmer::print_status();
  } else if (c == 'O') {
    if (arg.length() && (n != 2 || abs(a) > 20 || abs(b) > 20)) {
      Serial.println("usage: O dx dy   (px, -20..20; + = right/down)   or   O   (back to 0 0)");
      return;
    }
    if (arg.length() == 0) a = b = 0;
    face::set_offset(a, b);
    Serial.printf("face offset (%d,%d)%s\n", a, b,
                  cfg::ORBIT_PX > 0 ? "; the orbit moves it again at its next step" : "");
  } else if (c == '?') {
    Serial.printf("mode: %s\n", mode_name());
    print_time();
    tk::print_status();
    dimmer::print_status();
    print_sun();
    face::print_status();
    portal::print_status();
  } else {
    Serial.println("commands: W | T <unix_utc> [tz] | Z [tz] | R | F h m [s] | F | S n | B [n] | O [dx dy] | ?");
    return;
  }
  update_face();
}

void poll_serial() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      handle(line);
      line = "";
    } else if (line.length() < 96) {
      line += ch;
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);  // give the USB serial monitor time to attach
  delay(500);
  Serial.println("\n=== Minimal clock ===");
  {
    const char *why[] = {"unknown", "power-on", "ext", "software", "PANIC", "int watchdog",
                         "task watchdog", "watchdog", "deep sleep", "BROWNOUT", "SDIO", "USB", "JTAG"};
    int r = (int)esp_reset_reason();
    Serial.printf("Reset reason: %s (%d)\n", r < 13 ? why[r] : "?", r);
  }
  Serial.printf("Flash: %u MB, PSRAM: %u KB\n", ESP.getFlashChipSize() / (1024 * 1024), ESP.getPsramSize() / 1024);

#if CALIBRATE
  calibrate_setup();
  return;
#endif

  display_ok = display_init();
  if (!display_ok || !face::begin()) {
    display_ok = false;
    Serial.println("Display/face init failed; halting.");
    return;
  }
  dimmer::begin(cfg::BRIGHTNESS_DAY);  // the screen is still black; the schedule sets night below

  Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
  tk::begin();
  if (cfg::FAKE_TIME_SPEED > 0) {
    src = Src::FAKE;
    speed = cfg::FAKE_TIME_SPEED;
    fake_set(cfg::FAKE_START_HOUR, cfg::FAKE_START_MIN, 0);
    last_ms = millis();
  }
  last_nag = millis();
  dimmer::schedule(sun_phase());  // first call: no fade
  orbit_step();
  update_face();

  uint32_t r0 = millis();
  lv_timer_handler();  // first frame: includes rasterising the numeral glyphs
  if (cfg::DEBUG_LOG) Serial.printf("Face up. First frame took %u ms.\n", (unsigned)(millis() - r0));
  Serial.println("commands: W (hotspot) | T <unix_utc> [tz] | Z [tz] | R (real) | F h m [s] / F / S n (fake) | B [n] | O [dx dy] | ?");
  if (cfg::DEBUG_LOG) {
    print_time();
    face::print_status();
  }
  if (!tk::valid() && src == Src::REAL) enter_setup();
}

void loop() {
#if CALIBRATE
  calibrate_loop();
  return;
#endif
  if (!display_ok) { delay(1000); return; }
  dimmer::loop();  // steps a brightness fade (also between animation frames)
  housekeeping();
  poll_serial();
  poll_portal();
  if (mode != Mode::CLOCK) {
    if (mode == Mode::DONE && millis() - done_at >= cfg::SETUP_DONE_MS) {
      leave_setup();  // the hotspot lingers ~1 s more for the phone's reply; poll_portal ends it
      return;
    }
    face::setup_loop();
    tk::loop();
    lv_timer_handler();
    delay(portal::active() ? 1 : 5);
    return;
  }
  fake_advance();
  if (!face::busy() && face_due()) update_face();
  if (face::busy()) {  // an update (or a serial command) started an animation
    if (!anim_running) {
      anim_running = true;
      anim_frames = anim_max_ms = 0;
      anim_start = millis();
    }
    animate();
    return;
  }
  anim_running = false;
  orbit_step();
  tk::loop();  // hourly RTC re-discipline (kept out of the animation frames)
  if (src == Src::REAL && !tk::valid() && millis() - last_nag >= NAG_MS) {
    last_nag = millis();
    Serial.println("time not set: showing 12:00. W = hotspot, or T <unix_utc>  ([DateTimeOffset]::UtcNow.ToUnixTimeSeconds())");
  }
  lv_timer_handler();
  delay(portal::active() ? 1 : 5);
}
