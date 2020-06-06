#include <VirtualWire.h>
#include "FastLED.h"

const int transmit_pin = 12;
const int receive_pin = 8;//11;
const int transmit_en_pin = 3;

#define NUM_LEDS 2
#define DATA_PIN 6
CRGB leds[NUM_LEDS];
#define MAX_LUMA 160

uint8_t pattern_step = 0;
uint8_t last_rx_value;
bool i_am_home = false;
bool last_rx_succeeded = false;

unsigned long rx_poll_clk_start; 
unsigned long rx_poll_clk_now;
const unsigned long rx_poll_clk_period = 50;

unsigned long pattern_step_clk_start; 
unsigned long pattern_step_clk_now;
// controls speed of rainbow. mst be in sync with transmitter
const unsigned long pattern_step_clk_period = 400;

unsigned long max_home_radio_silence_clk_start; 
unsigned long max_home_radio_silence_clk_now;
const unsigned long max_home_radio_silence_clk_period = 45000;

uint8_t away_hue = random(0,255);

void setup() {
    Serial.begin(9600);
    // Initialise the IO and ISR
    vw_set_tx_pin(transmit_pin);
    vw_set_rx_pin(receive_pin);
    vw_set_ptt_pin(transmit_en_pin);
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(2000);	 // Bits per sec
    vw_rx_start();
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

    rx_poll_clk_start = millis(); 
    pattern_step_clk_start = millis(); 
    max_home_radio_silence_clk_start = millis(); 
}

void send_midi(){
  CHSV hsv(pattern_step, 255, 255);
  CRGB rgb;
  hsv2rgb_rainbow(hsv,rgb); 
  usbMIDI.sendControlChange(0, map(rgb.r,0,255,0,255), 1);  // 60 = C4
  usbMIDI.sendControlChange(1, map(rgb.g,0,255,0,255), 1);  // 60 = C#4
  usbMIDI.sendControlChange(2, map(rgb.b,0,255,0,255), 1);  // 60 = D4
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

void breathe_hue() {
  // the "away from home" pattern
  if( pattern_step == 0 || pattern_step == 128 ) {
    away_hue = random(0,255);
  }
  uint8_t value;
  uint8_t breath_low_val = 0;
  uint8_t breath_high_val = 40;
  if (pattern_step <= 63 ) {
    // breathe in
    value = map(pattern_step, 0, 63, breath_low_val, breath_high_val);
  } 
  else if (pattern_step >= 64 && pattern_step <= 127 ) {
    // breathe out
    value = map(pattern_step, 64, 127, breath_high_val, breath_low_val);
  }
  else if (pattern_step >= 128 && pattern_step <= 191 ) {
    // breathe in
    value = map(pattern_step, 128, 191, breath_low_val, breath_high_val);
  }
  else {
    // breathe out
    value = map(pattern_step, 192, 255, breath_high_val, breath_low_val);
  }
  Serial.print("###    value: ");
  Serial.println(value);
  fill_solid(leds, NUM_LEDS, CHSV( away_hue, 255, 255 ));
  FastLED.setBrightness(value);
  FastLED.show();

}

bool try_rx() {
  bool rx_successful = false;
 
  for (uint8_t i = 0; i < 2; i++) {
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    if (vw_get_message(buf, &buflen)) {
      if( buf[0] == 0x53 && buf[1] == 0x52 && buf[2] == 0x46 ) {
        last_rx_value = buf[3];
        rx_successful = true;
        break;
      }
    }
  }
  //vw_rx_stop();
  return rx_successful;
}

void loop() {
  rx_poll_clk_now = millis(); 
  pattern_step_clk_now = millis(); 
  max_home_radio_silence_clk_now = millis(); 
  
  if (rx_poll_clk_now - rx_poll_clk_start >= rx_poll_clk_period) {
    if(try_rx()) {
      i_am_home = true;
      last_rx_succeeded = true;
      Serial.println("+++ RX SUCCESS +++");
      max_home_radio_silence_clk_start = rx_poll_clk_now; // reset silence timer
    } else {
      last_rx_succeeded = false;
      Serial.println("--- RX FAIL ---");
    }
    rx_poll_clk_start = rx_poll_clk_now; // reset poll timer
  } // radio poll ticks

  if (max_home_radio_silence_clk_now - max_home_radio_silence_clk_start >= max_home_radio_silence_clk_period) {
    // radio silence timeout exceeded
    i_am_home = false;
  } // radio silence ticks

  if (pattern_step_clk_now - pattern_step_clk_start >= pattern_step_clk_period) {
    Serial.println(pattern_step);
    if (last_rx_succeeded) {
      //pattern_step_wheel((pattern_step - last_rx_value)/2);
      pattern_step = last_rx_value;
    }
    if (i_am_home) {
      //FastLED.setBrightness(MAX_LUMA);
      //display_step_as_hue(100);
      send_midi();
    } else { // not home
      breathe_hue();
      //display_step_as_value(90);
    } 
    pattern_step = pattern_step < 255 ? pattern_step + 1 : 0;
    pattern_step_clk_start = pattern_step_clk_now; // reset step timer
  } // pattern ticks
}
