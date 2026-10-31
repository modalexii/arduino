// portal_logic.h: the set-time portal's pure logic (no Arduino), so it can be tested on a PC
// (tools/host_sim/portal_test.cpp): parsing the /set JSON and choosing the POSIX TZ.
#pragma once
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tz_table.h"

namespace portal_logic {

struct SetRequest {
  int64_t utc_ms = 0;
  bool has_utc = false;
  char tz[48] = "";       // IANA name as sent (sanitised); "" = none
  int offset_min = 0;     // JS getTimezoneOffset(): minutes, positive WEST of UTC
  bool has_offset = false;
};

// Finds "key" followed by optional spaces and ':'; returns the char after the colon (spaces
// skipped), or nullptr.
inline const char *find_value(const char *json, const char *key) {
  char pat[24];
  snprintf(pat, sizeof pat, "\"%s\"", key);
  const char *p = strstr(json, pat);
  if (!p) return nullptr;
  p += strlen(pat);
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
  if (*p != ':') return nullptr;
  p++;
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
  return p;
}

// Parses an integer (an optional fraction like 1793.0 is accepted and dropped).
inline bool parse_int(const char *p, int64_t &out) {
  if (!p) return false;
  char *end = nullptr;
  long long v = strtoll(p, &end, 10);
  if (end == p) return false;
  if (*end == '.') {
    end++;
    while (isdigit((unsigned char)*end)) end++;
  }
  if (*end && *end != ',' && *end != '}' && !isspace((unsigned char)*end)) return false;
  out = v;
  return true;
}

// Parses the body of POST /set. Unknown keys are ignored. False if it isn't a JSON object or
// has no usable utc_ms. The tz name is kept only if it looks like an IANA name.
inline bool parse_set(const char *json, SetRequest &r) {
  r = SetRequest();
  if (!json) return false;
  while (isspace((unsigned char)*json)) json++;
  if (*json != '{') return false;
  int64_t v;
  if (parse_int(find_value(json, "utc_ms"), v) && v > 0) {
    r.utc_ms = v;
    r.has_utc = true;
  }
  if (parse_int(find_value(json, "offset_min"), v) && v >= -14 * 60 && v <= 14 * 60) {
    r.offset_min = (int)v;
    r.has_offset = true;
  }
  const char *p = find_value(json, "tz");
  if (p && *p == '"') {
    p++;
    size_t n = 0;
    bool ok = true;
    while (*p && *p != '"') {
      char c = *p++;
      if (!(isalnum((unsigned char)c) || c == '/' || c == '_' || c == '-' || c == '+')) ok = false;
      if (n + 1 >= sizeof r.tz) ok = false;
      if (ok) r.tz[n++] = c;
    }
    r.tz[ok && *p == '"' ? n : 0] = 0;
  }
  return r.has_utc;
}

// POSIX TZ for a fixed offset from JS getTimezoneOffset() (positive west), e.g. 240 ->
// "<-04>4", -330 -> "<+0530>-5:30", 0 -> "UTC0". No DST.
inline void posix_fixed(int offset_min, char *out, size_t n) {
  if (offset_min == 0) {
    snprintf(out, n, "UTC0");
    return;
  }
  int a = offset_min < 0 ? -offset_min : offset_min;
  int h = a / 60, m = a % 60;
  char name[12];
  if (m) snprintf(name, sizeof name, "%c%02d%02d", offset_min > 0 ? '-' : '+', h, m);
  else snprintf(name, sizeof name, "%c%02d", offset_min > 0 ? '-' : '+', h);
  if (m) snprintf(out, n, "<%s>%s%d:%02d", name, offset_min > 0 ? "" : "-", h, m);
  else snprintf(out, n, "<%s>%s%d", name, offset_min > 0 ? "" : "-", h);
}

enum class TzSource { TABLE, FIXED_OFFSET, UNCHANGED };

// Chooses the POSIX TZ for a request: the table entry for its IANA name, else a fixed offset,
// else none (keep the clock's current TZ). out gets the string for TABLE / FIXED_OFFSET.
inline TzSource choose_tz(const SetRequest &r, char *out, size_t n) {
  if (r.tz[0]) {
    const char *p = tzdb::lookup(r.tz);
    if (p) {
      snprintf(out, n, "%s", p);
      return TzSource::TABLE;
    }
  }
  if (r.has_offset) {
    posix_fixed(r.offset_min, out, n);
    return TzSource::FIXED_OFFSET;
  }
  out[0] = 0;
  return TzSource::UNCHANGED;
}

}  // namespace portal_logic
