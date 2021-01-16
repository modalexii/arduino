#include "FastLED.h"

#define NUM_STRIPS 2
#define NUM_LEDS_PER_STRIP 16

uint8_t gHue = 0;
uint8_t gHueComp = 0;
  
CRGB leds[NUM_STRIPS][NUM_LEDS_PER_STRIP];

void setup() {
  FastLED.addLeds<NEOPIXEL, 6>(leds[0], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 5>(leds[1], NUM_LEDS_PER_STRIP);
  FastLED.setBrightness(100);
  Serial.begin(115200);
}

void fill_l(uint8_t hue, uint8_t value) {
  fill_l_top(hue,value);
  fill_l_bottom(hue,value);
}

void fill_r(uint8_t hue, uint8_t value) {
  fill_r_top(hue,value);
  fill_r_bottom(hue,value);
}

void fill_l_top(uint8_t hue, uint8_t value) {
  for(int i = 11; i < 16; i++) {
    leds[0][i].setHSV(hue,255,value);
  }
  for(int i = 0; i < 3; i++) {
    leds[0][i].setHSV(hue,255,value);
  }
}

void fill_l_bottom(uint8_t hue, uint8_t value) {
  for(int i = 3; i < 11; i++) {
    leds[0][i].setHSV(hue,255,value);
  }
}

void fill_r_top(uint8_t hue, uint8_t value) {
  for(int i = 1; i < 9; i++) {
    leds[1][i].setHSV(hue,255,value);
  }
}

void fill_r_bottom(uint8_t hue, uint8_t value) {
  for(int i = 9; i < 16; i++) {
    leds[1][i].setHSV(hue,255,value);
  }
  leds[1][0].setHSV(hue,255,value);
}

void rotate(bool cw, bool side, uint8_t steps) {
  for(uint8_t s = 0; s <= steps; s++ ) {
    if(cw) {
      for(int i = 0; i < 16; i++) {
        leds[side][i] = leds[side][(i+1)%16]; 
      }
    } else {
      leds[side][0] = leds[1][15];
      for(int i = 15; i > 0; i--) {
        leds[side][i] = leds[side][(i-1)]; 
      }
    }
  }
}


void loop() {
  gHue = gHue < 255 ? gHue + 1 : 0;
  gHueComp = gHueComp > 0 ? gHueComp - 1 : 255;
  fill_r_top(gHue,255);
  fill_l_top(gHue,255);
  fill_r_bottom(gHueComp,255);
  fill_l_bottom(gHueComp,255);
  delay(50);
  FastLED.show();

  /*(int x = 0; x < NUM_STRIPS; x++) {
    // This inner loop will go over each led in the current strip, one at a time
    for(int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
      leds[x][i].setHSV(gHue,255,255);
      FastLED.show();
      leds[x][i] = CRGB::White;
      delay(100);
    }
  }*/
  /*fill_rainbow(leds[0], NUM_LEDS_PER_STRIP, gHue, 14); 
  fill_rainbow(leds[1], NUM_LEDS_PER_STRIP, gHue, 14);
  FastLED.show();
  gHue = gHue < 250 ? gHue + 5 : 0;
  Serial.println(gHue);
  delay(50);*/
}
