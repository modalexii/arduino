// input.cpp: see input.h.
// FT3168: reg 0x02 TD_STATUS (low nibble = touch count), 0x03-0x06 = P1 XH, XL, YH, YL.
// FocalTech power registers: 0x86 G_CTRL (1 = auto-switch to monitor mode when idle,
// 0 = stay active), 0xA5 power mode (0 = active).
#include "input.h"
#include <Arduino.h>
#include <Wire.h>
#include "board.h"

namespace {
constexpr uint8_t REG_TD_STATUS = 0x02;
constexpr uint8_t REG_G_CTRL    = 0x86;
constexpr uint8_t REG_POWER     = 0xA5;

bool write_reg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(I2C_ADDR_TOUCH);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

bool keep_active() { return write_reg(REG_G_CTRL, 0) && write_reg(REG_POWER, 0); }
}  // namespace

namespace touch {

bool begin() {
  for (int i = 0; i < 5; i++) {
    if (keep_active()) return true;
    delay(20);
  }
  return false;
}

bool present() {
  Wire.beginTransmission(I2C_ADDR_TOUCH);
  return Wire.endTransmission() == 0;
}

int read(uint16_t &x, uint16_t &y) {
  static uint32_t last_wake = 0;
  Wire.beginTransmission(I2C_ADDR_TOUCH);
  Wire.write(REG_TD_STATUS);
  bool ok = Wire.endTransmission(false) == 0 && Wire.requestFrom((int)I2C_ADDR_TOUCH, 5) == 5;
  if (!ok) {
    if (millis() - last_wake > 1000) {
      last_wake = millis();
      keep_active();
    }
    return -1;
  }
  uint8_t r[5];
  for (auto &b : r) b = Wire.read();
  int count = r[0] & 0x0F;
  if (count > 5) count = 0;  // 0x0F etc. = invalid/idle
  x = ((r[1] & 0x0F) << 8) | r[2];
  y = ((r[3] & 0x0F) << 8) | r[4];
  return count;
}

}  // namespace touch
