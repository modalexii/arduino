/* 
 *  Board settings:
 *  Teensy LC
 */
 
// ----- uncomment 1 per controller ----- //
#define THIS_CONTROLLER 0 // YELLOW
//#define THIS_CONTROLLER 1 // BLUE
//#define THIS_CONTROLLER 2 // GREEN
//#define THIS_CONTROLLER 3 // RED
//define THIS_CONTROLLER 4 // BLACK

// radio
// (it's Serial1... #define HC12 Serial1 had issues for some reason?)

// transmission control
byte start_marker = 0x3C; // '<'
byte end_marker = 0x3E; // '>'
byte rb;
byte byte_a;
byte byte_b;
bool transmit_success;

// the single int sent over the air, in which each bit represents
// a single relay state on the recieving end
uint32_t trigger_states;
uint32_t previous_trigger_states;

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
bool trigger_a_in_timeout = false;
bool trigger_b_in_timeout = false;
bool trigger_c_in_timeout = false;

// whats the longerst a trigger can be open before we force it closed? 
#define TRIGGER_MAX_OPEN_MILLIS 6000

void readAir() {
  byte transmission[4] = {0,0,0,0};
  /*while (Serial1.available() > 4) {
    // clear out the backup
    transmission[0] = Serial1.read(); // throw it away
  }*/
  if (Serial1.available() > 3) {
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
    //Serial1.read(); // pop off one byte
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
    Serial.println("!!! Trigger A open too long - putting it in time out");
    trigger_a_in_timeout = true;
  } else {
    trigger_a_in_timeout = false;
  }
  if (trigger_b_open_time > TRIGGER_MAX_OPEN_MILLIS) {
    Serial.println("!!! Trigger B open too long - putting it in time out");
    trigger_b_in_timeout = true;
  } else {
    trigger_b_in_timeout = false;
  }
  if (trigger_c_open_time > TRIGGER_MAX_OPEN_MILLIS) {
    Serial.println("!!! Trigger C open too long - putting it in time out");
    trigger_c_in_timeout = true;
  } else {
    trigger_c_in_timeout = false;
  }
  
} // /checkTimeouts()

void fireRelays() {

  if (trigger_a_in_timeout == false) {
    digitalWrite(TRIGGER_A_PIN, !bitRead(trigger_states, THIS_CONTROLLER*3));
  } else {
    digitalWrite(TRIGGER_A_PIN, HIGH);
  }
  if (trigger_b_in_timeout == false) { 
    digitalWrite(TRIGGER_B_PIN, !bitRead(trigger_states, (THIS_CONTROLLER*3)+1));
  } else {
    digitalWrite(TRIGGER_B_PIN, HIGH);
  }
  if (trigger_c_in_timeout == false) {
    digitalWrite(TRIGGER_C_PIN, !bitRead(trigger_states, (THIS_CONTROLLER*3)+2));
  } else {
    digitalWrite(TRIGGER_C_PIN, HIGH);
  }
  
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
  //Serial.println("Serial up");

  // relay pins
  //Serial.print("Setting LOW OUTPUT for relay A/B/C on pins ");
  //Serial.print(TRIGGER_A_PIN);
  //Serial.print("/");
  //Serial.print(TRIGGER_B_PIN);
  //Serial.print("/");
  //Serial.print(TRIGGER_C_PIN);
  //Serial.println(" respectively");
  pinMode(TRIGGER_A_PIN, OUTPUT);
  pinMode(TRIGGER_B_PIN, OUTPUT);
  pinMode(TRIGGER_C_PIN, OUTPUT);
  /*digitalWrite(TRIGGER_A_PIN, HIGH);
  digitalWrite(TRIGGER_B_PIN, HIGH);
  digitalWrite(TRIGGER_C_PIN, HIGH);*/
  
  // init radio
  //Serial.println("Initializing (unverifyable) HC12 radio on Serial1 rx(rx) tx(tx)");   
  Serial1.begin(9600);    

  /*Serial.print("This controller is ");
  Serial.print(THIS_CONTROLLER);
  Serial.print("; it's trigger LSB is ");
  Serial.println(THIS_CONTROLLER*3);*/
  
} // /setup()

void loop() {

  delay(10);
  previous_trigger_states = trigger_states;
  readAir();
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
