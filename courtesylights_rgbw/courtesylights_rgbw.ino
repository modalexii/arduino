#include "FastLED.h"
#include "FastLED_RGBW.h"

#define DATA_PIN 6 
#define SW1_PIN 4
#define SW2_PIN 3
#define NUM_LEDS 110

CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];
uint8_t gHue = 0; // rotating "base color"
uint8_t luma = 255; // global brightness

void setup() {
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setBrightness(luma);
  FastLED.show();
  Serial.begin(115200);
}

void fillWhite(uint8_t luma){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].w = luma;
  }
}

void blackout() {
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = 0;
  }
}

void rainbow(uint8_t start_position, float width_factor){
  // lower width factor = spread rainbow farther, higher = fit more rainbows
  // good values are 0.2 -> 2
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CHSV((i * 256 / NUM_LEDS * width_factor) + start_position, 255, 255);
  }
}

void loop() {

    if(!(digitalRead(SW1_PIN))){
      rainbow(gHue,0.9);
      gHue = gHue < 256 ? gHue + 1 : 0;
    } else if(!(digitalRead(SW2_PIN))) {
      fillWhite(200);
    } else {
      blackout();
    }
    FastLED.show();
    delay(50);
    
}
