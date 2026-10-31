// timeset_portal.h: SoftAP "Clock", DNSServer catch-all, WebServer and captive-portal endpoints
// (CLOCK_PLAN.md §2, §5.7).
//
// start() brings up an open (or WPA2, see cfg::AP_PASSWORD) access point at 192.168.4.1 with a
// DNS server that answers every name with that address, so a phone's captive-portal check
// opens the set-time page by itself. The page POSTs the phone's time and IANA zone to /set;
// the zone becomes a POSIX TZ string (tz_table.h, else a fixed offset) and is applied with
// tk::set_tz() and tk::set_utc_ms(). Wi-Fi then turns off completely a few seconds later
// (so the reply reaches the phone), or after cfg::AP_TIMEOUT_S with no time received (but not
// while the clock has no valid time, if cfg::AP_STAY_UP_UNTIL_SET).
#pragma once
#include <stdint.h>

namespace portal {

enum class Event {
  NONE,
  TIME_SET,  // a phone set the time (reported once; Wi-Fi turns off ~3 s later by itself)
  TIMEOUT,   // cfg::AP_TIMEOUT_S passed with no time received; Wi-Fi is already off
};

// Starts the hotspot. False if Wi-Fi couldn't start. Does nothing if already active.
bool start();

// Turns Wi-Fi off completely (no event). Safe to call when not active.
void stop();

// True while the hotspot is up.
bool active();

// Serves DNS and HTTP, and handles the linger/timeout. Call often (every loop) while active().
Event loop();

// Serial debug: state, stations connected, requests served, last set.
void print_status();

}  // namespace portal
