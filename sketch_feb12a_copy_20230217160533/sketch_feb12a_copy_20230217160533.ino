
#include <RF24.h>
 
RF24 radio(2, 4); // use this for NodeMCU Amica/AdaFruit Huzzah ESP8266 Feather
 
// transmission control
byte start_marker = 0x3C; // '<'
byte end_marker = 0x3E; // '>'
uint32_t transmission;
const byte address[6] = "00001";
uint16_t trigger_states;
uint16_t previous_trigger_states; // tracked exclusively for sane serial print debugging

void setup()
{
  Serial.begin(115200);
    // init radio
  radio.begin();                  //Starting the Wireless communication
  radio.openWritingPipe(address); //Setting the address where we will send the data
  radio.setPALevel(RF24_PA_MAX);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.setDataRate(RF24_250KBPS);
  radio.stopListening();  
  //uint8_t address[5] = {0,0,0,0,1};
  //uint8_t address[6] = "00001";
  //uint8_t address[5] = {0x31,0x30,0x30,0x30,0x30};
  //uint8_t address[6] = "00001";
  /*if (!nrf24.setNetworkAddress(address, 5))  {
    Serial.println("setNetworkAddress failed");
  }*/

}

void test() {
  // send a test pattern forever, when manually invoked
  Serial.println("Test mode");
  while (true) {
    for (uint8_t i=0; i<3; i++) {
      trigger_states=0;
      bitSet(trigger_states, i);
      for (uint8_t i = 0; i<16; i++) {
        Serial.print(bitRead(trigger_states, i));
      }
      Serial.println();
      transmission = (start_marker << 24) |
        (highByte(trigger_states) << 16) |
        (lowByte(trigger_states) << 8) |
        end_marker;
      radio.write(&transmission, sizeof(transmission));
      delay(500); 
    } 
  }
}
 
void loop() {

  test();
 
}