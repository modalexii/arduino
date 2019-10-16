#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN    6    
#define PIXEL_COUNT 12

int lastRead = 0;
int luma = 100;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  //Serial.begin(115200);
  strip.begin();
  pinMode(A3, INPUT);
}

void loop() {

  int newRead = analogRead(A3);
  //Serial.println(newRead);
  if ( abs(newRead - lastRead) > 7 ) {
    luma = map(newRead,0,1023,0,255);
  }
  colorFill(strip.Color(luma,luma,luma));
  delay(10);

}

void colorFill(uint32_t c) {
  // flood all pixels with a color
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}


