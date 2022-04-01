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

// how many hue steps to advance each pattern_tick_period when "chasing"
// to catch up from the current H value to the last rx value
const uint8_t chase_steps = 10;
uint8_t pattern_step = 0; // used in away pattern only
const uint8_t pattern_tick_period = 10; // ms how frequently to draw new frame

uint8_t away_hue_base = 160; // will be overwritten initially by random value

// RH_ASK driver(2000, 4, 5, true); // rx tx ptt ptt_invert // NodeMCU
RH_ASK driver(2000, 9, 5, true); // rx tx ptt ptt_invert // 328p-au

uint8_t current_hue;
uint8_t last_rx_value;
unsigned long last_rx_time; 
const unsigned long rx_poll_period = 500; // was: 50
const uint16_t rx_retry_count = 10000; // large # to ensure the radio chip has time to wake up

const unsigned long max_home_rx_silence_period = 1000*30; // how long til we're decidedly away from home

uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
uint8_t buflen = sizeof(buf);

#define RADIO_EN_PIN 8

void setup() {
  Serial.begin(115200); // needed!
  driver.init();
  set_away_hue();

  // disable features to save power
  power_adc_disable();
  digitalWrite(RADIO_EN_PIN, LOW);
  pinMode(RADIO_EN_PIN, OUTPUT); 
  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(3.5, 25); 
  FastLED.clear();
  FastLED.show(); 

  try_rx();
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

void chase_last_rx() {
  CHSV new_hue = blend(CHSV(current_hue,255,255), CHSV(last_rx_value,255,255), chase_steps, SHORTEST_HUES);
  leds[0] = new_hue;
  leds[1] = new_hue;
  current_hue = new_hue.hue;
}

void lonely() {
  // the away from home pattern
  uint8_t wave_pos = triwave8(pattern_step);
  uint8_t delta = map(wave_pos, 0, 254, 0, 20);
  /*Serial.print("delta ");
  Serial.print(delta);
  Serial.print(" hue ");
  Serial.println(away_hue_base+delta);  */
  fill_solid(leds, NUM_LEDS, CHSV( min(away_hue_base+delta,255), 255, 255 ));
} // lonely

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { lonely, chase_last_rx };
uint8_t gCurrentPatternNumber = 0;

void set_radio_state(bool state) {
  /*
   * Turn SYN480R radio on (state=1) or off (state=0)
   */
  // pull RADIO_PIN low to sink current and enable radio
  if (state) { digitalWrite(RADIO_EN_PIN, LOW); }
  else { digitalWrite(RADIO_EN_PIN, HIGH); }
}

bool try_rx() {
  /*
   * Attempt to receive a beacon.
   * Returns true if successful.
   */
  set_radio_state(1);
  // sleep while we wait for the radio to initilize 
  //delay(80); // 52 seems to be the minimum here, 60 gives some tolerance // trying disabled in hopes that high rx_retry_count gets an rx at the earliest possible moment
  bool rx_successful = false;
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  for (uint16_t i = 0; i < rx_retry_count; i++) { // retry rx a few times while the chip is awake
    if (driver.recv(buf, &buflen)) {
      // if the first 3 bytes are "SRF", its a good message
      if( buf[0] == 0x53 && buf[1] == 0x52 && buf[2] == 0x46 ) {
        last_rx_value = buf[3];
        last_rx_time = millis();
        away_hue_base = last_rx_value; // if we leave home, preserve the last value
        rx_successful = true;
        break;
      }
    }
  }
  set_radio_state(0);
  return rx_successful;
} // try_rx

void loop() {
  // radio polls
  EVERY_N_MILLISECONDS(rx_poll_period){
    try_rx();
  }

  EVERY_N_MILLISECONDS(pattern_tick_period){
    // pattern animation
    //fill_solid(leds,NUM_LEDS,CHSV(last_rx_value,255,255));
    // decide if we are away from home or not
    if ( millis() - last_rx_time < max_home_rx_silence_period && last_rx_time > 0 ){
      //gCurrentPatternNumber = 1; // we are home
    } 
    else {
      gCurrentPatternNumber = 0; // away
    }
    gPatterns[gCurrentPatternNumber]();
    FastLED.show();
    pattern_step = pattern_step < 255 ? pattern_step + 1 : 0; // used by lonely()
  }
  
} // loop