#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN    6    
#define PIXEL_COUNT 12

#define TOUCH_PIN 0
int touchThreshold = 2000;
int cycleDelay = 300;

const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  //Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  init();
}

void init() {
  colorWipe(strip.Color(255,0,0));
  delay(1000);
  colorWipe(strip.Color(0,255,0));
  delay(1000);
  colorWipe(strip.Color(0,0,255));
  delay(1000);
  colorWipe(strip.Color(0,0,0));
  delay(1000);
}

void loop() {

  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();

    int touchValue = touchRead(TOUCH_PIN);
    
    if(touchValue > touchThreshold) {
      onTouch();
    }
    else {
      delay(cycleDelay);
    }
    
  }
  
}

void colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
  }
}

void onTouch() {

  uint16_t wait = 0;
  
  uint16_t i, j, k, l;

  colorWipe(strip.Color(255,255,255));
  delay(400);

  for(j=0; j<256; j++) { // iterate colors
    for(i=0; i<strip.numPixels(); i++) { // iterate pixels
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(0);
  }

  while(wait < 10) {
    for(j=0; j<256; j++) { // iterate colors
      for(i=0; i<strip.numPixels(); i++) { // iterate pixels
        strip.setPixelColor(i, Wheel((i+j) & 255));
      }
      strip.show();
      delay(wait);
    }
    wait += 1;
  }
    
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(
      pgm_read_byte(&gamma8[255 - WheelPos * 3]), 
      pgm_read_byte(&gamma8[0]), 
      pgm_read_byte(&gamma8[WheelPos * 3])
    );
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(
      pgm_read_byte(&gamma8[0]), 
      pgm_read_byte(&gamma8[WheelPos * 3]), 
      pgm_read_byte(&gamma8[255 - WheelPos * 3])
    );
  }
  WheelPos -= 170;
  return strip.Color(
    pgm_read_byte(&gamma8[WheelPos * 3]), 
    pgm_read_byte(&gamma8[255 - WheelPos * 3]), 
    pgm_read_byte(&gamma8[0])
  );
}
