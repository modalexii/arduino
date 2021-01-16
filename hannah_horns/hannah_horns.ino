#include <Bounce2.h>
#include "FastLED.h"
FASTLED_USING_NAMESPACE

#define NUM_STRIPS 2
#define NUM_LEDS_PER_STRIP 26
#define BRIGHTNESS 100
#define FRAMES_PER_SECOND 60
CRGB leds[NUM_STRIPS][NUM_LEDS_PER_STRIP];
uint8_t gHue = 0;

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define BUTTON_PIN 14
Button button = Button();

void setup() {
  delay(500); 
  FastLED.addLeds<NEOPIXEL, 5>(leds[0], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 6>(leds[1], NUM_LEDS_PER_STRIP);
  FastLED.setBrightness(BRIGHTNESS);
  button.attach( BUTTON_PIN, INPUT_PULLUP );
  button.interval(5);
  button.setPressedState(LOW); 
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { off, dimmer_grad, brighter_grad };
uint8_t gCurrentPatternNumber = 1;

void loop() {
  /*fill_rainbow( leds[0], NUM_LEDS_PER_STRIP, gHue, 3);
  fill_rainbow( leds[1], NUM_LEDS_PER_STRIP, gHue, 3);
  FastLED.show();  
  FastLED.delay(1000/FRAMES_PER_SECOND);
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } */
  gPatterns[gCurrentPatternNumber]();
  FastLED.show();
  FastLED.delay(1000/FRAMES_PER_SECOND);
  button.update();
  if (button.pressed()) { nextPattern(); }
}

void nextPattern() {
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void off() {
  fill_solid(leds[0], NUM_LEDS_PER_STRIP, 0);
  fill_solid(leds[1], NUM_LEDS_PER_STRIP, 0);
}

void dimmer_grad() {
  FastLED.setBrightness(90);
  fill_grad();
}

void brighter_grad() {
  FastLED.setBrightness(180);
  fill_grad();
}

void fill_grad() {
  uint8_t starthue = beatsin8(1, 0, 255);
  uint8_t endhue = beatsin8(3, 0, 255);
  if (starthue < endhue) {
    fill_gradient(leds[0], NUM_LEDS_PER_STRIP, CHSV(starthue,255,255), CHSV(endhue,255,255), FORWARD_HUES);
    fill_gradient(leds[1], NUM_LEDS_PER_STRIP, CHSV(starthue,255,255), CHSV(endhue,255,255), FORWARD_HUES);
  } else {
    fill_gradient(leds[0], NUM_LEDS_PER_STRIP, CHSV(starthue,255,255), CHSV(endhue,255,255), BACKWARD_HUES);
    fill_gradient(leds[1], NUM_LEDS_PER_STRIP, CHSV(starthue,255,255), CHSV(endhue,255,255), BACKWARD_HUES);
  }
} // fill_grad()
