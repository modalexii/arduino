
#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>
#include <EEPROM.h>

#define DATA_PIN_A 4
#define DATA_PIN_B 5
#define DATA_PIN_C 6
#define LED_TYPE WS2813
#define COLOR_ORDER GRB
#define LEDS_PER_STRIP 108
#define UNPLAYABLE_LENGTH 18*2
int MAX_LUMA = 200; // will be overwritten if user value saved to eeprom
int WIN_DETECT_THRESHOLD = 650; // will be overwritten if user value saved to eeprom
#define IR_SENSOR_PIN A3
#define ENCODER_BUTTON_PIN A2
#define ENCODER_INC_PIN A0
#define ENCODER_DEC_PIN A1

CRGB leds[LEDS_PER_STRIP];
ClickEncoder *encoder;
int16_t last, value, difference;

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void saveSettings() {
  // brightness
  unsigned int eeAddress = 0; 
  EEPROM.put( eeAddress, MAX_LUMA );
  // sensitivity
  eeAddress += sizeof(int);
  EEPROM.put( eeAddress, WIN_DETECT_THRESHOLD );
}

void loadSettings() {
  unsigned int eeAddress = 0; 
  // brightness
  int candidateMAX_LUMA;
  EEPROM.get( eeAddress, candidateMAX_LUMA );
  // sanity check
  MAX_LUMA = candidateMAX_LUMA >= 0 && candidateMAX_LUMA <= 255 ? candidateMAX_LUMA : MAX_LUMA;
  // sensitivity
  eeAddress += sizeof(int);
  int candidateWIN_DETECT_THRESHOLD;
  EEPROM.get( eeAddress, candidateWIN_DETECT_THRESHOLD );
  // sanity check
  WIN_DETECT_THRESHOLD = candidateWIN_DETECT_THRESHOLD >= 250 && candidateWIN_DETECT_THRESHOLD <= 1200 ? candidateWIN_DETECT_THRESHOLD : WIN_DETECT_THRESHOLD;
}

void timerIsr() {
  encoder->service();
}

void setBrightnessLoop() {
  ClickEncoder::Button b;
  fill_solid(leds, LEDS_PER_STRIP, CHSV(120, 255, MAX_LUMA));
  FastLED.show();
  value = 0;
  while (true) {
    value += encoder->getValue();
    if (value != last) {
      difference = 4 * (value - last);
      last = value;
      MAX_LUMA = min(220, MAX_LUMA + difference);
      MAX_LUMA = max(40, MAX_LUMA);
      Serial.print("new MAX_LUMA: ");
      Serial.println(MAX_LUMA);
      fill_solid(leds, LEDS_PER_STRIP, CHSV(120, 255, MAX_LUMA));
      FastLED.show();
    }
    b = encoder->getButton();
    if (b != ClickEncoder::Open) {
      fill_solid(leds, LEDS_PER_STRIP, CHSV(0,0,0));
      FastLED.show();
      last = -1;
      saveSettings();
      return;
    }
  }
}

void setSensitivityLoop() {
  int thresholdMax = 1200;
  int thresholdMin = 250;
  int thresholdMiddle = 650; // a recommended starting place, not actually the range center
  ClickEncoder::Button b;
  int quarterStrip = LEDS_PER_STRIP / 4;
  uint8_t indicatorColor = 0;
  value = 0;
  while (true) {
    // draw interface canvas
    fill_solid(leds, LEDS_PER_STRIP, CHSV(0,0,0));
    leds[quarterStrip] = CHSV(0, 255, MAX_LUMA); // red dot
    leds[quarterStrip * 2] = CHSV(95, 255, MAX_LUMA); // green dot
    leds[LEDS_PER_STRIP - quarterStrip] = CHSV(160, 255, MAX_LUMA); // blue dot
    // update display
    if (WIN_DETECT_THRESHOLD > thresholdMiddle) {
      int graphPeak = map(WIN_DETECT_THRESHOLD, thresholdMiddle, thresholdMax, quarterStrip * 2, LEDS_PER_STRIP - quarterStrip);
      for (uint8_t px = quarterStrip * 2 + 1; px < graphPeak; px++) {
        leds[px] = CHSV(160, 255, MAX_LUMA);
      }
    } else if (WIN_DETECT_THRESHOLD < thresholdMiddle) {
      int graphTroph = map(WIN_DETECT_THRESHOLD, thresholdMin, thresholdMiddle, quarterStrip, quarterStrip * 2);
      for (uint8_t px = graphTroph; px < quarterStrip * 2; px++) {
        leds[px] = CHSV(0, 255, MAX_LUMA);
      }
    }
    // update can placement indicator
    if (gameWon()) { 
      indicatorColor = 95; // green
    } else {
      indicatorColor = 0; // red
    }
    for (uint8_t px = LEDS_PER_STRIP - 3; px < LEDS_PER_STRIP; px++) {
      leds[px] = CHSV(indicatorColor, 255, MAX_LUMA); // red
    }
    FastLED.show();
    // check new values and update interface for the next loop
    value += encoder->getValue();
    if (value != last) {
      difference = 4 * (value - last);
      last = value;
      WIN_DETECT_THRESHOLD += difference;
      WIN_DETECT_THRESHOLD = max(WIN_DETECT_THRESHOLD, thresholdMin);
      WIN_DETECT_THRESHOLD = min(WIN_DETECT_THRESHOLD, thresholdMax);
      Serial.print("New WIN_DETECT_THRESHOLD: ");
      Serial.println(WIN_DETECT_THRESHOLD);
    }  
    b = encoder->getButton();
    if (b != ClickEncoder::Open) {
      fill_solid(leds, LEDS_PER_STRIP, CHSV(0,0,0));
      FastLED.show();
      last = -1;
      saveSettings();
      return;
    }
  }
}

void checkSettingsRequest() {

   // check encoder button state
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    Serial.println("Entering settings");
    // first thing - brightness
    setBrightnessLoop();
    // second thing - sensitivity
    setSensitivityLoop();
  }
  
}

void waitForReset() {
  int blinkDelay = 500;
  Serial.println("waiting for players to place the can");
  fill_solid(leds, LEDS_PER_STRIP, CHSV(0,0,0));
  while (analogRead(IR_SENSOR_PIN) < WIN_DETECT_THRESHOLD) {
    for (uint8_t px = LEDS_PER_STRIP - 3; px < LEDS_PER_STRIP; px++) {
      leds[px] = CHSV(0, 255, MAX_LUMA); // red
    }
    FastLED.show();
    delay(blinkDelay);
    fill_solid(leds, LEDS_PER_STRIP, CHSV(0,0,0));
    FastLED.show();
    delay(blinkDelay);
    checkSettingsRequest();
  }
  // for easing, fill out the strip quickly but perceptably 
 for (uint8_t px = LEDS_PER_STRIP; px > UNPLAYABLE_LENGTH; px--) {
  leds[px] = CHSV(0,255,MAX_LUMA); // red
  writeDisplay();
  delay(1);
 }
}

void setup() {

  Serial.begin(115200);

  // create & init  encoder object
  encoder = new ClickEncoder(ENCODER_DEC_PIN, ENCODER_INC_PIN, ENCODER_BUTTON_PIN);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  last = -1;
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN_A,COLOR_ORDER>(leds, LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_B,COLOR_ORDER>(leds, LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN_C,COLOR_ORDER>(leds, LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(MAX_LUMA);
  
  // IR Sensor
  pinMode(A3,INPUT);

  loadSettings();
  
  // so we dont appear to win immediately on power up
  waitForReset();
  
}

void writeDisplay() {
  // a wrapper for show()
  // overwrite the unplayable space
  for (uint8_t px = 0; px < UNPLAYABLE_LENGTH; px++) {
    leds[px] = CHSV(0,255,max(MAX_LUMA-80,10));
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
  int fade = 0;
  for (int i = 0; i < CYCLES * 255; i++) {
    fill_rainbow(leds, LEDS_PER_STRIP, gHue, NUM_SPECTRA);
    drawBug(BUG_POS, BUG_LEN, 2);
    FastLED.setBrightness( dim8_video( max(MAX_LUMA - fade, 0) ) );
    FastLED.show();
    delay(WRITE_SPEED);
    BUG_POS++; // move bug up the line
    gHue += HUE_SKIP; // rotate base color
    // ease out
    if (i > (CYCLES * 255) - 85) { // last 85 iterations
      fade += 3;
      Serial.print("easing fade: "); Serial.println(fade);
    }
  }
  FastLED.setBrightness(0);
  FastLED.show();
  delay(500);
  FastLED.setBrightness(MAX_LUMA);
  // check button state and do nothing with the value, to clear it
  // this prevents clicks during the win pattern from dropping us in 
  // to settings after the pattern concludes, which feels weird
  ClickEncoder::Button b = encoder->getButton();
  
}

bool gameWon() {
  return analogRead(IR_SENSOR_PIN) < WIN_DETECT_THRESHOLD ? true : false;
}

void loop() {

  checkSettingsRequest();
  
  fill_solid(leds, LEDS_PER_STRIP, CHSV(gHue,255,MAX_LUMA));
  writeDisplay();
  gHue = gHue < 256 ? gHue + 1 : 0;

  if (gameWon()) { 
    playWinShow(); 
    waitForReset();
    gHue = 0;
  }
  else { 
    Serial.println("no win :-("); 
  }

  delay(100);

}
