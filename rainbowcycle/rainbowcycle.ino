#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN    6 
#define PIXEL_COUNT 12


Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
  
void setup() {

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
}



void flood(uint8_t v) {
  for(int i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i,v,v,v);
  }
}

void loop() {
  flood(255);

}
