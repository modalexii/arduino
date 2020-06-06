#include "FastLED.h"
#include <VirtualWire.h>

const int led_pin = 11;
const int transmit_pin = 1                                                         ;
const int receive_pin = 2;
const int transmit_en_pin = 3;

#define NUM_LEDS 1
#define DATA_PIN 6
CRGB leds[NUM_LEDS];

unsigned long pattern_step_clk_start; 
unsigned long pattern_step_clk_now;
// controls speed of rainbow. mst be in sync with receiver
const unsigned long pattern_step_clk_period = 400;
uint8_t pattern_step = 0;

void setup()
{
    Serial.begin(9600);
    // Initialise the IO and ISR
    vw_set_tx_pin(transmit_pin);
    vw_set_rx_pin(receive_pin);
    vw_set_ptt_pin(transmit_en_pin);
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(2000);       // Bits per sec
    pinMode(led_pin, OUTPUT);
    pattern_step_clk_start = millis(); 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

void display_hue(uint8_t hue) {
  leds[0] = CHSV( hue, 255, 100 );
  FastLED.show();
}

void spam() {
  uint8_t msg[4] = {byte(0x53), byte(0x52), byte(0x46), pattern_step};
  vw_send((uint8_t *)msg, 4);
  vw_wait_tx(); // Wait until the whole message is gone
}

byte count = 1;

void loop() {
  pattern_step_clk_now = millis(); 
  if (pattern_step_clk_now - pattern_step_clk_start >= pattern_step_clk_period) {
    Serial.println(pattern_step);
    display_hue(pattern_step);
    pattern_step = pattern_step < 255 ? pattern_step + 1 : 0;
    pattern_step_clk_start = pattern_step_clk_now; // reset step timer 
  }
  spam();
}
