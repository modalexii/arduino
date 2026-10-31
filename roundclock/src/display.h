// display.h: AMOLED panel power, Arduino_GFX driver and LVGL glue.
#pragma once
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include "board.h"

// Arduino_CO5300 plus a way to change the RAM offsets at runtime (calibration).
class ClockCO5300 : public Arduino_CO5300 {
 public:
  using Arduino_CO5300::Arduino_CO5300;
  void setOffsets(uint8_t col, uint8_t row) {
    COL_OFFSET1 = col;
    ROW_OFFSET1 = row;
    setRotation(0);                           // recomputes _xStart/_yStart
    _currentX = _currentY = -1;               // force CASET/PASET on the next write
    _currentW = _currentH = 0;
  }
  uint8_t colOffset() const { return COL_OFFSET1; }
  uint8_t rowOffset() const { return ROW_OFFSET1; }
};

extern ClockCO5300 *gfx;

// Powers the panel and starts Arduino_GFX. With with_lvgl, also starts LVGL and
// registers the flush and even-pixel rounder callbacks. Returns false on failure.
bool display_init(bool with_lvgl = true);

// Panel brightness 0-255 (CO5300 DCS 0x51). The panel's init value is 208.
void display_set_brightness(uint8_t level);
