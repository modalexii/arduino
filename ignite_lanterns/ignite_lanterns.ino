#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN    6    
#define PIXEL_COUNT 12

#define TOUCH_PIN 15

//int touchThreshold = 1200;
int cycleSpeed = 200;
bool whiteModeOn = false;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  strip.begin();
  pinMode(A3, INPUT);
}

void loop() {

  uint32_t color;
  
  for(color=0; color<256; color++) { // foreach color
    if(whiteModeOn) {
      // override cycle color with white
      colorFill(strip.Color(255,255,255));
    } else {
      colorFill(Wheel(color));
    }
    delay(cycleSpeed);
    setMode();
  }
  
}

bool amBeingTouched() {
  int val = touchRead(TOUCH_PIN);
  Serial.print("capacitance: ");
  Serial.println(val);
  int touchThreshold = analogRead(A3)*1.5+1000;
  Serial.print("touchThreshold: ");
  Serial.println(touchThreshold);
  return (val > touchThreshold) ? true : false;
}

void setMode() {

  if(amBeingTouched()) {
    //Serial.println("Touched!");
    whiteModeOn = !whiteModeOn;
  }
  
}

void colorFill(uint32_t c) {
  // flood all pixels with a color
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

uint32_t Wheel(byte WheelPos) {
  // map int 0->255 to color red->red
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
