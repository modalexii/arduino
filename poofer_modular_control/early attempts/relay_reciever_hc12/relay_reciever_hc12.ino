/* 
 *  Board settings:
 *  Arduino Leonardo
 */

#include <SoftwareSerial.h>

// ----- uncomment 1 per controller ----- //
#define THIS_CONTROLLER 0 // YELLOW
//#define THIS_CONTROLLER 1 // BLUE
//#define THIS_CONTROLLER 2 // GREEN
//#define THIS_CONTROLLER 3 // RED
//define THIS_CONTROLLER 4 // BLACK

// radio
#define RADIO_RX_PIN 3
#define RADIO_TX_PIN 2
SoftwareSerial HC12(RADIO_RX_PIN, RADIO_TX_PIN); 

// transmission control
byte start_marker = 0x3C; // '<'
byte end_marker = 0x3E; // '>'
byte rb;
byte byte_a;
byte byte_b;
bool transmit_success;

// the single int sent over the air, in which each bit represents
// a single relay state on the recieving end
int trigger_states;
int previous_trigger_states;

#define TRIGGER_A_PIN 2
#define TRIGGER_B_PIN 3
#define TRIGGER_C_PIN 4

// places to store how long triggers have been open so we can
// force them closed after a timeout
unsigned long trigger_a_open_time;
unsigned long trigger_b_open_time;
unsigned long trigger_c_open_time;
unsigned long previous_timer = 0;
unsigned long elapsed;

// whats the longerst a trigger can be open before we force it closed? 
#define TRIGGER_MAX_OPEN_MILLIS 6000

void readAir() {
  byte transmission[4] = {0,0,0,0};
  while (Serial1.available() > 3) {
    transmission[0] = Serial1.read();
    transmission[1] = Serial1.read();
    transmission[2] = Serial1.read();
    transmission[3] = Serial1.read();
  }
  /*Serial.print(transmission[0],BIN);
  Serial.print(" ");
  Serial.print(transmission[1],BIN);
  Serial.print(" ");
  Serial.print(transmission[2],BIN);
  Serial.print(" ");
  Serial.print(transmission[3],BIN);
  Serial.println();*/
  if (transmission[0] == start_marker && transmission[3] == end_marker) {
    //Serial.println("Transmission control success");
    trigger_states = (transmission[1] << 8) + transmission[2];
  } else {
    //Serial.println("Transmission control failure");
  }
} // /readAir()

void checkTimeouts() {

  elapsed = millis() - previous_timer;
  previous_timer = millis();
  if (bitRead(trigger_states, THIS_CONTROLLER*3) == HIGH) {
    trigger_a_open_time += elapsed;
  } else {
    trigger_a_open_time = 0;
  }
  if (bitRead(trigger_states, (THIS_CONTROLLER*3)+1) == HIGH) {
    trigger_b_open_time += elapsed;
  } else {
    trigger_b_open_time = 0;
  }
  if (bitRead(trigger_states, (THIS_CONTROLLER*3)+2) == HIGH) {
    trigger_c_open_time += elapsed;
  } else {
    trigger_c_open_time = 0;
  }

  if (trigger_a_open_time > TRIGGER_MAX_OPEN_MILLIS) {
    bitClear(trigger_states, THIS_CONTROLLER*3);
    Serial.println("!!! Trigger A was open too long.");
  }
  if (trigger_b_open_time > TRIGGER_MAX_OPEN_MILLIS) {
    bitClear(trigger_states, (THIS_CONTROLLER*3)+1);
    Serial.println("!!! Trigger B was open too long.");
  }
  if (trigger_c_open_time > TRIGGER_MAX_OPEN_MILLIS) {
    bitClear(trigger_states, (THIS_CONTROLLER*3)+2);
    Serial.println("!!! Trigger C was open too long.");
  }
  
} // /checkTimeouts()

void fireRelays() {

  digitalWrite(TRIGGER_A_PIN, bitRead(trigger_states, THIS_CONTROLLER*3));
  digitalWrite(TRIGGER_B_PIN, bitRead(trigger_states, (THIS_CONTROLLER*3)+1));
  digitalWrite(TRIGGER_C_PIN, bitRead(trigger_states, (THIS_CONTROLLER*3)+2));
   
} // fireRelays()

void test() {
  while (true) {
    while (Serial1.available() > 0) {
      Serial.print(Serial1.read(), BIN);
      Serial.print("  ");
    }
    Serial.println();
    delay(100);
  }
}

void setup() {
  
  Serial.begin(115200);
  Serial.println("Serial up");
  Serial1.begin(9600);

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
  Serial.print("Initializing HC12 radio on TX ");
  Serial.print(RADIO_TX_PIN);
  Serial.print(" RX ");
  Serial.println(RADIO_RX_PIN);
  HC12.begin(9600);    

  Serial.print("This controller is ");
  Serial.print(THIS_CONTROLLER);
  Serial.print("; it's trigger LSB is ");
  Serial.println(THIS_CONTROLLER*3);
  
} // /setup()

void loop() {
  
  //test();
  previous_trigger_states = trigger_states;
  readAir();
  //recvWithStartEndMarkers();
  //checkTimeouts();
  fireRelays();

  // print trigger states to console on change
  if (trigger_states != previous_trigger_states) {
    Serial.print("New trigger states: ");
    for (uint8_t i = 0; i<16; i++) {
      Serial.print(bitRead(trigger_states, i));
    }
    Serial.println();
    Serial.print(" My trigger states: ");
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
