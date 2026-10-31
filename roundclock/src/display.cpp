// display.cpp: lifted from the original LVGL font test sketch
// (now extras/lvgl_font_test.cpp): flush, even-pixel rounder, PSRAM buffers.
#include "display.h"

static Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCK, LCD_D0, LCD_D1, LCD_D2, LCD_D3);
ClockCO5300 *gfx = new ClockCO5300(bus, LCD_RST, 0 /*rotation*/, LCD_W, LCD_H,
                                   LCD_COL_OFFSET, LCD_ROW_OFFSET, 0, 0);

// ---- LVGL <-> display glue ----
static uint32_t tick_cb() { return millis(); }

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
  lv_display_flush_ready(disp);
}

// CO5300 needs redraw areas to start/end on even pixels, or you get glitches
static void rounder_cb(lv_event_t *e) {
  lv_area_t *a = (lv_area_t *)lv_event_get_param(e);
  a->x1 &= ~1;
  a->y1 &= ~1;
  a->x2 |= 1;
  a->y2 |= 1;
}

bool display_init(bool with_lvgl) {
  pinMode(LCD_EN, OUTPUT);
  digitalWrite(LCD_EN, HIGH);
  delay(50);
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed");
    return false;
  }
  gfx->fillScreen(RGB565_BLACK);
  if (!with_lvgl) return true;

  lv_init();
  lv_tick_set_cb(tick_cb);

  lv_display_t *disp = lv_display_create(LCD_W, LCD_H);
  // Two 1/10-screen draw buffers in PSRAM
  size_t buf_bytes = LCD_W * (LCD_H / 10) * sizeof(uint16_t);
  void *buf1 = heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM);
  void *buf2 = heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM);
  if (!buf1 || !buf2) {
    Serial.println("LVGL draw buffer allocation in PSRAM failed");
    return false;
  }
  lv_display_set_buffers(disp, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, flush_cb);
  lv_display_add_event_cb(disp, rounder_cb, LV_EVENT_INVALIDATE_AREA, NULL);
  return true;
}

void display_set_brightness(uint8_t level) { gfx->setBrightness(level); }
