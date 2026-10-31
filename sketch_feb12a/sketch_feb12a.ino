#include <RH_NRF24.h>
#include <EEPROM.h>
#include "printf.h"
 
RH_NRF24 nrf24(2, 4); // use this for NodeMCU Amica/AdaFruit Huzzah ESP8266 Feather
 
// transmission control
byte start_marker = 0x3C; // '<'
byte end_marker = 0x3E; // '>'
uint32_t transmission;
 
void setup()
{
  Serial.begin(115200);
  if (!nrf24.init())   {
    Serial.println("init failed");
  }
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  if (!nrf24.setChannel(0x4c))  {
    Serial.println("setChannel failed");
  }
  //uint8_t address[5] = {0,0,0,0,1};
  //uint8_t address[6] = "00001";
  //uint8_t address[5] = {0x31,0x30,0x30,0x30,0x30};
  //uint8_t address[6] = "00001";
  /*if (!nrf24.setNetworkAddress(address, 5))  {
    Serial.println("setNetworkAddress failed");
  }*/
  if (!nrf24.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm)) {
    Serial.println("setRF failed");
  }
  nrf24.setModeTx();
  Serial.println("Transmitter started");
  printf_begin();
  nrf24.printRegisters();
}

void test() {
  // send a test pattern forever, when manually invoked
  Serial.println("Test mode");
  while (true) {
    for (uint8_t i=0; i<=8; i++) {
      uint8_t control_byte = 0;
      bitSet(control_byte, i);
      for (uint8_t i = 0; i<8; i++) {
        Serial.print(bitRead(control_byte, i));
      }
      Serial.println();
      //uint8_t dataArray[4] = {start_marker, control_byte, control_byte, end_marker};
      uint8_t dataArray[4] = {start_marker, control_byte, control_byte, end_marker};
      nrf24.send(dataArray,sizeof(dataArray));
      nrf24.waitPacketSent();
      delay(400); 
    } 
  }
}
 
void loop() {

  test();
 
}