/* 
 *  Board settings:
 *  Teensy 3.2
 */
 
#include <SoftwareSerial.h>
#include <Bounce2.h>

// Power /OK LED
#define LEDPIN_UP 17

// radio
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(18, 19); // CE, CSN 
const byte address[6] = "00001";

// transmission control
byte start_marker = 0x3C; // '<'
byte end_marker = 0x3E; // '>'
uint32_t transmission;

// button combination behavior switch
#define combo_ignore_pin 20
#define combo_ab_eq_c_pin 21
#define combo_ab_eq_bc_pin 22
#define combo_ab_eq_abc_pin 23
#define BEHAVIOR_COUNT 4
const uint8_t BEHAVIOR_SELECT_PINS[BEHAVIOR_COUNT] = {
  combo_ignore_pin, 
  combo_ab_eq_c_pin, 
  combo_ab_eq_bc_pin, 
  combo_ab_eq_abc_pin
};
uint8_t current_behavior;

// trigger, identifier, & button pin map
struct fire_control {
  char name[10];
  uint8_t button_a_pin;
  uint8_t button_b_pin;
  uint8_t trigger_a_index;
  uint8_t trigger_b_index;
  uint8_t trigger_c_index;
} triggers[] = {
  {"YELLOW", 2, 3, 0, 1, 2},
  {"BLUE", 4, 5, 3, 4, 5},
  {"GREEN", 6, 7, 6, 7, 8},
  {"RED", 8, 9, 9, 10, 11},
  {"BLACK", 14, 15, 12, 13, 14}
};

// the single int sent over the air, in which each bit represents
// a single relay state on the recieving end
int trigger_states;
int previous_trigger_states;

// button state tracking
int button_a_index;
int button_b_index; 
bool button_a_state;
bool button_b_state;
// bools to track if either button changed this iternation,
// becuase we can't check update() more than once
bool button_a_updated;
bool button_b_updated;

int control_count = sizeof(triggers)/sizeof(*triggers);
int button_count = 2*control_count;

// debounce lib
Bounce * buttons = new Bounce[button_count]; // fire control buttons
Bounce * behaviors = new Bounce[BEHAVIOR_COUNT]; // behavior select switch

void setPinModes() {
  
  Serial.println("Set pin modes:");
  
  int button_a_index;
  int button_b_index;
  
  for (uint8_t i = 0; i < control_count; i++) {
    button_a_index = i+i;
    button_b_index = i+i+1;
    Serial.print("Set INPUT_PULLUP debounce idx ");
    Serial.print(button_a_index);
    Serial.print(" (btn A) trigger ");
    Serial.println(triggers[i].name);
    buttons[button_a_index].attach(triggers[i].button_a_pin , INPUT_PULLUP);
    buttons[button_a_index].interval(25); // interval in ms
    Serial.print("Set INPUT_PULLUP debounce idx ");
    Serial.print(button_b_index);
    Serial.print(" (btn B) trigger ");
    Serial.println(triggers[i].name);
    buttons[button_b_index].attach(triggers[i].button_b_pin , INPUT_PULLUP);
    buttons[button_b_index].interval(25); // interval in ms
  }
  
  Serial.print("Set OUTPUT health LED pin ");
  Serial.println(LEDPIN_UP);
  pinMode(LEDPIN_UP, OUTPUT);
  
  for (uint8_t i = 0; i < BEHAVIOR_COUNT; i++) {
    behaviors[i].attach(BEHAVIOR_SELECT_PINS[i], INPUT_PULLUP);
    behaviors[i].interval(10); // interval in ms - not expecting much bounce here
  }
  
} // /setPinModes()

void collectButtonStates() {

  for (uint8_t i = 0; i < control_count; i++) { // each set of colors
    
    button_a_index = i+i;
    button_b_index = i+i+1;

    // poll button A
    if (buttons[button_a_index].update()) {
      button_a_updated = true;
      button_a_state = buttons[button_a_index].read();
      Serial.print(triggers[i].name);
      Serial.print(" - A now ");
      Serial.println(button_a_state);
      if (button_a_state == LOW) {
        // set trigger ON bit
        bitSet(trigger_states, triggers[i].trigger_a_index);
      } else if (button_a_state == HIGH) {
        // set triffer OFF bit
        bitClear(trigger_states, triggers[i].trigger_a_index);
      }
    } else {
      button_a_updated = false;
    }
    
    // poll button B
    if (buttons[button_b_index].update()) {
      button_b_updated = true;
      button_b_state = buttons[button_b_index].read();
      Serial.print(triggers[i].name);
      Serial.print(" - B now ");
      Serial.println(button_b_state);
      if (button_b_state == LOW) {
        // set trigger ON bit
        bitSet(trigger_states, triggers[i].trigger_b_index);
      } else if (button_b_state == HIGH) {
        // set triffer OFF bit
        bitClear(trigger_states, triggers[i].trigger_b_index);
      }
    } else {
      button_b_updated = false;
    }
    
    if (button_a_updated || button_b_updated) {
      //if (button_a_state == LOW && button_b_state == LOW) {
      if (
        bitRead(trigger_states, triggers[i].trigger_a_index) == 1 && 
        bitRead(trigger_states, triggers[i].trigger_b_index) == 1) {
        // translate combination according to switch setting
        switch (current_behavior) {
          case 0:
            Serial.print(triggers[i].name);
            Serial.println(" - not translating A+B combo");
            break;
          case 1:
            Serial.print(triggers[i].name);
            Serial.println(" - translating A+B combo to trig C");
            bitClear(trigger_states, triggers[i].trigger_a_index);
            bitClear(trigger_states, triggers[i].trigger_b_index);
            bitSet(trigger_states, triggers[i].trigger_c_index);
            break;
          case 2:
            Serial.print(triggers[i].name);
            Serial.println(" - translating A+B combo to trig B+C");
            bitClear(trigger_states, triggers[i].trigger_a_index);
            bitSet(trigger_states, triggers[i].trigger_c_index);
            break;
          case 3:
            // translate A + B to trigger A + B + C
            Serial.print(triggers[i].name);
            Serial.println(" - translating A+B combo to trig A+B+C");
            bitSet(trigger_states, triggers[i].trigger_c_index);
            break;
        }
      } else {
        // no combination - turn off trigger c
        Serial.print(triggers[i].name);
        Serial.println(" - clearing trig C");
        bitClear(trigger_states, triggers[i].trigger_c_index);
      }
    }
  }

} // /collectButtonStates()

void getComboBehavior() {
  bool was_updated = false;
  for (uint8_t i = 0; i < BEHAVIOR_COUNT; i++)  {
    if (behaviors[i].update() == true) {
      was_updated = true;
    }
  }
  if (was_updated == true) {
    Serial.println("UPDATED");
    for (uint8_t i = BEHAVIOR_COUNT - 1; i > 0; i--)  {
      if (behaviors[i].read() == LOW) {
        Serial.print("New behavior setting: ");
        Serial.println(i);
        current_behavior = i;
        return;
      }
    }
    Serial.println("New behavior setting: 0");
    current_behavior = 0;
  }
} // /getComboBehavior()

void test() {
  // send a test pattern forever
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

void setup() {

  // init serial for debug
  Serial.begin(115200);
  Serial.println("Serial up");

  setPinModes();
  // turn on status LED
  // (not much happens with this, but if the controller dies or isnt 
  // getting enough power it obviously won't go on, which is helpful)
  digitalWrite(LEDPIN_UP, HIGH);

  // collect inital behavior state
  Serial.println("Collecting initial combo behavior");
  getComboBehavior();

  // init radio
  radio.begin();                  //Starting the Wireless communication
  //radio.setPayloadSize(4);
  radio.openWritingPipe(address); //Setting the address where we will send the data
  radio.setPALevel(RF24_PA_HIGH); //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.stopListening();         

} // setup()

void loop() {

  //test();
  previous_trigger_states = trigger_states;
  getComboBehavior();
  collectButtonStates();

  // print trigger states to console on change
  if (trigger_states != previous_trigger_states) {
    Serial.print("New trigger states: ");
    for (uint8_t i = 0; i<16; i++) {
      Serial.print(bitRead(trigger_states, i));
    }
    Serial.println();
  }

  // send OTA even if no changes
  transmission = (start_marker << 24) |
                 (highByte(trigger_states) << 16) |
                 (lowByte(trigger_states) << 8) |
                 end_marker;
  radio.write(&transmission, sizeof(transmission));
  delay(5); //  remove this, just trying to rule out buffer filling
  
} // /loop()
