// input.h: FT3168 touch, polled over I2C.
// Raw touch read only, used by the calibration screen. Phase 5 (long-press / BOOT button to
// enter set-time mode) was skipped: Kyle enters set-time mode by unplugging the clock.
#pragma once
#include <stdint.h>

namespace touch {
bool present();
// Keeps the FT3168 in active mode. By default it drops into "monitor" mode when idle
// and then NAKs I2C polls every few seconds (seen in Phase 1). Retries a few times.
bool begin();
// Number of touch points (0 = not touched), or -1 on an I2C error.
// x/y are the first point's coordinates when the count is > 0.
// On an error it re-sends the keep-active setting (at most once a second).
int read(uint16_t &x, uint16_t &y);
}  // namespace touch
