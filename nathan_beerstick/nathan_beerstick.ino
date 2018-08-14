
#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

#define DATA_PIN_A 4
#define DATA_PIN_B 5
#define DATA_PIN_C 6
#define LED_TYPE WS2813
#define COLOR_ORDER GRB
#define LEDS_PER_STRIP 120
#define UNPLAYABLE_LENGTH 18*2
#define MAX_LUMA 200
#define WIN_DETECT_THRESHOLD 650
#define IR_SENSOR_PIN A3

CRGB leds[LEDS_PER_STRIP];
ClickEncoder *encoder;
int16_t last, value;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void timerIsr() {
  encoder->service();
}

void waitForReset() {
  uint8_t hueSkip = 5;
  Serial.println("waiting for players to place the can");
  gHue = 0;
  fill_solid(leds, LEDS_PER_STRIP, CHSV(0,0,0));
  while (analogRead(IR_SENSOR_PIN) < WIN_DETECT_THRESHOLD) {
    for (uint8_t px = LEDS_PER_STRIP - 4; px < LEDS_PER_STRIP; px++) {
      leds[px] = CHSV(gHue,255,MAX_LUMA);
    }
    FastLED.show();
    delay(250);
    fill_solid(leds, LEDS_PER_STRIP, CHSV(0,0,0));
    FastLED.show();
    delay(250);
    gHue = gHue < 256 - hueSkip ? gHue + hueSkip : 0;
  }
}

void setup() {

  Serial.begin(115200);

  // create & init  encoder object
  encoder = new ClickEncoder(A1, A0, A2);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  last = -1;
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN_A,COLOR_ORDER>(leds, LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_B,COLOR_ORDER>(leds, LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_C,COLOR_ORDER>(leds, LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(MAX_LUMA);
  
  // IR Sensor
  pinMode(A0,INPUT);
  
  // so we dont appear to win immediately on power up
  waitForReset();
  
}

void writeDisplay() {
  // a wrapper for show()
  // overwrite the unplayable space
  for (uint8_t px = 0; px < UNPLAYABLE_LENGTH; px++) {
    leds[px] = CRGB::Red;
  }
  FastLED.show();
}

void drawBug(uint16_t pos, uint8_t len, int turn) {
  pos = pos % LEDS_PER_STRIP;
  if (pos % turn == 0) {
    for (uint8_t i = pos; i < pos + len; i++) {
      if (i < LEDS_PER_STRIP + 1) {
        leds[i] = CRGB::White;
      }
    }
  }
}

void playWinShow() {
  
  Serial.println("win");
  int NUM_SPECTRA = 3;
  int CYCLES = 2;
  int WRITE_SPEED = 1; // higher makes the cycle faster
  int HUE_SKIP = 5; // higher makes the cycle appear faster
  int BUG_LEN = 5;
  uint16_t BUG_POS = 0;
  gHue = 0;
  for (int i = 0; i < CYCLES * 255; i++) {
    fill_rainbow(leds, LEDS_PER_STRIP, gHue, NUM_SPECTRA);
    drawBug(BUG_POS, BUG_LEN, 2);
    FastLED.show();
    delay(WRITE_SPEED);
    BUG_POS++; // move bug up the line
    gHue += HUE_SKIP; // rotate base color
  }
  
}

bool gameWon() {
  return analogRead(IR_SENSOR_PIN) < WIN_DETECT_THRESHOLD ? true : false;
}

void loop() {
  
  fill_solid(leds, LEDS_PER_STRIP, CHSV(gHue,255,MAX_LUMA));
  writeDisplay();
  gHue = gHue < 256 ? gHue + 1 : 0;

  if (gameWon()) { 
    playWinShow(); 
    waitForReset();
  }
  else { 
    Serial.println("no win :-("); 
  }

  delay(100);

}
