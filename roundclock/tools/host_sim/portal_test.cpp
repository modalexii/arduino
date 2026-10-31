// portal_test.cpp: PC tests for src/portal_logic.h (the set-time portal's /set parsing and
// time zone choice).  g++ -std=c++17 -I../../src portal_test.cpp -o portal_test && ./portal_test
#include <cstdio>
#include <cstring>
#include <time.h>
#include "portal_logic.h"
using namespace portal_logic;
static int fails = 0;
static void expect(bool ok, const char *what) {
  if (!ok) { printf("FAIL: %s\n", what); fails++; }
}
static const char *tz_for(const char *json) {
  static char out[64];
  SetRequest r;
  if (!parse_set(json, r)) return "(parse failed)";
  choose_tz(r, out, sizeof out);
  return out;
}
int main() {
  SetRequest r;
  expect(parse_set("{\"utc_ms\":1793233159871,\"tz\":\"America/New_York\",\"offset_min\":240}", r) &&
             r.utc_ms == 1793233159871LL && !strcmp(r.tz, "America/New_York") && r.offset_min == 240,
         "normal request");
  expect(parse_set(" { \"tz\" : \"Asia/Kolkata\" , \"utc_ms\" : 1793233159871.0 , \"offset_min\" : -330 } ", r) &&
             r.utc_ms == 1793233159871LL && r.offset_min == -330, "spaces, order, float ms");
  expect(!parse_set("", r), "empty body rejected");
  expect(!parse_set("hello", r), "not JSON rejected");
  expect(!parse_set("{\"tz\":\"America/New_York\"}", r), "missing utc_ms rejected");
  expect(!parse_set("{\"utc_ms\":\"abc\"}", r), "string utc_ms rejected");
  expect(!parse_set("{\"utc_ms\":12x}", r), "junk number rejected");
  expect(parse_set("{\"utc_ms\":1793233159871,\"tz\":\"Bad\\\"Zone<script>\"}", r) && r.tz[0] == 0,
         "odd tz name dropped");
  expect(!strcmp(tz_for("{\"utc_ms\":1,\"tz\":\"America/New_York\",\"offset_min\":240}"),
                 "EST5EDT,M3.2.0,M11.1.0"), "table: New York");
  expect(!strcmp(tz_for("{\"utc_ms\":1,\"tz\":\"Asia/Calcutta\"}"), "IST-5:30"), "alias: Calcutta");
  expect(!strcmp(tz_for("{\"utc_ms\":1,\"tz\":\"Mars/Base\",\"offset_min\":240}"), "<-04>4"), "fixed -4");
  expect(!strcmp(tz_for("{\"utc_ms\":1,\"tz\":\"\",\"offset_min\":-330}"), "<+0530>-5:30"), "fixed +5:30");
  expect(!strcmp(tz_for("{\"utc_ms\":1,\"offset_min\":0}"), "UTC0"), "fixed 0");
  expect(!strcmp(tz_for("{\"utc_ms\":1,\"offset_min\":-765}"), "<+1245>-12:45"), "fixed +12:45");
  expect(!strcmp(tz_for("{\"utc_ms\":1}"), ""), "no zone info: unchanged");
  expect(!strcmp(tz_for("{\"utc_ms\":1,\"offset_min\":9999}"), ""), "absurd offset ignored");
  // The fixed-offset strings must mean what we think to the C library.
  struct { const char *tz; long expect_off; } fx[] = {{"<-04>4", -4 * 3600}, {"<+0530>-5:30", 19800}, {"UTC0", 0}};
  for (auto &f : fx) {
    setenv("TZ", f.tz, 1); tzset();
    time_t t = 1793233159; struct tm lt; localtime_r(&t, &lt);
    expect(lt.tm_gmtoff == f.expect_off, f.tz);
  }
  printf("%s (%zu zones in table)\n", fails ? "PORTAL LOGIC FAILURES" : "portal logic: all tests passed", tzdb::ZONE_COUNT);
  return fails ? 1 : 0;
}
