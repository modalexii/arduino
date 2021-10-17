#include "FastLED.h"
#include "FastLED_RGBW.h"

#define SM_RING_PIN 6
#define LG_RING_PIN 5
#define SM_NUM_LEDS 12
#define LG_NUM_LEDS 16
CRGB smRing[SM_NUM_LEDS];
CRGBW lgRing[LG_NUM_LEDS];
CRGB *lgRingRGB = (CRGB *) &lgRing[0];

#define TOUCH_PIN 15

//int touchThreshold = 1200;
int cycleSpeed = 200;
bool whiteModeOn = false;

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, LG_RING_PIN, RGB>(lgRingRGB, getRGBWsize(LG_NUM_LEDS));
  FastLED.addLeds<NEOPIXEL, SM_RING_PIN>(smRing, SM_NUM_LEDS);
  pinMode(A3, INPUT);
  randomSeed(analogRead(0));
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { gothy, blackout, usefulLight, blackout };
int16_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
int animate = 0;

void loop() {

  if ( amBeingTouched() ) {
    nextPattern();
  }
  gPatterns[gCurrentPatternNumber]();
  FastLED.show();
  FastLED.delay(60);
}

bool amBeingTouched() {
  int val = touchRead(TOUCH_PIN);
  Serial.print("capacitance: ");
  Serial.println(val);
  //int touchThreshold = analogRead(A3)*1.5+4000;
  int touchThreshold = map(analogRead(A3),0,1023,1000,7000);
  Serial.print("touchThreshold: ");
  Serial.println(touchThreshold);
  return (val > touchThreshold) ? true : false;
}

void nextPattern(){
  /*blackoutRGB();
  blackoutW();*/
  FastLED.clear();
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void usefulLight(){
  for(int i = 0; i < LG_NUM_LEDS; i++){
    //lgRing[i] = CRGBW(100,100,100,255);
    lgRing[i].w = 255;
  }
  for(int i = 0; i < SM_NUM_LEDS; i++){
    smRing[i] = CRGB(170,110,50);
  }
}

void gothy(){
  for(int i = 0; i < LG_NUM_LEDS; i++){
    lgRing[i] = CHSV(190,255,200);
  }
  for(int i = 0; i < SM_NUM_LEDS; i++){
    smRing[i] = CHSV(15,255,200);
  }
}

void blackout(){
  FastLED.clear();
}
