#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>

#define PIXEL_PIN    6 
#define PIXEL_COUNT 8*18 //144
#define MATRIX_WIDTH 18
#define MATRIX_HEIGHT 8

char text[64] = "LIGHT CITY BALTIMORE";
int  pixelPerChar = 5;
int  maxDisplacement;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH,
  MATRIX_HEIGHT,
  PIXEL_PIN,
  NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS   + NEO_MATRIX_ZIGZAG,
  NEO_GRB           + NEO_KHZ800 
);

const uint16_t colors[] = {
  matrix.Color(0, 255, 0), matrix.Color(0, 255, 255), matrix.Color(255, 0, 255) };
  
void setup() {

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(colors[0]);
  maxDisplacement = strlen(text) * pixelPerChar + matrix.width();

  strip.begin();
  strip.setBrightness(60);
  strip.show(); // Initialize all pixels to 'off'
  
}

void marqueeText() {
  int x = matrix.width();
  int pass = 0;
  for(int pos = 0; pos <= MATRIX_WIDTH*5*3; pos++) {
    matrix.fillScreen(0);
    matrix.setCursor(x, 0);
    matrix.print(F("LIGHT CITY"));
    if (--x < -maxDisplacement) {
      x = matrix.width();
      if(++pass >= 3) pass = 0;
      matrix.setTextColor(colors[pass]);
    }
    matrix.show();
    delay(100);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait, int rpt) {
  uint16_t i, j;

  for(j=0; j<256*rpt; j++) { 
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( ( (i * 256 / strip.numPixels() ) + j) & 255) );
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
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
  //rainbowCycle(20);
  rainbowCycle(10, 5);
  marqueeText();
}
