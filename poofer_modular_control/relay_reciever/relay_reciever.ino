/* 
 *  Board settings:
 *  Arduino Nano
 *  ATMega328p (Old Bootloader)
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ----- uncomment 1 per controller ----- //
#define THIS_CONTROLLER 0 // YELLOW
//#define THIS_CONTROLLER 1 // BLUE
//#define THIS_CONTROLLER 2 // GREEN
//#define THIS_CONTROLLER 3 // RED
//#define THIS_CONTROLLER 4 // BLACK

// radio
#define RADIO_CE_PIN 10
#define RADIO_CSN_PIN 11
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);
// Byte array representing the radio address
// (needs to be same on the receiving side)
const byte radio_address[5] = {'P','R','P','L','E'};  

// the single int sent over the air, in which each bit represents
// a single relay state on the recieving end
int trigger_states;
int previous_trigger_states;

#define TRIGGER_A_PIN 4//2
#define TRIGGER_B_PIN 5//3
#define TRIGGER_C_PIN 6//4

void readAir() {
  if (radio.available()) {
    radio.read(&trigger_states, sizeof(trigger_states));
  }
} // /readAir()

void fireRelays() {

  digitalWrite(TRIGGER_A_PIN, bitRead(trigger_states, THIS_CONTROLLER*3));
  digitalWrite(TRIGGER_B_PIN, bitRead(trigger_states, (THIS_CONTROLLER*3)+1));
  digitalWrite(TRIGGER_C_PIN, bitRead(trigger_states, (THIS_CONTROLLER*3)+2));
   
} // fireRelays()

void setup() {
  
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("Serial up");

  // relay pins
  Serial.print("Setting LOW OUTPUT for relay A/B/C on pins ");
  Serial.print(TRIGGER_A_PIN);
  Serial.print("/");
  Serial.print(TRIGGER_B_PIN);
  Serial.print("/");
  Serial.print(TRIGGER_C_PIN);
  Serial.println(" respectively");
  // make sure they are HIGH when we start -
  // we're doing normally-closed mode so if power disappears, 
  // relay closes
  digitalWrite(TRIGGER_A_PIN, HIGH);
  digitalWrite(TRIGGER_B_PIN, HIGH);
  digitalWrite(TRIGGER_C_PIN, HIGH);
  pinMode(TRIGGER_A_PIN, OUTPUT);
  pinMode(TRIGGER_B_PIN, OUTPUT);
  pinMode(TRIGGER_C_PIN, OUTPUT);
  
  // init radio
  Serial.print("Initializing radio on CE ");
  Serial.print(RADIO_CE_PIN);
  Serial.print(" CSN ");
  Serial.println(RADIO_CSN_PIN);
  radio.begin(); 
  radio.setPALevel(RF24_PA_MIN);  // set MIN or MAX
  radio.setChannel(0x6C);
  //radio.setAutoAck(false);
  radio.openReadingPipe(1, radio_address); 
  // set the module as receiver
  radio.startListening();   

  Serial.print("This controller is ");
  Serial.print(THIS_CONTROLLER);
  Serial.print("; it's trigger LSB is ");
  Serial.println(THIS_CONTROLLER*3);
  
} // /setup()

void loop() {

  previous_trigger_states = trigger_states;
  readAir();
  fireRelays();

  // print trigger states to console on change
  if (trigger_states != previous_trigger_states) {
    Serial.print("New trigger states: ");
    for (uint8_t i = 0; i<16; i++) {
      Serial.print(bitRead(trigger_states, i));
    }
    Serial.println();
    Serial.print("My trigger states: ");
    for (uint8_t i = 0; i<16; i++) {
      if (i>=THIS_CONTROLLER*3 && i<(THIS_CONTROLLER*3)+3) {
        Serial.print(bitRead(trigger_states, i));
      } else {
        Serial.print("-");
      }
    }
    Serial.println();
  } 

} // /loop()
