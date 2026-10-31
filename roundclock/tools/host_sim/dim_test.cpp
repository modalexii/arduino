// dim_test.cpp: checks dimmer.h's sun times, phases and fade maths on a PC.
//   g++ -std=c++17 -I../../src -I../../include dim_test.cpp -o dim_test && ./dim_test
#include <cstdio>
#include <cstdlib>
#include "clock_config.h"
#include "dimmer.h"

int main() {
  int bad = 0;
  // Sun times for Washington, DC (38.9 N, 77.0 W), minutes after UTC midnight, from the astral
  // library (sunset > 1440 = after UTC midnight). The simple formula must be within 4 min.
  struct { int yday, rise, set; } ref[] = {
    {0, 746, 1316},  // 2026-01-01
    {44, 720, 1364},  // 2026-02-14
    {78, 671, 1399},  // 2026-03-20
    {119, 611, 1439},  // 2026-04-30
    {171, 583, 1476},  // 2026-06-21
    {226, 621, 1442},  // 2026-08-15
    {264, 656, 1384},  // 2026-09-22
    {301, 692, 1330},  // 2026-10-29
    {318, 710, 1313},  // 2026-11-15
    {354, 743, 1309},  // 2026-12-21
  };
  int worst = 0;
  for (auto &r : ref) {
    int rise, set;
    dimmer::sun_times_utc(r.yday, 38.9f, -77.0f, rise, set);
    int e = abs(rise - r.rise) > abs(set - r.set) ? abs(rise - r.rise) : abs(set - r.set);
    if (e > worst) worst = e;
    if (e > 4) { printf("yday %d: rise %d (want %d), set %d (want %d)\n", r.yday, rise, r.rise, set, r.set); bad++; }
  }
  printf("sun times: worst error %d min\n", worst);

  // Phases with sunrise 7:30, sunset 18:10 (local), +60 / +120 min, day from sunrise.
  struct { int t; dimmer::Phase want; } ph[] = {
      {0, dimmer::NIGHT}, {7 * 60 + 29, dimmer::NIGHT}, {7 * 60 + 30, dimmer::DAY},
      {19 * 60 + 9, dimmer::DAY}, {19 * 60 + 10, dimmer::EVENING}, {20 * 60 + 9, dimmer::EVENING},
      {20 * 60 + 10, dimmer::NIGHT}, {23 * 60 + 59, dimmer::NIGHT}};
  for (auto &p : ph)
    if (dimmer::phase_at(p.t, 450, 1090, 0, 60, 120) != p.want) {
      printf("phase_at(%d) wrong\n", p.t);
      bad++;
    }
  // Latest DC night start (late June, EDT): must not cross midnight.
  int latest = 0;
  for (int yd = 0; yd < 366; yd++) {
    int rise, set;
    dimmer::sun_times_utc(yd, cfg::SUN_LAT, cfg::SUN_LON, rise, set);
    int night = dimmer::wrap_day(set - 240) + cfg::NIGHT_AFTER_SUNSET_MIN;  // EDT, worst case
    if (night > latest) latest = night;
  }
  printf("latest night start: %d:%02d local\n", latest / 60, latest % 60);
  if (latest >= 1440) { puts("night start crosses midnight"); bad++; }

  // Fades: exact endpoints, monotonic both ways, no overshoot.
  const uint8_t D = cfg::BRIGHTNESS_DAY, N = cfg::BRIGHTNESS_NIGHT;
  if (dimmer::fade_level(D, N, 0) != D || dimmer::fade_level(D, N, 1) != N) { puts("endpoints wrong"); bad++; }
  if (dimmer::fade_level(0, 255, 1) != 255 || dimmer::fade_level(255, 0, 1) != 0) { puts("0/255 endpoints wrong"); bad++; }
  int prev_down = 256, prev_up = -1;
  for (int i = 0; i <= 100; i++) {
    float t = i / 100.0f;
    int down = dimmer::fade_level(D, N, t), up = dimmer::fade_level(N, D, t);
    if (down > prev_down || up < prev_up) { printf("not monotonic at t=%.2f\n", t); bad++; }
    if (down < N || down > D || up < N || up > D) { printf("out of range at t=%.2f\n", t); bad++; }
    prev_down = down, prev_up = up;
  }
  printf("%s\n", bad ? "DIM TESTS FAILED" : "all dim tests passed");
  return bad ? 1 : 0;
}
