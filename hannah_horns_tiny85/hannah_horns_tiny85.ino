/*
 * 8MHz Internal T85
 */
#include <SystemStatus.h>
#include <Bounce2.h>
#include <FastLED.h>

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#define adc_disable() (ADCSRA &= ~(1<<ADEN)) 
#define adc_enable() (ADCSRA |= (1 << ADEN))

#define SSR_PIN 4
#define NUM_STRIPS 2
#define NUM_LEDS_PER_STRIP 26
#define BRIGHTNESS 100
#define FRAMES_PER_SECOND 60
CRGB leds[NUM_STRIPS][NUM_LEDS_PER_STRIP];
uint8_t gHue = 0;

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define BUTTON_PIN 3
Bounce2::Button button = Bounce2::Button();


void setup() {
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, HIGH);
  pinMode(2, INPUT_PULLUP);
  FastLED.addLeds<NEOPIXEL, 0>(leds[0], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 1>(leds[1], NUM_LEDS_PER_STRIP);
  FastLED.setBrightness(BRIGHTNESS);
  button.attach( BUTTON_PIN, INPUT_PULLUP );
  button.interval(100);
  button.setPressedState(LOW); 
  adc_disable();
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { showBatteryIndicator, do_nothing, dimmer_grad, brighter_grad, off };
uint8_t gCurrentPatternNumber = 0;

void loop() {
  /*fill_rainbow( leds[0], NUM_LEDS_PER_STRIP, gHue, 3);
  fill_rainbow( leds[1], NUM_LEDS_PER_STRIP, gHue, 3);*/
  //FastLED.delay(1000/FRAMES_PER_SECOND);
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } 
  button.update();
  //if ( button.rose() || button.fell() ) { nextPattern(); }
  if ( button.changed() ) { nextPattern(); }
  gPatterns[gCurrentPatternNumber]();
  FastLED.show();
  FastLED.delay(1000/FRAMES_PER_SECOND);
}

void nextPattern() {
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
  FastLED.setBrightness(BRIGHTNESS);
}

void off() {
  FastLED.clear();
  digitalWrite(SSR_PIN, LOW);
  //sleepNow();
  system_sleep(); 
  digitalWrite(SSR_PIN, HIGH);
  nextPattern();
}

void showBatteryIndicator() {
  uint8_t status_hue = 0;
  uint16_t vcc = SystemStatus().getVCC();
  if (vcc > 2700) { status_hue = 100; }
  else if (vcc < 2200) { status_hue = 0; }
  else { status_hue = map(SystemStatus().getVCC(), 2200, 2700, 0, 100); }

  for(uint8_t i = 0; i < 180; i++) {
    fill_solid(leds[0], NUM_LEDS_PER_STRIP, CHSV(status_hue,255,i));
    fill_solid(leds[1], NUM_LEDS_PER_STRIP, CHSV(status_hue,255,i));
    FastLED.show();
  }
  FastLED.delay(800);
  nextPattern();
}

void dimmer_grad() {
  FastLED.setBrightness(BRIGHTNESS-10);
  fill_grad();
}

void brighter_grad() {
  FastLED.setBrightness(BRIGHTNESS+80);
  fill_grad();
}

void fill(uint8_t hue) {
  fill_solid(leds[0], NUM_LEDS_PER_STRIP, CHSV(hue,255,255));
  fill_solid(leds[1], NUM_LEDS_PER_STRIP, CHSV(hue,255,255));
}

void do_nothing() {
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

// From http://interface.khm.de/index.php/lab/experiments/sleep_watchdog_battery/
void system_sleep() {
  GIMSK |= _BV(PCIE);                     // Enable Pin Change Interrupts
  PCMSK |= _BV(PCINT3);                   // sense changes in PCINT2 = PB2
  ADCSRA &= ~_BV(ADEN);                   // ADC off
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // replaces above statement

  sleep_enable();                         // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();                                  // Enable interrupts
  sleep_cpu();                            // sleep

  cli();                                  // Disable interrupts
  PCMSK &= ~_BV(PCINT3);                  // Turn off INT0 (PB2) as interrupt pin
  sleep_disable();                        // Clear SE bit
  //ADCSRA |= _BV(ADEN);                    // ADC on

  sei();                                  // Enable interrupts  
}

ISR(PCINT0_vect) {
}
