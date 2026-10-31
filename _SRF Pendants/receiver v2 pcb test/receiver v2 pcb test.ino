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
CRGB pxbuf[NUM_LEDS]; // holder for rx'd values tha we're not ready to write to LEDs yet

const uint8_t chase_steps = 10; // how many hue steps to advance each pattern_tick_period when "chasing" to catch up 
uint8_t pattern_step = 0; // used in away pattern only
const uint8_t pattern_tick_period = 10; // ms how frequently to draw new frame

uint8_t away_hue_base = 160; // will be overwritten initially by random value

// RH_ASK driver(2000, 4, 5, true); // rx tx ptt ptt_invert // NodeMCU
RH_ASK driver(2000, 9, 5, true); // rx tx ptt ptt_invert // 328p-au

unsigned long last_rx_time; 
const unsigned long rx_poll_period = 500; // how often (ms) between radio polls
const uint16_t rx_retry_count = 10000; // poll retries - large # to ensure the radio chip has time to wake up

const unsigned long max_home_rx_silence_period = 1000*30; // how long (ms) til we're decidedly away from home

uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
uint8_t buflen = sizeof(buf);
#define RADIO_EN_PIN 8

void setup() {
  Serial.begin(115200); // needed!
  driver.init(); // ASK library
  set_away_hue();

  power_adc_disable(); // disable ADC to save power

  // turn radio chip off
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
   * Set a hue value to use for the away pattern if we've never been home.
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
  /*
   * Nudge the led hues towards the last recieved values in at most `chase_steps` chunks
   * If we show rx'd values on the LEDs right away, they appear to jump since we only check the radio
   * every rx_poll_period and may miss polls due to weak signal. It's much nicer to blend towards where 
   * we need to be. chase_steps and pattern_tick_period together need to balance "getting to the new
   * color quickly" versus "not jumping colors unappealingly"
   */
  leds[0] = blend(leds[0], pxbuf[0], chase_steps);
  leds[1] = blend(leds[1], pxbuf[1], chase_steps);
}

void lonely() {
  // the away from home pattern - uses HSV for simplicity
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
  // rather than actually wait for the chip to wake up, we just retry the RX a ton of times (rx_retry_count)
  bool rx_successful = false;
  //uint8_t buf[9];//[RH_ASK_MAX_MESSAGE_LEN];
  //uint8_t buflen = sizeof(buf);
  for (uint16_t i = 0; i < rx_retry_count; i++) { // retry rx a few times while the chip is awake
    if (driver.recv(buf, &buflen)) {
      // if the first 3 bytes are "SRF", its a good message
      if( buf[0] == 0x53 && buf[1] == 0x52 && buf[2] == 0x46 ) {
        pxbuf[0].setRGB( buf[3], buf[4], buf[5]);        
        pxbuf[1].setRGB( buf[6], buf[7], buf[8]);
        last_rx_time = millis();
        away_hue_base = rgb2hsv_approximate(pxbuf[0]).hue; // if we leave home, preserve the last value
        rx_successful = true;
        break;
      }
    }
  }
  set_radio_state(0);
  return rx_successful;
} // try_rx

void loop() {

 pinMode(3, INPUT_PULLUP);
 if(digitalRead(3) == HIGH) { leds[0] = CHSV(160,255,150); }
 else if((digitalRead(3) == LOW)){ leds[0] = CHSV(0,255,150); }
 FastLED.show();
 delay(100);
  
} // loop