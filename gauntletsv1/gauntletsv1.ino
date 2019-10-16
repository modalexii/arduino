/* FastLED RGBW Example Sketch
 *
 * Example sketch using FastLED for RGBW strips (SK6812). Includes
 * color wipes and rainbow pattern.
 *
 * Written by David Madison
 * http://partsnotincluded.com
*/

#include "FastLED.h"
#include "FastLED_RGBW.h"

#define NUM_LEDS 12+13+12
#define DATA_PIN 6
#define FSR_PIN A0

const int button_threshold = 75;

// Board setting: Arduino Leonardo

uint8_t gPos255 = 0; // global step position 0-255
uint8_t mode = 0; // track which pattern we're on

CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];

const uint8_t brightness = 15;

const uint8_t curve[2 * (12+13+12) + 13 ] = {
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,
    25,45,65,95,115,135,115,95,65,45,25,20,15,
    0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,
    };

void setup() { 
  pinMode(A0, INPUT);
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  FastLED.setBrightness(brightness);
  FastLED.show();
  Serial.begin(115200);
}

void loop(){
  //fillWhite( max( 0, -1*abs(gPos255-125)+(100) ) );
  Serial.print("loop start! pattern in: ");
  Serial.println(mode);
  if (checkButtonPress() == true) {
    Serial.print("button! pattern in: ");
    Serial.println(mode);
    if ( mode == 0 ) {
      mode++;
    }
    else if (mode == 1) {
      mode--;
    }
    Serial.print("pattern out: ");
    Serial.println(mode);
  }
  if (mode == 0) {
    Serial.println("pattern was 0");
    P1RainbowWhiteWave();
  }
  if (mode == 1) {
    Serial.println("pattern was 1");
    P2RainbowCycle();
  }
  gPos255 = gPos255 < 256 ? gPos255 + 1 : 0;  
}

bool checkButtonPress() {
  bool press = false;
  // visually acknowledge the press and wait for it to stop
  while (analogRead(FSR_PIN) > button_threshold) {
    press = true;
    customClear();
    fillAqua(100);
    FastLED.show();
    delay(200);
  }
  return press;
}

void P1RainbowWhiteWave() {
  rainbow(gPos255,2);
  whitePulse(gPos255);
  FastLED.show();
  delay(30);
}

void P2RainbowCycle() {
  colorFill(gPos255);
  FastLED.show();
  delay(60);
}

void colorFill(uint8_t hue){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CHSV(hue, 255, 255);
  }
}

void whitePulse(uint8_t start_position) {
  //Serial.print("start_position=");
  //Serial.print(start_position);
  //Serial.print("; ");
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].w = curve[((start_position + i) % (14*5))];
    //Serial.print((start_position + i) % (14*5));
    //Serial.print(", ");
  }
  //Serial.println();
}

void fillWhite(uint8_t luma){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].w = luma;
  }
}

void fillAqua(uint8_t luma){
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].b = luma;
    leds[i].g = luma;
  }
}

void customClear() {
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = 0;
  }
}

void rainbow(uint8_t start_position, float width_factor){
  // lower width factor = spread rainbow farther, higher = fit more rainbows
  // good values are 0.2 -> 2
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CHSV((i * 256 / NUM_LEDS * width_factor) + start_position, 255, 255);
  }
}


/*
void rainbowLoop(){
  long millisIn = millis();
  long loopTime = 5000; // 5 seconds

  while(millis() < millisIn + loopTime){
    rainbow();
    delay(5);
  }
}*/
