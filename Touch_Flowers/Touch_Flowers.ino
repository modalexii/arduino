#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN    6    
#define PIXEL_COUNT 13

/* GROUND PIN 3 AT BOOT TO DISABLE TOUCH */
#define TOUCH_ENABLE_PIN 3
#define TOUCH_PIN 0

bool touchEnabled = true;
int touchThreshold = 2000;
int wheelPosition = 0; // tracking to always return 255 identical delay values
int wheelSpeed = 300; // moving wheel speed through recovery curve
int wheelSpeedDefault = 300;
uint32_t recoveryCurve[11] = {
  0, 0, 0, 1, 2, 3, 6, 10, 15, 20, 50
};
uint32_t curveLength = sizeof(recoveryCurve)/sizeof(recoveryCurve[0]);
uint32_t curvePosition = curveLength + 1; // set position high to disable curve
  
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
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 
};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  pinMode(TOUCH_ENABLE_PIN, INPUT_PULLUP);
  strip.begin();
  initWash();
  setMode();
}

void initWash() {
  colorWash(strip.Color(255,0,0));
  delay(1000);
  colorWash(strip.Color(0,255,0));
  delay(1000);
  colorWash(strip.Color(0,0,255));
  delay(1000);
  colorWash(strip.Color(0,0,0));
  delay(1000);
}

void setMode() {
  touchEnabled = digitalRead(TOUCH_ENABLE_PIN);
}

void loop() {

  uint32_t color;
  
  for(color=0; color<256; color++) { // foreach color
    
    colorWash(Wheel(color));
    delay(getDelay());
    
  }
  
}

bool amBeingTouched() {
  return (touchRead(TOUCH_PIN) > touchThreshold) ? true : false;
}

uint32_t getDelay() {

  if(amBeingTouched()) {
    Serial.println("Touched!");
    colorWash(strip.Color(255,255,255)); // solid white for a moment
    delay(200);
    curvePosition = 0; // reset curve so we iterate through it
  }
  else if(wheelPosition < 256) { 
    // we have not yet completed a full wheel rotation -
    // releat the previous delay time and exit
    Serial.print("COLOR: ");
    Serial.print(wheelPosition);
    Serial.print(" CURVE-X: ");
    Serial.print(curvePosition);
    Serial.print(" CURVE-Y: ");
    Serial.println(wheelSpeed);
    wheelPosition++;    
    return wheelSpeed; // unchanged
  }
 
  // we have just completed a rotation
  // figure out a new delay time
  wheelPosition = 0; // reset wheel
  Serial.println("RESET WHEEL");
  
  if(curvePosition < curveLength) { // we are somewhere in the recovery curve
    wheelSpeed = recoveryCurve[curvePosition];
    curvePosition++;
  }
  else { // we are not in the recovery curve
    wheelSpeed = wheelSpeedDefault;
  }
  return wheelSpeed;
}


void colorWash(uint32_t c) {
  // flood all pixels with a color
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

uint32_t gWheel(byte WheelPos) {
  // map int 0->255 to color red->red with gamma correction
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
