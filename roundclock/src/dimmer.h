// dimmer.h: brightness that follows the sun (CLOCK_PLAN.md Phase 6). Three levels: DAY from
// sunrise, EVENING from sunset + cfg::EVENING_AFTER_SUNSET_MIN, NIGHT from sunset +
// cfg::NIGHT_AFTER_SUNSET_MIN until the next sunrise. Changes fade over cfg::DIM_FADE_MS.
// A manual level (serial "B n") holds until the next scheduled change.
#pragma once
#include <math.h>
#include <stdint.h>

namespace dimmer {

enum Phase { DAY = 0, EVENING = 1, NIGHT = 2 };

// ---- Pure logic (also tested on a PC: tools/host_sim/dim_test.cpp) ----

// Sunrise and sunset for a day of the year (tm_yday, 0-365) at lat/lon (degrees, east +), in
// minutes after UTC midnight of that date. They can fall outside 0-1439 (a US summer sunset is
// ~1477 = 00:37 UTC the next day); callers wrap. Simple declination + equation-of-time formula,
// standard -0.833 deg horizon: within ~3 min of exact at mid latitudes. Polar day/night clamp.
inline void sun_times_utc(int yday, float lat, float lon, int &rise, int &set) {
  const float D = (float)M_PI / 180.0f, n = yday + 1.0f;
  float decl = 23.44f * sinf(2.0f * (float)M_PI * (284.0f + n) / 365.0f);
  float b = 2.0f * (float)M_PI * (n - 81.0f) / 364.0f;
  float eot = 9.87f * sinf(2 * b) - 7.53f * cosf(b) - 1.5f * sinf(b);  // equation of time, min
  float c = (sinf(-0.833f * D) - sinf(lat * D) * sinf(decl * D)) / (cosf(lat * D) * cosf(decl * D));
  c = c < -1 ? -1 : c > 1 ? 1 : c;
  float half = 4.0f * acosf(c) / D;  // half the day length, minutes
  float noon = 720.0f - 4.0f * lon - eot;
  rise = (int)lroundf(noon - half);
  set = (int)lroundf(noon + half);
}

inline int wrap_day(int m) { return ((m % 1440) + 1440) % 1440; }

// Phase at local minute-of-day t, given local sunrise/sunset (0-1439). Assumes the switch times
// don't cross midnight (true in DC: night starts by ~22:40 at the latest).
inline Phase phase_at(int t, int rise, int set, int day_after_rise, int evening_after_set,
                      int night_after_set) {
  if (t < rise + day_after_rise) return NIGHT;
  if (t < set + evening_after_set) return DAY;
  if (t < set + night_after_set) return EVENING;
  return NIGHT;
}

// Brightness t (0-1) of the way through a fade from a to b. Interpolates in a gamma-2 space
// (square roots), so the dim end of the fade doesn't rush past: perceived brightness changes
// at a steadier rate than with a straight line.
inline uint8_t fade_level(uint8_t a, uint8_t b, float t) {
  if (t <= 0) return a;
  if (t >= 1) return b;
  float ra = sqrtf((float)a), rb = sqrtf((float)b), r = ra + (rb - ra) * t;
  int v = (int)lroundf(r * r);
  return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
}

// ---- On the board ----

// Sets the level immediately (no fade) and starts automatic mode.
void begin(uint8_t level);
// Call once a second or so with the current phase, or -1 if the time isn't known (then DAY).
// Starts a fade when the phase changes, and ends a manual override at the change.
void schedule(int phase);
// Manual level (serial "B n"): applied at once, held until the next scheduled switch.
void manual(uint8_t level);
// Back to the scheduled level (serial "B" with no number), with a fade.
void automatic();
// Steps a running fade. Call every loop; cheap when idle.
void loop();
void print_status();

}  // namespace dimmer
