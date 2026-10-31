// timeset_portal.cpp: see timeset_portal.h.
#include "timeset_portal.h"
#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>
#include "clock_config.h"
#include "portal_logic.h"
#include "portal_page.h"
#include "timekeeping.h"

namespace portal {
namespace {

constexpr uint32_t LINGER_MS = 3000;  // keep Wi-Fi up this long after a set, so the reply arrives
constexpr int64_t MIN_UTC_MS = 1735689600LL * 1000;  // 2025-01-01 (tk::set_utc_ms checks too)
constexpr int64_t MAX_UTC_MS = 4102444800LL * 1000;  // 2100-01-01
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_MASK(255, 255, 255, 0);

DNSServer *dns = nullptr;
WebServer *web = nullptr;
bool active_ = false;
uint32_t started_ms = 0;
uint32_t set_ms = 0;       // when the last set happened (linger timer); 0 = none this session
bool set_pending = false;  // TIME_SET not yet reported by loop()
int last_stations = -1;
uint32_t requests = 0, sets = 0, rejects = 0;
char last_result[96] = "none";

// With no valid time there's nothing better to show, so (cfg::AP_STAY_UP_UNTIL_SET) the
// hotspot stays up until a phone sets the time: there's no other trigger to restart it.
bool times_out() { return tk::valid() || !cfg::AP_STAY_UP_UNTIL_SET; }

String portal_url() { return String("http://") + AP_IP.toString() + "/"; }

void log_req(const char *what) {
  requests++;
  if (cfg::DEBUG_LOG) Serial.printf("portal: %s %s%s (host %s)\n", web->method() == HTTP_POST ? "POST" : "GET",
                web->uri().c_str(), what, web->hostHeader().c_str());
}

void no_cache() {
  web->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  web->sendHeader("Pragma", "no-cache");
  web->sendHeader("Expires", "-1");
}

void send_page() {
  log_req("");
  no_cache();
  web->send_P(200, "text/html; charset=utf-8", PORTAL_PAGE);
}

// OS connectivity checks and anything unknown: send the phone to our page, which makes iOS,
// Android and Windows show their "sign in to network" sheet.
void redirect() {
  log_req(" -> redirect");
  no_cache();
  web->sendHeader("Location", portal_url(), true);
  web->send(302, "text/plain", "");
}

void reply_error(int code, const char *err) {
  rejects++;
  snprintf(last_result, sizeof last_result, "rejected: %s", err);
  Serial.printf("portal: set rejected (%d): %s\n", code, err);
  no_cache();
  String body = String("{\"ok\":false,\"error\":\"") + err + "\"}";
  web->send(code, "application/json", body);
}

void handle_set() {
  log_req("");
  String body = web->arg("plain");
  if (cfg::DEBUG_LOG) Serial.printf("portal: body %s\n", body.c_str());
  portal_logic::SetRequest r;
  if (!portal_logic::parse_set(body.c_str(), r)) return reply_error(400, "Bad request");
  if (r.utc_ms < MIN_UTC_MS || r.utc_ms >= MAX_UTC_MS)
    return reply_error(400, "Phone time out of range (2025-2099)");

  char posix[64];
  portal_logic::TzSource src = portal_logic::choose_tz(r, posix, sizeof posix);
  if (src == portal_logic::TzSource::FIXED_OFFSET)
    Serial.printf("portal: WARNING: zone '%s' not in tz_table.h; using fixed offset %s (no DST)\n",
                  r.tz, posix);
  if (src != portal_logic::TzSource::UNCHANGED && !tk::set_tz(posix))
    return reply_error(400, "Bad time zone");
  // The phone's clock was read when it sent the request; add the time since it arrived.
  // (Round trip on the local AP is a few tens of ms; not measured.)
  if (!tk::set_utc_ms(r.utc_ms)) return reply_error(400, "Time rejected");

  struct tm t;
  int ms;
  char local[16] = "?";
  if (tk::local_now(t, ms)) {
    int h = t.tm_hour % 12;
    if (h == 0) h = 12;
    snprintf(local, sizeof local, "%d:%02d %s", h, t.tm_min, t.tm_hour < 12 ? "AM" : "PM");
  }
  const char *zone = r.tz[0] ? r.tz : "";
  snprintf(last_result, sizeof last_result, "set %s, zone %s (%s)", local, zone[0] ? zone : "-",
           src == portal_logic::TzSource::TABLE         ? "table"
           : src == portal_logic::TzSource::FIXED_OFFSET ? "fixed offset"
                                                          : "unchanged");
  Serial.printf("portal: %s\n", last_result);
  no_cache();
  String out = String("{\"ok\":true,\"local\":\"") + local + "\",\"tz\":\"" + zone + "\"}";
  web->send(200, "application/json", out);
  sets++;
  set_ms = millis();
  if (set_ms == 0) set_ms = 1;
  set_pending = true;
}

void shutdown(const char *why) {
  if (web) {
    web->stop();
    delete web;
    web = nullptr;
  }
  if (dns) {
    dns->stop();
    delete dns;
    dns = nullptr;
  }
  WiFi.softAPdisconnect(true);  // with wifioff = true this already switches Wi-Fi off
  if (WiFi.getMode() != WIFI_OFF) WiFi.mode(WIFI_OFF);  // (calling it again while stopping logs an error)
  active_ = false;
  Serial.printf("portal: Wi-Fi off (%s)\n", why);
}

}  // namespace

bool start() {
  if (active_) return true;
  uint32_t t0 = millis();
  WiFi.persistent(false);  // never write Wi-Fi settings to flash
  if (!WiFi.mode(WIFI_AP)) {
    Serial.println("portal: WiFi.mode(AP) failed");
    return false;
  }
  WiFi.softAPConfig(AP_IP, AP_IP, AP_MASK);
  const char *pw = strlen(cfg::AP_PASSWORD) >= 8 ? cfg::AP_PASSWORD : nullptr;
  if (!WiFi.softAP(cfg::AP_SSID, pw)) {
    Serial.println("portal: softAP failed");
    WiFi.mode(WIFI_OFF);
    return false;
  }
  dns = new DNSServer();
  dns->setErrorReplyCode(DNSReplyCode::NoError);
  dns->start(53, "*", AP_IP);

  web = new WebServer(80);
  web->on("/", HTTP_GET, send_page);
  web->on("/set", HTTP_POST, handle_set);
  web->on("/set", HTTP_OPTIONS, [] { web->send(204); });
  // Captive-portal probes (iOS/macOS, Android, Windows, Firefox) all land on the page.
  for (const char *p : {"/hotspot-detect.html", "/library/test/success.html", "/generate_204",
                        "/gen_204", "/connecttest.txt", "/ncsi.txt", "/redirect",
                        "/canonical.html", "/success.txt"})
    web->on(p, redirect);
  web->onNotFound(redirect);
  web->begin();

  active_ = true;
  started_ms = millis();
  set_ms = 0;
  set_pending = false;
  last_stations = -1;
  Serial.printf("portal: hotspot \"%s\" (%s) up at %s in %u ms; ", cfg::AP_SSID,
                pw ? "WPA2" : "open", AP_IP.toString().c_str(), (unsigned)(millis() - t0));
  if (times_out()) Serial.printf("timeout %u s\n", (unsigned)cfg::AP_TIMEOUT_S);
  else Serial.println("stays up until the time is set");
  return true;
}

void stop() {
  if (active_) shutdown("stopped");
}

bool active() { return active_; }

Event loop() {
  if (!active_) {
    if (set_pending) {  // can't happen (set needs active), but don't lose the event
      set_pending = false;
      return Event::TIME_SET;
    }
    return Event::NONE;
  }
  dns->processNextRequest();
  web->handleClient();

  int st = WiFi.softAPgetStationNum();
  if (st != last_stations) {
    if (last_stations >= 0 && cfg::DEBUG_LOG) Serial.printf("portal: %d phone(s) connected\n", st);
    last_stations = st;
  }

  if (set_pending) {
    set_pending = false;
    return Event::TIME_SET;
  }
  if (set_ms && millis() - set_ms >= LINGER_MS) {
    shutdown("time set");
    return Event::NONE;
  }
  if (!set_ms && times_out() && millis() - started_ms >= cfg::AP_TIMEOUT_S * 1000UL) {
    shutdown("timeout, no time received");
    return Event::TIMEOUT;
  }
  return Event::NONE;
}

void print_status() {
  if (active_) {
    uint32_t el = (millis() - started_ms) / 1000;
    Serial.printf("portal: ACTIVE \"%s\" at %s, %d phone(s), up %u s", cfg::AP_SSID,
                  AP_IP.toString().c_str(), WiFi.softAPgetStationNum(), (unsigned)el);
    if (!set_ms && !times_out()) Serial.print(", stays up until the time is set");
    else if (!set_ms) Serial.printf(", timeout in %d s", (int)cfg::AP_TIMEOUT_S - (int)el);
    Serial.println();
  } else {
    Serial.printf("portal: off (Wi-Fi mode %d)\n", (int)WiFi.getMode());
  }
  Serial.printf("  requests %u, sets %u, rejected %u; last: %s\n", (unsigned)requests,
                (unsigned)sets, (unsigned)rejects, last_result);
}

}  // namespace portal
