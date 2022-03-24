#include <avr/power.h>
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif
#include "FastLED.h"
#include <EEPROM.h>

#define NUM_LEDS 2
#define DATA_PIN 6
CRGB leds[NUM_LEDS];
#define MAX_LUMA 140
#define AWAY_LUMA 100

uint8_t pattern_step = 0;
uint8_t last_rx_value;
bool i_am_home = false;
bool i_was_home_last_tick = false;
bool last_rx_succeeded = false;
#define RADIO_PIN 8

RH_ASK driver(2000, 9, 5, true); // rx tx ptt ptt_invert // 328p-au

unsigned long rx_poll_clk_start; 
unsigned long rx_poll_clk_now;
const unsigned long rx_poll_clk_period = 1000; // was: 50

unsigned long pattern_step_clk_start; 
unsigned long pattern_step_clk_now;
// controls speed of rainbow. must be in sync with transmitter
const unsigned long pattern_step_clk_period = 400; 

unsigned long max_home_radio_silence_clk_start; 
unsigned long max_home_radio_silence_clk_now;
const unsigned long max_home_radio_silence_clk_period = 45000;

uint8_t away_hue_base = 55; // will be overwritten initially by random value

void setup() {
  Serial.begin(115200);

  set_away_hue();
  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  // capture start times for all the clocks
  rx_poll_clk_start = millis(); 
  pattern_step_clk_start = millis(); 
  max_home_radio_silence_clk_start = millis(); 

  // disable features to save power
  //power_adc_disable(); // should be uncommented
  //power_spi_disable();
  //power_usi_disable();
  //power_timer2_disable();
  //power_usart0_disable();

  digitalWrite(RADIO_PIN, LOW);
  pinMode(RADIO_PIN, OUTPUT); // radio enable pin
}

void set_away_hue() {
  /*
   * Return a hue value to use for the away pattern if we've never been home.
   * Make sure its a bit different from the previous value.
   */
  unsigned int away_hue_eeAddress = 0; 
  uint8_t away_hue_base_min_delta = 40; // how similar can the colors be
  // init random seed and capture random value 
  power_adc_enable();
  randomSeed(analogRead(A1)+analogRead(A2));
  uint8_t away_hue_base_candidate = random(255);
  power_adc_disable();
  /*Serial.print("away hue candidate: ");
  Serial.print(away_hue_base_candidate);*/
  // look up the last value that was used as base
  uint8_t away_hue_base_last;
  EEPROM.get( away_hue_eeAddress, away_hue_base_last );
  /*Serial.print(" last: ");
  Serial.print(away_hue_base_last);*/
  // if the current value and last value are too similar, change the candidate
  if ( abs(away_hue_base_candidate - away_hue_base_last) < away_hue_base_min_delta ) {
    // make sure its within 0-255
    away_hue_base = away_hue_base_candidate + away_hue_base_min_delta < 256 ? away_hue_base_candidate + away_hue_base_min_delta : away_hue_base_candidate - away_hue_base_min_delta;
    /*Serial.print(" - too similar! Will use: ");*/
  } else {
    away_hue_base = away_hue_base_candidate;
    /*Serial.print(" - using as-is: ");*/
  }
  // record new hue selection for reference next boot
  /*Serial.println(away_hue_base);*/
  EEPROM.put( away_hue_eeAddress, away_hue_base );
} // set_away_hue

void set_radio_state(bool state) {
  /*
   * Turn SYN480R radio on (state=1) or off (state=0)
   */
  // pull RADIO_PIN low to sink current and enable radio
  if (state) { digitalWrite(RADIO_PIN, LOW); }
  else { digitalWrite(RADIO_PIN, HIGH); }
}

void display_step_as_hue(uint8_t value) {
  fill_solid(leds, NUM_LEDS, CHSV( pattern_step, 255, value ));
  FastLED.show();
}

void display_step_as_value(uint8_t hue) {
  fill_solid(leds, NUM_LEDS, CHSV( hue, 255, pattern_step ));
  FastLED.show();
}

void display_hue(uint8_t hue) {
  fill_solid(leds, NUM_LEDS, CHSV( hue, 255, 100 ));
  FastLED.show();
}

void lonely() {
  // the away from home pattern
  if( i_was_home_last_tick ) {
    // capture current step to use as hue base if we just left home
    // if we were never home, the random value assigned in set_away_hue() is used
    away_hue_base = pattern_step;
    Serial.print("capturing away hue center ");
    Serial.println(away_hue_base);
  }
  uint8_t wave_pos = triwave8(pattern_step);
  uint8_t amplitude = 30;
  uint8_t delta = map(wave_pos, 0, 254, 0, 20);
  /*Serial.print("delta ");
  Serial.print(delta);
  Serial.print(" hue ");
  Serial.println(away_hue_base+delta);  */
  fill_solid(leds, NUM_LEDS, CHSV( away_hue_base+delta, 200, AWAY_LUMA ));
  FastLED.show();
} // lonely

void ack_return_home() {
  /*
   * quick rainbow pattern to acknowledge return home
   * make non-blocking? meh
   */
  /*Serial.println("returned home!");
  Serial.println(away_hue_base);*/
  uint8_t timing = 1;
  uint8_t rh_luma = MAX_LUMA;
  for (uint8_t i = away_hue_base; i < 255; i++) {
    fill_solid(leds, NUM_LEDS, CHSV( i, 255, rh_luma ));
    FastLED.show();
    delayMicroseconds(timing);
  }
  for (uint8_t i = 0; i < 255; i++) {
    fill_solid(leds, NUM_LEDS, CHSV( i, 255, rh_luma ));
    FastLED.show();
    delayMicroseconds(timing);
  }
  for (uint8_t i = 0; i < 255; i++) {
    fill_solid(leds, NUM_LEDS, CHSV( i, 255, rh_luma ));
    FastLED.show();
    delayMicroseconds(timing);
  }
  for (uint8_t i = 0; i < pattern_step; i++) {
    fill_solid(leds, NUM_LEDS, CHSV( i, 255, rh_luma ));
    FastLED.show();
    delayMicroseconds(timing);
  }
} // ack_return_home

bool try_rx() {
  /*
   * Attempt to receive a beacon.
   * Returns true if successful.
   */
  set_radio_state(1);
  // sleep while we wait for the radio to initilize 
  delay(80); // 52 seems to be the minimum here, 60 gives some tolerance
  bool rx_successful = false;
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  if (driver.recv(buf, &buflen)) {
    // if the first 3 bytes are "SRF", its a good message
    if( buf[0] == 0x53 && buf[1] == 0x52 && buf[2] == 0x46 ) {
      last_rx_value = buf[3];
      rx_successful = true;
    }
  }
  set_radio_state(0);
  return rx_successful;
} // try_rx

void loop() {
 
  rx_poll_clk_now = millis(); 
  pattern_step_clk_now = millis(); 
  max_home_radio_silence_clk_now = millis(); 

  // if it's time to poll the radio, do so
  if (rx_poll_clk_now - rx_poll_clk_start >= rx_poll_clk_period) {

    if( try_rx() ) {
      i_am_home = true;
      last_rx_succeeded = true;
      //Serial.println("+++ RX SUCCESS +++");
      max_home_radio_silence_clk_start = rx_poll_clk_now; // reset silence timer
    } else {
      last_rx_succeeded = false;
      //Serial.println("--- RX FAIL ---");
    }
    rx_poll_clk_start = rx_poll_clk_now; // reset poll timer
  } // radio poll ticks

  // check if the max radio silence timeout has been exceeded
  if (max_home_radio_silence_clk_now - max_home_radio_silence_clk_start >= max_home_radio_silence_clk_period) {
    // radio silence timeout exceeded
    i_am_home = false;
  } // radio silence ticks

  // pattern animation ticks
  if (pattern_step_clk_now - pattern_step_clk_start >= pattern_step_clk_period) {
    /*Serial.print("home: ");
    Serial.print(i_am_home);
    Serial.print("; was home last: ");
    Serial.println(i_was_home_last_tick);
    Serial.print("pattern step ");
    Serial.println(pattern_step);*/
    if (last_rx_succeeded) {
      //pattern_step_wheel((pattern_step - last_rx_value)/2);
      pattern_step = last_rx_value;
    }
    if (i_am_home) {
      FastLED.setBrightness(MAX_LUMA);
      if( i_was_home_last_tick == false ) {
        ack_return_home();
      }
      display_step_as_hue(100);
    } else { // not home
      lonely();
    } 
    i_was_home_last_tick = i_am_home;
    pattern_step = pattern_step < 255 ? pattern_step + 1 : 0;
    pattern_step_clk_start = pattern_step_clk_now; // reset step timer
  } // pattern ticks

} // loop
