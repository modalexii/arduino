// timekeeping.cpp: see timekeeping.h. Raw PCF85063 driver (rtc::) plus system time / TZ (tk::).
// PCF85063 registers: 0x00 Ctrl1, 0x01 Ctrl2, 0x02 Offset, 0x04 Seconds (bit 7 = OS),
// 0x05 Min, 0x06 Hour, 0x07 Day, 0x08 Weekday, 0x09 Month, 0x0A Year. All BCD.
#include "timekeeping.h"
#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include <sys/time.h>
#include "board.h"
#include "clock_config.h"

namespace {
constexpr uint8_t REG_CTRL1   = 0x00;
constexpr uint8_t REG_SECONDS = 0x04;
constexpr uint8_t CTRL1_STOP  = 1 << 5;
constexpr uint8_t CTRL1_12_24 = 1 << 1;  // 1 = 12 h mode; we want 0 (24 h)
constexpr uint8_t SECONDS_OS  = 1 << 7;

uint8_t bcd2bin(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
uint8_t bin2bcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

bool read_regs(uint8_t reg, uint8_t *buf, size_t n) {
  Wire.beginTransmission(I2C_ADDR_RTC);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)I2C_ADDR_RTC, (int)n) != (int)n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

bool write_regs(uint8_t reg, const uint8_t *buf, size_t n) {
  Wire.beginTransmission(I2C_ADDR_RTC);
  Wire.write(reg);
  Wire.write(buf, n);
  return Wire.endTransmission() == 0;
}
}  // namespace

namespace rtc {

bool present() {
  Wire.beginTransmission(I2C_ADDR_RTC);
  return Wire.endTransmission() == 0;
}

int weekday_of(int y, int m, int d) {  // Sakamoto's method
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

bool read(RtcTime &t, bool &osc_stopped) {
  uint8_t r[7];
  if (!read_regs(REG_SECONDS, r, sizeof r)) return false;
  osc_stopped = r[0] & SECONDS_OS;
  t.second  = bcd2bin(r[0] & 0x7F);
  t.minute  = bcd2bin(r[1] & 0x7F);
  t.hour    = bcd2bin(r[2] & 0x3F);
  t.day     = bcd2bin(r[3] & 0x3F);
  t.weekday = r[4] & 0x07;
  t.month   = bcd2bin(r[5] & 0x1F);
  t.year    = 2000 + bcd2bin(r[6]);
  return true;
}

bool write(const RtcTime &t) {
  uint8_t ctrl1;
  if (!read_regs(REG_CTRL1, &ctrl1, 1)) return false;
  // Stop the clock while writing (datasheet procedure), and force 24 h mode.
  uint8_t stopped = (ctrl1 | CTRL1_STOP) & ~CTRL1_12_24;
  if (!write_regs(REG_CTRL1, &stopped, 1)) return false;
  uint8_t r[7] = {
      bin2bcd(t.second),  // writing bit 7 = 0 clears the OS flag
      bin2bcd(t.minute),
      bin2bcd(t.hour),
      bin2bcd(t.day),
      (uint8_t)weekday_of(t.year, t.month, t.day),
      bin2bcd(t.month),
      bin2bcd(t.year % 100),
  };
  bool ok = write_regs(REG_SECONDS, r, sizeof r);
  uint8_t running = stopped & ~CTRL1_STOP;
  return write_regs(REG_CTRL1, &running, 1) && ok;
}

}  // namespace rtc

// ======================================================================================
// tk: system time, TZ, NVS, re-discipline (Phase 3)
// ======================================================================================
namespace {

constexpr int64_t MIN_VALID_UTC = 1735689600;  // 2025-01-01 00:00:00 UTC
constexpr int64_t MAX_VALID_UTC = 4102444800;  // 2100-01-01 00:00:00 UTC
// PCF85063 datasheet: after the STOP bit is released, the first seconds increment comes
// ~0.5 s later (only the upper prescaler stages are reset). So second S is written at system
// time S + 0.5 s and the RTC then ticks to S + 1 right on the system's whole second. The first
// re-discipline after a set measures the result (shown by "?").
constexpr int64_t  RTC_WRITE_PHASE_MS  = 500;
constexpr int64_t  SLEW_LIMIT_MS       = 2000;   // smaller offsets are slewed (adjtime), larger stepped
constexpr uint32_t REDISCIPLINE_MS     = 3600UL * 1000;  // once an hour
constexpr uint32_t FIRST_AFTER_SET_MS  = 15UL * 1000;    // check soon after a set, to show the offset
constexpr uint32_t RETRY_MS            = 60UL * 1000;    // after a failed RTC read
constexpr uint32_t EDGE_POLL_MS        = 2;      // RTC poll interval while waiting for a seconds edge
constexpr uint32_t EDGE_TIMEOUT_MS     = 1500;   // no edge in this long = RTC not ticking
constexpr uint32_t EDGE_MAX_GAP_MS     = 25;     // a longer gap between polls (loop was busy, e.g.
                                                 // animating) is too coarse: start over
constexpr const char *NVS_NS = "clock";

bool valid_ = false;
const char *invalid_reason = "not started";
char tz_buf[64] = "";
bool tz_from_nvs = false;

// Re-discipline state
enum class Disc { IDLE, WAIT_EDGE } disc = Disc::IDLE;
uint32_t disc_from_ms = 0, disc_interval_ms = REDISCIPLINE_MS;
uint32_t edge_start_ms = 0, last_poll_ms = 0;
int edge_first_sec = -1;
// Results of the last one
bool disc_done = false, disc_stepped = false;
int64_t disc_offset_ms = 0;      // RTC minus system (positive = system was behind)
uint32_t disc_at_ms = 0, disc_poll_gap_ms = 0;
uint32_t disc_count = 0, disc_failures = 0;

// Days since 1970-01-01 for a proleptic Gregorian date (H. Hinnant's algorithm).
int64_t days_from_civil(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int64_t)doe - 719468;
}

int64_t rtc_to_epoch(const RtcTime &t) {
  return days_from_civil(t.year, t.month, t.day) * 86400 + t.hour * 3600 + t.minute * 60 + t.second;
}

RtcTime epoch_to_rtc(int64_t s) {
  time_t tt = (time_t)s;
  struct tm g;
  gmtime_r(&tt, &g);
  return RtcTime{g.tm_year + 1900, g.tm_mon + 1, g.tm_mday, g.tm_hour, g.tm_min, g.tm_sec, g.tm_wday};
}

void set_system_ms(int64_t ms) {
  struct timeval tv;
  tv.tv_sec = (time_t)(ms / 1000);
  tv.tv_usec = (suseconds_t)((ms % 1000) * 1000);
  settimeofday(&tv, nullptr);
}

bool tz_ok(const char *s) {
  size_t n = strlen(s);
  if (n < 3 || n >= sizeof tz_buf) return false;
  if (!isalpha((unsigned char)s[0]) && s[0] != '<') return false;
  for (size_t i = 0; i < n; i++)
    if (!isprint((unsigned char)s[i]) || s[i] == ' ') return false;
  return true;
}

void apply_tz(const char *s) {
  strlcpy(tz_buf, s, sizeof tz_buf);
  setenv("TZ", tz_buf, 1);
  tzset();
}

// "2026-10-28 20:15:03 EDT" for a UTC second, in local time (or UTC).
void fmt_time(char *buf, size_t n, int64_t utc_s, bool local) {
  time_t tt = (time_t)utc_s;
  struct tm t;
  if (local) localtime_r(&tt, &t);
  else gmtime_r(&tt, &t);
  strftime(buf, n, local ? "%Y-%m-%d %H:%M:%S %Z" : "%Y-%m-%d %H:%M:%S UTC", &t);
}

void schedule_disc(uint32_t in_ms) {
  disc = Disc::IDLE;
  disc_from_ms = millis();
  disc_interval_ms = in_ms;
}

void print_rtc_hint() {
  Serial.println("  Set it over serial:  T <unix_utc_seconds>   e.g. from PowerShell:");
  Serial.println("    [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()");
}

}  // namespace

namespace tk {

bool begin() {
  // Time zone: NVS, else the default.
  Preferences prefs;
  String saved;
  if (prefs.begin(NVS_NS, false)) {
    saved = prefs.getString("tz", "");
    prefs.end();
  }
  tz_from_nvs = saved.length() && tz_ok(saved.c_str());
  apply_tz(tz_from_nvs ? saved.c_str() : cfg::DEFAULT_TZ);

  RtcTime t;
  bool os = false;
  if (!rtc::read(t, os)) {
    valid_ = false;
    invalid_reason = "RTC not responding";
  } else if (os) {
    valid_ = false;
    invalid_reason = "RTC oscillator-stop flag set (power was lost)";
  } else if (t.year < 2025 || rtc_to_epoch(t) < MIN_VALID_UTC) {
    valid_ = false;
    invalid_reason = "RTC year < 2025";
  } else {
    // Catch the next seconds edge so the system clock starts in phase with the RTC.
    uint32_t t0 = millis();
    RtcTime e = t;
    bool o2;
    while (millis() - t0 < EDGE_TIMEOUT_MS) {
      delay(EDGE_POLL_MS);
      if (rtc::read(e, o2) && e.second != t.second) break;
    }
    bool edge = e.second != t.second;
    set_system_ms(rtc_to_epoch(edge ? e : t) * 1000 + (edge ? EDGE_POLL_MS / 2 : 0));
    valid_ = true;
    invalid_reason = "";
    if (!edge) Serial.println("tk: WARNING: RTC seconds didn't tick within 1.5 s; set from it anyway");
  }

  char buf[40];
  Serial.printf("tk: TZ %s (%s)\n", tz_buf, tz_from_nvs ? "from NVS" : "default");
  if (valid_) {
    fmt_time(buf, sizeof buf, utc_ms() / 1000, true);
    Serial.printf("tk: time from RTC: %s\n", buf);
    schedule_disc(REDISCIPLINE_MS);
  } else {
    Serial.printf("tk: TIME NOT SET: %s. Showing 12:00 until it is.\n", invalid_reason);
    print_rtc_hint();
  }
  return valid_;
}

bool valid() { return valid_; }

int64_t utc_ms() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

bool local_now(struct tm &tm, int &ms) {
  if (!valid_) return false;
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  time_t tt = tv.tv_sec;
  localtime_r(&tt, &tm);
  ms = tv.tv_usec / 1000;
  return true;
}

void loop() {
  if (!valid_) return;
  uint32_t now = millis();
  if (disc == Disc::IDLE) {
    if (now - disc_from_ms < disc_interval_ms) return;
    RtcTime t;
    bool os;
    if (!rtc::read(t, os)) {
      disc_failures++;
      schedule_disc(RETRY_MS);
      return;
    }
    if (os) {
      Serial.println("tk: WARNING: RTC oscillator-stop flag is set; not re-disciplining from it");
      disc_failures++;
      schedule_disc(REDISCIPLINE_MS);
      return;
    }
    edge_first_sec = t.second;
    edge_start_ms = last_poll_ms = now;
    disc = Disc::WAIT_EDGE;
    return;
  }
  // WAIT_EDGE: poll the RTC until its seconds change, then compare with the system clock.
  if (now - last_poll_ms < EDGE_POLL_MS) return;
  uint32_t gap = now - last_poll_ms;
  last_poll_ms = now;
  RtcTime t;
  bool os;
  bool ok = rtc::read(t, os);
  if (ok && gap > EDGE_MAX_GAP_MS) {
    edge_first_sec = t.second;  // too coarse to time an edge: wait for the next one
    if (now - edge_start_ms > EDGE_TIMEOUT_MS) edge_start_ms = now;
    return;
  }
  if (ok && t.second != edge_first_sec) {
    // The edge happened somewhere in the last poll gap: take the middle of it.
    int64_t rtc_ms = rtc_to_epoch(t) * 1000 + gap / 2;
    int64_t off = rtc_ms - utc_ms();
    if (off > -SLEW_LIMIT_MS && off < SLEW_LIMIT_MS) {
      struct timeval d;
      d.tv_sec = (time_t)(off / 1000);
      d.tv_usec = (suseconds_t)((off % 1000) * 1000);
      adjtime(&d, nullptr);
      disc_stepped = false;
    } else {
      set_system_ms(rtc_ms);
      disc_stepped = true;
      Serial.printf("tk: system clock was %lld ms off the RTC; stepped\n", (long long)off);
    }
    disc_offset_ms = off;
    disc_at_ms = now;
    disc_poll_gap_ms = gap;
    disc_done = true;
    disc_count++;
    schedule_disc(REDISCIPLINE_MS);
  } else if (now - edge_start_ms > EDGE_TIMEOUT_MS) {
    disc_failures++;
    schedule_disc(RETRY_MS);
  }
}

bool set_utc_ms(int64_t new_ms) {
  int64_t s = new_ms / 1000;
  if (s < MIN_VALID_UTC || s >= MAX_VALID_UTC) {
    Serial.println("tk: rejected: time must be in 2025-2099 (unix UTC seconds, or ms)");
    return false;
  }
  Preferences prefs;
  bool nvs = prefs.begin(NVS_NS, false);
  int64_t last_set = nvs ? prefs.getLong64("set_at", 0) : 0;

  // How far off was the clock? (Includes however long the command took to type/paste.)
  if (valid_) {
    int64_t err = utc_ms() - new_ms;
    Serial.printf("tk: clock was %+.3f s off", err / 1000.0);
    // Drift is only meaningful over a day or more (the set itself is off by up to ~1 s).
    if (last_set > 0 && s - last_set >= 86400) {
      double span_s = (double)(s - last_set);
      Serial.printf(" after %.2f days since the last set (%+.1f ppm)", span_s / 86400.0,
                    err / 1000.0 / span_s * 1e6);
    }
    Serial.println();
  }

  set_system_ms(new_ms);
  // RTC: write second S at S + RTC_WRITE_PHASE_MS (see above). Blocks < 1 s.
  int64_t target = (utc_ms() / 1000) * 1000 + RTC_WRITE_PHASE_MS;
  if (utc_ms() >= target) target += 1000;
  int64_t wait = target - utc_ms();
  if (wait > 3) delay((uint32_t)(wait - 3));
  while (utc_ms() < target) {
  }
  bool rtc_ok = rtc::write(epoch_to_rtc(target / 1000));
  valid_ = true;
  invalid_reason = "";
  if (nvs) {
    prefs.putLong64("set_at", s);
    prefs.end();
  }
  disc_done = false;
  schedule_disc(FIRST_AFTER_SET_MS);

  char buf[40];
  fmt_time(buf, sizeof buf, s, true);
  Serial.printf("tk: time set: %s%s\n", buf,
                rtc_ok ? "" : "  (WARNING: RTC write failed; time will be lost on reset)");
  return true;
}

bool set_tz(const char *posix_tz) {
  if (!tz_ok(posix_tz)) {
    Serial.println("tk: rejected TZ: expected a POSIX TZ string, e.g. EST5EDT,M3.2.0,M11.1.0");
    return false;
  }
  apply_tz(posix_tz);
  Preferences prefs;
  bool saved = prefs.begin(NVS_NS, false) && prefs.putString("tz", tz_buf) > 0;
  prefs.end();
  tz_from_nvs = saved;
  char buf[40];
  fmt_time(buf, sizeof buf, utc_ms() / 1000, true);
  Serial.printf("tk: TZ %s%s; local time now %s%s\n", tz_buf, saved ? " (saved)" : " (NOT saved)",
                buf, valid_ ? "" : " (time not set)");
  return true;
}

const char *tz() { return tz_buf; }

void print_status() {
  char buf[40];
  int64_t now = utc_ms();
  if (valid_) {
    time_t tt = (time_t)(now / 1000);
    struct tm t;
    localtime_r(&tt, &t);
    char zone[16];
    strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &t);
    strftime(zone, sizeof zone, "%Z", &t);
    Serial.printf("time: %s.%03d %s%s", buf, (int)(now % 1000), zone, t.tm_isdst > 0 ? " (DST)" : "");
    fmt_time(buf, sizeof buf, now / 1000, false);
    Serial.printf(", %s\n", buf);
  } else {
    Serial.printf("time: NOT SET (%s)\n", invalid_reason);
  }
  Serial.printf("  tz: %s (%s)\n", tz_buf, tz_from_nvs ? "from NVS" : "default");

  RtcTime r;
  bool os;
  if (rtc::read(r, os)) {
    Serial.printf("  rtc: %04d-%02d-%02d %02d:%02d:%02d UTC, OS flag %d\n", r.year, r.month, r.day,
                  r.hour, r.minute, r.second, os ? 1 : 0);
  } else {
    Serial.println("  rtc: read FAILED");
  }

  if (disc_done) {
    Serial.printf("  re-discipline #%u, %u s ago: system clock minus RTC = %+lld ms (%s; edge within "
                  "%u ms)\n",
                  (unsigned)disc_count, (unsigned)((millis() - disc_at_ms) / 1000),
                  (long long)-disc_offset_ms, disc_stepped ? "stepped" : "slewed",
                  (unsigned)disc_poll_gap_ms);
  } else if (valid_) {
    Serial.println("  re-discipline: none yet");
  }
  if (valid_) {
    uint32_t el = millis() - disc_from_ms;
    uint32_t left = disc == Disc::WAIT_EDGE ? 0 : (el < disc_interval_ms ? disc_interval_ms - el : 0);
    Serial.printf("  next re-discipline in %u s; failures %u\n", (unsigned)(left / 1000),
                  (unsigned)disc_failures);
  }
  Preferences prefs;
  if (prefs.begin(NVS_NS, false)) {
    int64_t last = prefs.getLong64("set_at", 0);
    prefs.end();
    if (last > 0) {
      fmt_time(buf, sizeof buf, last, true);
      Serial.printf("  last set: %s", buf);
      if (valid_ && now / 1000 >= last)
        Serial.printf(" (%.2f days ago)", (now / 1000 - last) / 86400.0);
      Serial.println();
    } else {
      Serial.println("  last set: never");
    }
  }
}

}  // namespace tk
