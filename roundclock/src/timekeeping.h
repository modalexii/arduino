// timekeeping.h: PCF85063 driver (UTC), system time, POSIX TZ, NVS, hourly RTC re-discipline
// (CLOCK_PLAN.md §5.6).
//
// The RTC holds UTC and is the source of truth. At boot the ESP32's system clock is set from
// it (aligned on an RTC seconds edge), and once an hour it is re-disciplined against it:
// small differences are slewed away with adjtime() so the displayed time never steps backwards,
// large ones are stepped with settimeofday(). The local time zone is a POSIX TZ string kept in
// NVS (Preferences namespace "clock"), so localtime_r() applies DST by itself.
#pragma once
#include <stdint.h>
#include <time.h>

struct RtcTime {
  int year;     // 2000-2099
  int month;    // 1-12
  int day;      // 1-31
  int hour;     // 0-23 (24 h mode)
  int minute;   // 0-59
  int second;   // 0-59
  int weekday;  // 0-6, 0 = Sunday
};

namespace rtc {
bool present();                                  // ACKs on the I2C bus
bool read(RtcTime &t, bool &osc_stopped);        // osc_stopped = OS flag (time not trustworthy)
bool write(const RtcTime &t);                    // also clears the OS flag, forces 24 h mode
int  weekday_of(int year, int month, int day);   // 0 = Sunday
}  // namespace rtc

namespace tk {

// Call once after Wire.begin(). Loads the TZ from NVS (else cfg::DEFAULT_TZ), reads the RTC
// and, if its time is valid, sets the system clock from it (blocks up to ~1 s to catch a
// seconds edge). Returns valid().
bool begin();

// True once the clock knows the real time (valid RTC at boot, or set since).
bool valid();

// Current local time and its milliseconds. False (and tm untouched) if !valid().
bool local_now(struct tm &tm, int &ms);

// Current UTC in ms since 1970 (system clock), valid or not.
int64_t utc_ms();

// Non-blocking housekeeping: the hourly RTC re-discipline. Call often from loop().
void loop();

// Sets the time from UTC ms since 1970: system clock now, RTC on the next whole second
// (blocks up to 1 s). Rejects years outside 2025-2099. Prints what it did.
bool set_utc_ms(int64_t utc_ms);

// Sets the local time zone (POSIX TZ string, e.g. "EST5EDT,M3.2.0,M11.1.0") and saves it to
// NVS. Rejects obviously malformed strings. Prints the resulting local time.
bool set_tz(const char *posix_tz);
const char *tz();

// Serial debug: system/local time, TZ, RTC registers, last re-discipline, last set.
void print_status();

}  // namespace tk
