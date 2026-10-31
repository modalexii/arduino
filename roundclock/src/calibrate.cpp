// calibrate.cpp: see calibrate.h.
//
// Screen: three 1 px rings, measured from the geometric centre (233.0, 233.0), which
// lies between pixels 232 and 233 on this even-sized panel:
//   red   = outermost pixel ring (touches the panel edge)
//   green = 2 px in from the edge
//   white = 6 px in from the edge
// plus a grey 2 px crosshair at the centre and the current settings as text.
// The "disk" view fills the whole 466 px circle in white instead: a black sliver on
// one side means the image is shifted toward the other side.
#include "calibrate.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "board.h"
#include "display.h"
#include "input.h"
#include "timekeeping.h"

namespace {

constexpr uint16_t COL_BLACK = 0x0000;
constexpr uint16_t COL_WHITE = 0xFFFF;
constexpr uint16_t COL_RED   = 0xF800;
constexpr uint16_t COL_GREEN = 0x07E0;
constexpr uint16_t COL_GREY  = 0x8410;
constexpr uint8_t  DEFAULT_BRIGHTNESS = 208;  // the CO5300 init value

Arduino_Canvas *canvas = nullptr;
bool view_disk = false;
uint8_t brightness = DEFAULT_BRIGHTNESS;
String line;

const char *device_name(uint8_t a) {
  switch (a) {
    case I2C_ADDR_TOUCH: return "FT3168 touch (expected)";
    case I2C_ADDR_RTC:   return "PCF85063 RTC (expected)";
    case I2C_ADDR_IMU_A:
    case I2C_ADDR_IMU_B: return "QMI8658 IMU (expected)";
    default:             return "unknown";
  }
}

void i2c_scan() {
  Serial.printf("I2C scan (SDA %d, SCL %d):\n", I2C_SDA, I2C_SCL);
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  0x%02X  %s\n", a, device_name(a));
      found++;
    }
  }
  if (!found) Serial.println("  nothing found");
}

void print_rtc() {
  RtcTime t;
  bool os;
  if (!rtc::read(t, os)) {
    Serial.printf("RTC: read failed (no reply at 0x%02X)\n", I2C_ADDR_RTC);
    return;
  }
  bool valid = !os && t.year >= 2025;
  Serial.printf("RTC: %04d-%02d-%02d %02d:%02d:%02d  weekday=%d  OS flag=%d  -> %s\n",
                t.year, t.month, t.day, t.hour, t.minute, t.second, t.weekday, os ? 1 : 0,
                valid ? "valid" : "INVALID (the clock would enter set-time mode)");
}

void write_rtc(const String &arg) {
  RtcTime t = {2026, 1, 1, 0, 0, 0, 0};
  if (arg.length() > 0 &&
      sscanf(arg.c_str(), "%d-%d-%d %d:%d:%d", &t.year, &t.month, &t.day, &t.hour, &t.minute, &t.second) != 6) {
    Serial.println("usage: w   or   w YYYY-MM-DD HH:MM:SS");
    return;
  }
  if (t.year < 2000 || t.year > 2099 || t.month < 1 || t.month > 12 || t.day < 1 || t.day > 31 ||
      t.hour < 0 || t.hour > 23 || t.minute < 0 || t.minute > 59 || t.second < 0 || t.second > 59) {
    Serial.println("w: value out of range");
    return;
  }
  Serial.printf("RTC: writing %04d-%02d-%02d %02d:%02d:%02d ... %s\n", t.year, t.month, t.day, t.hour,
                t.minute, t.second, rtc::write(t) ? "ok" : "FAILED");
  print_rtc();
}

void print_centered(const char *s, int y, uint16_t color) {
  canvas->setTextSize(2);  // built-in 6x8 font -> 12x16 px per char
  canvas->setTextColor(color);
  canvas->setCursor((LCD_W - (int)strlen(s) * 12) / 2, y);
  canvas->print(s);
}

void draw() {
  uint16_t *fb = canvas->getFramebuffer();
  const float c = LCD_W / 2.0f;  // 233.0
  for (int y = 0; y < LCD_H; y++) {
    float dy = y + 0.5f - c;
    for (int x = 0; x < LCD_W; x++) {
      float dx = x + 0.5f - c;
      float d = sqrtf(dx * dx + dy * dy);
      uint16_t col = COL_BLACK;
      if (view_disk) {
        if (d < c) col = COL_WHITE;
      } else if (d >= c - 1 && d < c) {
        col = COL_RED;
      } else if (d >= c - 3 && d < c - 2) {
        col = COL_GREEN;
      } else if (d >= c - 7 && d < c - 6) {
        col = COL_WHITE;
      }
      fb[y * LCD_W + x] = col;
    }
  }
  uint16_t fg = view_disk ? COL_BLACK : COL_GREY;
  // 2 px crosshair, symmetric about the centre (pixels 212..253 on both axes)
  for (int i = 212; i <= 253; i++) {
    fb[232 * LCD_W + i] = fb[233 * LCD_W + i] = fg;
    fb[i * LCD_W + 232] = fb[i * LCD_W + 233] = fg;
  }
  char buf[40];
  snprintf(buf, sizeof buf, "col %u  row %u", gfx->colOffset(), gfx->rowOffset());
  print_centered(buf, 280, fg);
  snprintf(buf, sizeof buf, "bri %u", brightness);
  print_centered(buf, 304, fg);
  canvas->flush();
}

// Blank the panel's whole 480x480 RAM, not just the 466x466 window at the current
// offsets, so a previous offset can't leave stale pixels in the margins.
void clear_panel_ram() {
  uint8_t col = gfx->colOffset(), row = gfx->rowOffset();
  for (uint8_t c : {0, 14})
    for (uint8_t r : {0, 14}) {
      gfx->setOffsets(c, r);
      gfx->fillScreen(COL_BLACK);
    }
  gfx->setOffsets(col, row);
}

void set_brightness(uint8_t b) {
  brightness = b;
  display_set_brightness(b);
  draw();
  Serial.printf("brightness %u\n", b);
}

void brightness_test() {
  Serial.println("Brightness test: 255, 128, 32 for 3 s each, then back to 208.");
  for (uint8_t b : {255, 128, 32}) {
    set_brightness(b);
    delay(3000);
  }
  set_brightness(DEFAULT_BRIGHTNESS);
}

void help() {
  Serial.println(
      "\nCommands (type, then Enter):\n"
      "  c <n>   column offset 0-14 (currently shown on screen), e.g. c 4\n"
      "  r <n>   row offset 0-14\n"
      "  v       switch view: rings <-> solid white disk\n"
      "  b <n>   brightness 0-255\n"
      "  bt      brightness test: 255, 128, 32 (3 s each)\n"
      "  s       I2C scan\n"
      "  t       read the RTC\n"
      "  w       write 2026-01-01 00:00:00 to the RTC (or: w YYYY-MM-DD HH:MM:SS)\n"
      "  ?       this help\n"
      "Touch events print automatically.\n");
}

bool parse_uint(const String &arg, int max, int &out) {
  if (arg.length() == 0) return false;
  for (char ch : arg) if (!isDigit(ch)) return false;
  out = arg.toInt();
  return out <= max;
}

void handle(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  String arg = cmd.substring(1);
  arg.trim();
  int n;
  if (cmd == "?") {
    help();
  } else if (cmd == "bt") {
    brightness_test();
  } else if (cmd[0] == 'c' || cmd[0] == 'r') {
    if (!parse_uint(arg, 14, n)) {
      Serial.println("usage: c <0-14>  or  r <0-14>");
      return;
    }
    if (cmd[0] == 'c') gfx->setOffsets(n, gfx->rowOffset());
    else gfx->setOffsets(gfx->colOffset(), n);
    clear_panel_ram();
    draw();
    Serial.printf("offsets: col %u, row %u\n", gfx->colOffset(), gfx->rowOffset());
  } else if (cmd == "v") {
    view_disk = !view_disk;
    draw();
    Serial.println(view_disk ? "view: solid disk" : "view: rings");
  } else if (cmd[0] == 'b') {
    if (!parse_uint(arg, 255, n)) {
      Serial.println("usage: b <0-255>");
      return;
    }
    set_brightness(n);
  } else if (cmd == "s") {
    i2c_scan();
  } else if (cmd == "t") {
    print_rtc();
  } else if (cmd[0] == 'w') {
    write_rtc(arg);
  } else {
    Serial.printf("unknown command '%s' (? for help)\n", cmd.c_str());
  }
}

void poll_touch() {
  static uint32_t last_poll = 0;
  static int last_count = 0;
  static uint32_t down_ms = 0;
  static uint32_t window_start = 0;
  static int errors = 0, reads = 0;
  if (millis() - last_poll < 50) return;
  last_poll = millis();

  // Once a minute, report how many polls failed (the FT3168 NAKs when it's asleep).
  if (millis() - window_start >= 60000) {
    if (window_start != 0)
      Serial.printf("touch: %d of %d polls failed in the last minute\n", errors, reads);
    window_start = millis();
    errors = reads = 0;
  }

  uint16_t x, y;
  int n = touch::read(x, y);
  reads++;
  if (n < 0) {  // keep the previous state, so one bad read doesn't end a press
    errors++;
    return;
  }

  if (n > 0 && last_count == 0) {
    down_ms = millis();
    Serial.printf("touch DOWN  points=%d  x=%u y=%u\n", n, x, y);
  } else if (n == 0 && last_count > 0) {
    Serial.printf("touch UP    held %lu ms\n", (unsigned long)(millis() - down_ms));
  } else if (n != last_count) {
    Serial.printf("touch points=%d\n", n);
  }
  last_count = n;
}

}  // namespace

void calibrate_setup() {
  Serial.println("--- Phase 1: hardware bring-up and calibration ---");
  Wire.begin(I2C_SDA, I2C_SCL, I2C_FREQ);
  i2c_scan();
  print_rtc();
  Serial.printf("touch: %s\n", touch::present() ? "FT3168 answers at 0x38" : "NO reply at 0x38");
  Serial.printf("touch: keep-active mode %s\n", touch::begin() ? "set" : "FAILED to set");

  if (!display_init(false)) {
    Serial.println("display init failed");
    return;
  }
  clear_panel_ram();
  size_t psram_before = ESP.getFreePsram();
  canvas = new Arduino_Canvas(LCD_W, LCD_H, gfx);
  if (!canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) {
    Serial.println("canvas framebuffer allocation failed");
    canvas = nullptr;
    return;
  }
  Serial.printf("framebuffer: %u KB taken from PSRAM (%u KB free)\n",
                (unsigned)((psram_before - ESP.getFreePsram()) / 1024), (unsigned)(ESP.getFreePsram() / 1024));
  set_brightness(DEFAULT_BRIGHTNESS);
  help();
}

void calibrate_loop() {
  if (!canvas) {
    delay(1000);
    return;
  }
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      handle(line);
      line = "";
    } else if (line.length() < 64) {
      line += ch;
    }
  }
  poll_touch();
  delay(5);
}
