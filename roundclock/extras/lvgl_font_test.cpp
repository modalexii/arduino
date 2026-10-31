#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

// Pins from Waveshare docs
#define LCD_CS   9
#define LCD_SCK  10
#define LCD_D0   11
#define LCD_D1   12
#define LCD_D2   13
#define LCD_D3   14
#define LCD_RST  21
#define LCD_EN   42   // AMOLED power enable
#define LCD_W    466
#define LCD_H    466

Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCK, LCD_D0, LCD_D1, LCD_D2, LCD_D3);
// CO5300 panel; 6px column offset is typical for the 466x466 glass
Arduino_GFX *gfx = new Arduino_CO5300(bus, LCD_RST, 0 /*rotation*/, LCD_W, LCD_H, 6, 0, 0, 0);

LV_FONT_DECLARE(nixie_one_120);   // generated from NixieOne-Regular.ttf (digits, ':' and space)

static lv_obj_t *clock_label;

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

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== AMOLED 1.43 LVGL test ===");
  Serial.printf("Flash: %u MB, PSRAM: %u KB\n", ESP.getFlashChipSize() / (1024 * 1024), ESP.getPsramSize() / 1024);

  pinMode(LCD_EN, OUTPUT);
  digitalWrite(LCD_EN, HIGH);
  delay(50);
  if (!gfx->begin()) Serial.println("gfx->begin() failed");
  gfx->fillScreen(RGB565_BLACK);

  lv_init();
  lv_tick_set_cb(tick_cb);

  lv_display_t *disp = lv_display_create(LCD_W, LCD_H);
  // Two 1/10-screen draw buffers in PSRAM
  size_t buf_bytes = LCD_W * (LCD_H / 10) * sizeof(uint16_t);
  void *buf1 = heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM);
  void *buf2 = heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM);
  lv_display_set_buffers(disp, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, flush_cb);
  lv_display_add_event_cb(disp, rounder_cb, LV_EVENT_INVALIDATE_AREA, NULL);

  // ---- UI ----
  lv_obj_t *scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

  clock_label = lv_label_create(scr);
  lv_obj_set_style_text_font(clock_label, &nixie_one_120, 0);
  lv_obj_set_style_text_color(clock_label, lv_color_hex(0xFF8A1E), 0);  // nixie-tube orange
  lv_label_set_text(clock_label, "00:00");
  lv_obj_center(clock_label);

  lv_obj_t *caption = lv_label_create(scr);
  lv_obj_set_style_text_font(caption, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(caption, lv_color_hex(0x808080), 0);
  lv_label_set_text(caption, "Nixie One");
  lv_obj_align(caption, LV_ALIGN_CENTER, 0, 100);
}

void loop() {
  // Fake clock: minutes:seconds since boot (RTC comes later)
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    uint32_t s = millis() / 1000;
    lv_label_set_text_fmt(clock_label, "%02lu:%02lu", (unsigned long)((s / 60) % 100), (unsigned long)(s % 60));
  }
  lv_timer_handler();
  delay(5);
}
