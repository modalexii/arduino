// board.h: Waveshare ESP32-S3-Touch-AMOLED-1.43 pin map and I2C addresses (see BOARD_NOTES.md).
#pragma once
#include <stdint.h>

// AMOLED (QSPI)
constexpr int LCD_CS  = 9;
constexpr int LCD_SCK = 10;
constexpr int LCD_D0  = 11;
constexpr int LCD_D1  = 12;
constexpr int LCD_D2  = 13;
constexpr int LCD_D3  = 14;
constexpr int LCD_RST = 21;
constexpr int LCD_EN  = 42;   // AMOLED power enable; must be HIGH
constexpr int LCD_W   = 466;
constexpr int LCD_H   = 466;
constexpr int LCD_COL_OFFSET = 6;  // CO5300 column offset; Phase 1 verifies this
constexpr int LCD_ROW_OFFSET = 0;  // CO5300 row offset

// Shared I2C bus: touch, RTC, IMU
constexpr int I2C_SDA = 47;
constexpr int I2C_SCL = 48;
constexpr uint32_t I2C_FREQ = 400000;
constexpr uint8_t I2C_ADDR_TOUCH = 0x38;  // FT3168
constexpr uint8_t I2C_ADDR_RTC   = 0x51;  // PCF85063
constexpr uint8_t I2C_ADDR_IMU_A = 0x6A;  // QMI8658 (SA0 low)
constexpr uint8_t I2C_ADDR_IMU_B = 0x6B;  // QMI8658 (SA0 high)

// Buttons
constexpr int PIN_BOOT_BTN = 0;  // active low; normal input after boot
