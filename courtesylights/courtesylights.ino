#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN 6 
#define SW1_PIN 4
#define SW2_PIN 3
#define PIXEL_COUNT 65

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
uint8_t gHue = 0; // rotating "base color"

void setup() {
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);
  strip.begin();
  strip.setBrightness(200);
  strip.show(); // Initialize all pixels to 'off'
}

void fillAll(uint32_t color){
  for (uint8_t px = 0; px < PIXEL_COUNT; px++) {
    strip.setPixelColor(px, color);
  }
}

void rainbowFill(uint8_t ghue) {
  uint16_t i;
  for(i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel( ( (i * 256 / strip.numPixels() ) + ghue) & 255) );
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void loop() {

    if(!(digitalRead(SW1_PIN))){
      fillAll(Wheel(gHue));
      gHue = gHue < 256 ? gHue + 1 : 0;
    } else if(!(digitalRead(SW2_PIN))) {
      fillAll(strip.Color(85, 85, 80));
    } else {
      fillAll(0);
    }
    strip.show();
    delay(50);

  /*
  for (uint8_t px = C[0]; px < C[1]; px++){
    strip.setPixelColor(px, strip.Color(0,0,255));
    delay(200);
    strip.show();
  }
*/
}
