#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN    6 
#define PIXEL_COUNT 225

uint8_t R[2] = {0,26};
uint8_t O[2] = {26,53};
uint8_t C[2] = {53,75};
uint8_t K[2] = {75,100};
uint8_t E[2] = {100,121};
uint8_t T[2] = {121,137};
uint8_t E2[2] = {137,157};
uint8_t R2[2] = {157,184};
uint8_t I[2] = {184,196};
uint8_t A[2] = {196,225};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  //strip.setBrightness(60);
  strip.show(); // Initialize all pixels to 'off'
}

void fillAll(uint32_t color){
  for (uint8_t px = 0; px < PIXEL_COUNT; px++) {
    strip.setPixelColor(px, color);
  }
}

void fillLetter(uint8_t letter[2], uint32_t c) {
  for (uint8_t i = letter[0]; i < letter[1]; i++) {
    strip.setPixelColor(i, c);
  }
}

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

void simpleRGB() {
  uint32_t red = strip.Color(255,0,0);
  uint32_t green = strip.Color(0,255,0);
  uint32_t blue = strip.Color(0,0,255);
  fillLetter(R,red);
  delay(500);
  strip.show();
  fillLetter(O,green);
  delay(500);
  strip.show();
  fillLetter(C,blue);
  delay(500);
  strip.show();
  fillLetter(K,red);
  delay(500);
  strip.show();
  fillLetter(E,green);
  delay(500);
  strip.show();
  fillLetter(T,blue);
  delay(500);
  strip.show();
  fillLetter(E2,red);
  delay(500);
  strip.show();
  fillLetter(R2,green);
  delay(500);
  strip.show();
  fillLetter(I,blue);
  delay(500);
  strip.show();
  fillLetter(A,red);
  delay(500);
  strip.show();
}

void loop() {
  //rainbowCycle(20);
  fillAll(strip.Color(200,200,200));
  strip.show();
  delay(3000);
  fillAll(strip.Color(255,255,0));
  strip.show();
  delay(3000);
  simpleRGB();
  delay(1000);
  rainbowCycle(20, 20);
  /*
  for (uint8_t px = C[0]; px < C[1]; px++){
    strip.setPixelColor(px, strip.Color(0,0,255));
    delay(200);
    strip.show();
  }
*/
}
