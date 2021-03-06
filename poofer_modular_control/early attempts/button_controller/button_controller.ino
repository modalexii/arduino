/* 
 *  Board settings:
 *  Arduino Nano
 *  ATMega328p (Old Bootloader)
 */
 
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Bounce2.h>

// Power /OK LED
#define LEDPIN_UP 12

// radio
#define RADIO_CE_PIN 2
#define RADIO_CSN_PIN 3
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);
// Byte array representing the radio address
// (needs to be same on the receiving side)
const byte radio_address[5] = {'P','R','P','L','E'};   

// transmission control
byte start_marker = 0x3C; // '<'
byte end_marker = 0x3E; // '>'

// button combination behavior switch
#define combo_ignore_pin 8
#define combo_ab_eq_c_pin 9
#define combo_ab_eq_bc_pin 10
#define combo_ab_eq_abc_pin 11
#define BEHAVIOR_COUNT 4
const uint8_t BEHAVIOR_SELECT_PINS[BEHAVIOR_COUNT] = {combo_ignore_pin, combo_ab_eq_c_pin, combo_ab_eq_bc_pin, combo_ab_eq_abc_pin};
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
  {"YELLOW", A0, A1, 0, 1, 2},
  {"BLUE", A2, A3, 3, 4, 5},
  {"GREEN", A4, A5, 6, 7, 8},
  {"RED", 4, 5, 9, 10, 11},
  {"BLACK", 6, 7, 12, 13, 14}
};

// the single int sent over the air, in which each bit represents
// a single relay state on the recieving end
int trigger_states;
int previous_trigger_states;

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

  int button_a_index;
  int button_b_index; 
  bool button_a_state;
  bool button_b_state;
  // bools to track if either button changed this iternation,
  // becuase we can't check update() more than once
  bool button_a_updated;
  bool button_b_updated;
  for (uint8_t i = 0; i < control_count; i++) {
    
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
      if (button_a_state == LOW && button_b_state == LOW) {
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

void getComboBehavior(bool set_without_update) {
  // set_without_update is used to firse read() on controller boot
  // when the swatch has never update()ed. All future loop()s we
  // dont want to have to read() 4 pins if there was no update()
  bool was_updated;
  for (uint8_t i = 0; i < BEHAVIOR_COUNT; i++)  {
    was_updated = behaviors[i].update();
    // return the first behavior pin that is low
    // this isnt perfect, but the hardware is a rotary selector, so 
    // there "should" never be more than one low pin
    if (set_without_update) {
      if (behaviors[i].read() == LOW) {
        Serial.print("New behavior setting: ");
        Serial.println(i);
        current_behavior = i;
      }
    } else if (was_updated) {
      if (behaviors[i].read() == LOW) {
        Serial.print("New behavior setting: ");
        Serial.println(i);
        current_behavior = i;
      }
    }
  }
  return 0; // if something went wrong?
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
      radio.write(&trigger_states, sizeof(trigger_states));
      delay(250);
    } 
  }
}

void setup() {

  // init serial for debug
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("Serial up");
  
  setPinModes();
  
  // turn on status LED
  // (not much happens with this, but if the controller dies or isnt 
  // getting enough power it obviously won't go on, which is helpful)
  digitalWrite(LEDPIN_UP, HIGH);

  // collect inital behavior state
  Serial.println("Collecting initial combo behavior");
  getComboBehavior(true);

  // init radio
  Serial.print("Initializing radio on CE ");
  Serial.print(RADIO_CE_PIN);
  Serial.print(" CSN ");
  Serial.println(RADIO_CSN_PIN);
  radio.begin(); 
  radio.setPALevel(RF24_PA_MIN);  // set MIN or MAX
  radio.setChannel(0x6C);
  //radio.setAutoAck(false);
  radio.openWritingPipe(radio_address); 
  // set the module as transmitter
  radio.stopListening();          

} // setup()

void loop() {

  test();
  previous_trigger_states = trigger_states;
  getComboBehavior(false);
  collectButtonStates();

  // print trigger states to console on change
  if (trigger_states != previous_trigger_states) {
    Serial.print("New trigger states: ");
    for (uint8_t i = 0; i<16; i++) {
      Serial.print(bitRead(trigger_states, i));
    }
    Serial.println();
  }

  // send OTA
  radio.write(&start_marker, sizeof(start_marker));
  radio.write(&trigger_states, sizeof(trigger_states));
  radio.write(&end_marker, sizeof(end_marker));

} // /loop()
