
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif
#include "FastLED.h"


#define NUM_LEDS 2
#define DATA_PIN 6
CRGB leds[NUM_LEDS];


// RH_ASK driver(2000, 4, 5, true); // rx tx ptt ptt_invert // NodeMCU
RH_ASK driver(2000, 9, 5, true); // rx tx ptt ptt_invert // 328p-au

#define RADIO_EN_PIN 8

void setup() {
  Serial.begin(115200);	  // Debugging only
  if (!driver.init()) {
    Serial.println("init failed");
  }
  Serial.println("Rx: Serial up");
  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  digitalWrite(RADIO_EN_PIN, LOW);
  pinMode(RADIO_EN_PIN, OUTPUT); 
}

void set_radio_state(bool state) {
  /*
   * Turn SYN480R radio on (state=1) or off (state=0)
   */
  // pull RADIO_PIN low to sink current and enable radio
  if (state) { digitalWrite(RADIO_EN_PIN, LOW); }
  else { digitalWrite(RADIO_EN_PIN, HIGH); }
}

void loop() {

  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

   set_radio_state(1);
  // sleep while we wait for the radio to initilize 
  delay(80); // 52 seems to be the minimum here, 60 gives some tolerance

  if (driver.recv(buf, &buflen)) {
    driver.printBuffer("-Got:", buf, buflen);
    // if the first 3 bytes are "SRF", its a good message
    if( buf[0] == 0x53 && buf[1] == 0x52 && buf[2] == 0x46 ) {
      uint8_t last_rx_value = buf[3];
      leds[0]=CHSV(last_rx_value,255,255);
      leds[1]=CHSV(last_rx_value,255,255);
      FastLED.show();
    } 

    set_radio_state(0);
    
  } 
  delay(100);

} // loop