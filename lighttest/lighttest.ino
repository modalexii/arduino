#include <OctoWS2811.h>

const int ledsPerStrip = 115*2;
const int totalLEDs = 115*2*8;
int brightness = 30;

DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];

const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

void setup() {
  leds.begin();
  leds.show();
}

#define RED    0xFF0000
#define GREEN  0x00FF00
#define BLUE   0x0000FF
#define YELLOW 0xFFFF00
#define PINK   0xFF1088
#define ORANGE 0xE05800
#define WHITE  0xFFFFFF

void loop() {
  int microsec = 2000000 / leds.numPixels();  // change them all in 2 seconds

  // uncomment for voltage controlled speed
  // millisec = analogRead(A9) / 40;

  rainbowCycle(10);

}

void colorWipe(int color, int wait)
{
  for (int i=0; i < leds.numPixels(); i++) {
    leds.setPixel(i, color);
    //delayMicroseconds(wait);
  }
  delay(500);
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return getRGBL(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return getRGBL(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return getRGBL(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void fillSection(const int section[], uint32_t color) {
  for (int i=section[0]; i <= section[1]; i++) {
    leds.setPixel(i,color);
  }
  leds.show();
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) { 
    for(i=0; i< totalLEDs; i++) {
      leds.setPixel(i, Wheel(((i * 256 / totalLEDs) + j) & 255));
    }
    leds.show();
    delay(wait);
  }
}

uint32_t getRGBL(uint8_t r, uint8_t g, uint8_t b) {
  r = (r * brightness) >> 8;
  g = (g * brightness) >> 8;
  b = (b * brightness) >> 8;
  return (r << 16) | (g << 8) | b;
}


