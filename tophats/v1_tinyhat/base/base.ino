#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6

Adafruit_NeoPixel strip = Adafruit_NeoPixel(39, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(40);
  pinMode(A0, INPUT_PULLUP);
  digitalWrite(A0, HIGH);
}

void offAndPause() {
  for(int i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
  delay(1500);
}

void loop() {
  //rainbowCycle3(20);
  //checker(500);
  while(digitalRead(A0) == HIGH) {
    rainbow(20);
  }
  offAndPause();
  while(digitalRead(A0) == HIGH) {
    rainbowCycle3(20);
  }
  offAndPause();
  while(digitalRead(A0) == HIGH) {
    checker(2000);
  }
  offAndPause();
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbowCycle3(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) { 
    for(i=0; i< 11; i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / 11) + j) & 255));
    }
    for(i=0; i< 12; i++) {
      strip.setPixelColor(i+11, Wheel(((i * 256 / 12) + j) & 255));
    }
    for(i=0; i< 13; i++) {
      strip.setPixelColor(i+11+12, Wheel(((i * 256 / 13) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void checker(uint16_t wait) {
  
  uint16_t i;
  
  for(i=0; i< strip.numPixels(); i++) {
    if(i%2) {
      strip.setPixelColor(i, strip.Color(0,255,255));
    }
    else {
      strip.setPixelColor(i, strip.Color(255,0,255));
    }
    strip.show();
  }
   
  delay(wait);

  for(i=0; i< strip.numPixels(); i++) {
    if(i%2) {
      strip.setPixelColor(i, strip.Color(255,0,255));
    }
    else {
      strip.setPixelColor(i, strip.Color(0,255,255));
    }
    strip.show();
  }
  delay(wait);
  
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
