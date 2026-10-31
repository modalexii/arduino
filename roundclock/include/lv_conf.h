// Minimal LVGL 9 config - anything not set here uses LVGL's defaults.
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16

// Use the C library malloc, so big buffers can land in PSRAM
#define LV_USE_STDLIB_MALLOC  LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING  LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

#define LV_USE_LOG 0

// Built-in fonts (smooth). Custom fonts live in src/ and use LV_FONT_DECLARE.
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_28 1   // set-time screen message
#define LV_FONT_MONTSERRAT_48 1   // set-time screen check mark (LV_SYMBOL_OK)
#define LV_FONT_DEFAULT &lv_font_montserrat_14

// Hour numeral: rendered at any size from the embedded TTF (CLOCK_PLAN.md §5.1).
// Glyph bitmaps are cached in PSRAM through the CLIB malloc above.
#define LV_USE_TINY_TTF 1
#define LV_TINY_TTF_FILE_SUPPORT 0   // fonts come from memory, not a filesystem
// LV_USE_FLOAT is left off: the ring and ticks are drawn by face.cpp's own float
// renderer, so LVGL's integer arc/line coordinates don't matter.

#endif
