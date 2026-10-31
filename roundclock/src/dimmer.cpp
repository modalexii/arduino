// dimmer.cpp: see dimmer.h.
#include "dimmer.h"
#include <Arduino.h>
#include "clock_config.h"
#include "display.h"

namespace dimmer {
namespace {
constexpr uint32_t STEP_MS = 20;  // fade step interval (~50 steps a second at most)

uint8_t level_ = 0;               // what the panel is set to
uint8_t from_ = 0, to_ = 0;       // current fade
uint32_t fade_t0 = 0, last_step = 0;
bool fading = false;
bool override_ = false;
int phase_ = -1;                  // last scheduled Phase; -1 = none yet

void apply(uint8_t v) {
  if (v == level_) return;
  level_ = v;
  display_set_brightness(v);
}

void fade_to(uint8_t target) {
  if (target == level_ && !fading) return;
  from_ = level_;
  to_ = target;
  fade_t0 = millis();
  last_step = 0;
  fading = cfg::DIM_FADE_MS > 0;
  if (!fading) apply(target);
}

const char *const NAMES[] = {"day", "evening", "night"};
uint8_t scheduled() {
  return phase_ == NIGHT ? cfg::BRIGHTNESS_NIGHT : phase_ == EVENING ? cfg::BRIGHTNESS_EVENING : cfg::BRIGHTNESS_DAY;
}
}  // namespace

void begin(uint8_t level) {
  level_ = level;
  display_set_brightness(level);
  fading = override_ = false;
}

void schedule(int phase) {
  int n = phase >= DAY && phase <= NIGHT ? phase : DAY;
  if (n == phase_) return;
  bool first = phase_ < 0;
  phase_ = n;
  if (override_) {
    override_ = false;
    Serial.println("brightness: back to the schedule");
  }
  if (first) {
    fading = false;
    apply(scheduled());  // boot: no fade
  } else {
    if (cfg::DEBUG_LOG) Serial.printf("brightness: %s, fading to %u\n", NAMES[n], scheduled());
    fade_to(scheduled());
  }
}

void manual(uint8_t level) {
  override_ = true;
  fading = false;
  apply(level);
}

void automatic() {
  override_ = false;
  fade_to(scheduled());
}

void loop() {
  if (!fading) return;
  uint32_t now = millis();
  if (now - last_step < STEP_MS && last_step) return;
  last_step = now;
  uint32_t el = now - fade_t0;
  float t = el >= cfg::DIM_FADE_MS ? 1.0f : (float)el / cfg::DIM_FADE_MS;
  apply(fade_level(from_, to_, t));
  if (t >= 1.0f) fading = false;
}

void print_status() {
  Serial.printf("brightness: %u (%s", level_,
                override_ ? "manual until the next scheduled change; B = back to schedule"
                          : phase_ >= 0 ? NAMES[phase_] : "not scheduled yet");
  if (fading) Serial.printf(", fading to %u", to_);
  Serial.println(")");
}

}  // namespace dimmer
