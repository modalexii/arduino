#include <ClickEncoder.h>
#include <TimerOne.h>
#include "FastLED.h"
#include "FastLED_RGBW.h"
#define NUM_LEDS 175
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];
uint8_t luma = 30; // global brightness
uint8_t geom_bottom_center[2] = {26,47};
uint8_t geom_bottom_left[2] = {47,73};
uint8_t geom_side_left[2] = {73,89};
uint8_t geom_top_left[2] = {89,114};
uint8_t geom_top_center[2] = {114,135};
uint8_t geom_top_right[2] = {135,162};
uint8_t geom_side_right[2] = {162,NUM_LEDS};
uint8_t geom_bottom[2] = {0,73};
uint8_t geom_top[2] = {89,162};

ClickEncoder *encoder;
int16_t last, value;

void timerIsr() {
  encoder->service();
}

void setup() {
  Serial.begin(9600);
  encoder = new ClickEncoder(A1, A0, A2, 1);
  Timer1.initialize(10000);
  Timer1.attachInterrupt(timerIsr); 
  last = -1;
  FastLED.addLeds<WS2812B, 6, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setBrightness(luma);
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { fillWWhite, pastel1, rainbow, slowRainbowFade, dimWhiteNumber };
int16_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void loop() {  
  //value += encoder->getValue();
  /*if (value > last) {
    last = value;
    Serial.println("Incremented");
    nextPattern(1);
  } else if (value < last) {
    last = value;
    Serial.println("Decremented");
    nextPattern(-1);
  }*/
  /*if (value != last) {
    last = value;
    Serial.print("Encoder Value: ");
    Serial.println(value);
    nextPattern(value);
  }*/
  value += encoder->getValue();
  if (value != last) {
    last = value;
    Serial.println("Changing pattern");
    nextPattern(1);
  }
  /*ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
      Serial.println("ClickEncoder::Clicked");       
  }*/
  gPatterns[gCurrentPatternNumber]();
  FastLED.show();
}


#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern(int16_t vector){
  blackoutRGB();
  blackoutW();
  gCurrentPatternNumber = (gCurrentPatternNumber + vector) % ARRAY_SIZE(gPatterns);
  Serial.print("Next pattern; vector ");
  Serial.print(vector);
  Serial.print(", pattern number now ");
  Serial.println(gCurrentPatternNumber);
}

void geomTest() {
  for(uint8_t i = geom_bottom_center[0]; i < geom_bottom_center[1]; i++){
    leds[i] = CHSV(0, 255, 255);
  }
  /*for(int i = 0; i < 26; i++){
    leds[i] = CHSV(0, 255, 255);
  }
  for(int i = 26; i < 47; i++){
    leds[i] = CHSV(90, 255, 255);
  }
  for(int i = 47; i < 73; i++){
    leds[i] = CHSV(0, 255, 255);
  }
  for(int i = 73; i < 89; i++){
    leds[i] = CHSV(180, 255, 255);
  }
  for(int i = 89; i < 114; i++){
    leds[i] = CHSV(0, 255, 255);
  }
  for(int i = 114; i < 135; i++){
    leds[i] = CHSV(90, 255, 255);
  }
  for(int i = 135; i < 162; i++){
    leds[i] = CHSV(0, 255, 255);
  }
  for(int i = 162; i < NUM_LEDS; i++){
    leds[i] = CHSV(180, 255, 255);
  }*/
}

void rainbow(){
  float width_factor = 0.9;
  // lower width factor = spread rainbow farther, higher = fit more rainbows
  // good values are 0.2 -> 2
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CHSV((i * 256 / NUM_LEDS * width_factor) + gHue, 255, 255);
  }
  EVERY_N_MILLISECONDS( 400 ) { gHue++; } 
  noSides();
}

void slowRainbowFade(){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CHSV(gHue, 255, 255);
  }
  EVERY_N_MILLISECONDS( 200 ) { gHue++; }
  noSides();
}

void pastel1(){
  float width_factor = 0.8;
  // lower width factor = spread rainbow farther, higher = fit more rainbows
  // good values are 0.2 -> 2
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CHSV((i * 256 / NUM_LEDS * width_factor) + gHue, 255, 255);
    leds[i].w = luma+5;
  }
  EVERY_N_MILLISECONDS( 200 ) { gHue++; } 
  noSides();
}

void dimWhiteNumber() {
  for(uint8_t i = geom_bottom_center[0]; i < geom_bottom_center[1]; i++){
    leds[i].w = 5;
  }
}

void noSides() {
  for(uint8_t i = geom_side_left[0]; i < geom_side_left[1]; i++){
    leds[i] = 0;
  }
  for(uint8_t i = geom_side_right[0]; i < geom_side_right[1]; i++){
    leds[i] = 0;
  }
}

void fillWWhite(){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].w = luma;
  }
  noSides();
}
void blackoutW() {
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].w = 0;
  }
}

void blackoutRGB() {
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].r = 0;
    leds[i].g = 0;
    leds[i].b = 0;
  }
}
