/* 
 *  Board settings:
 *  Teensy LC
 */
 
#include <SoftwareSerial.h>
#include <Bounce2.h>
#include <arduino-timer.h>

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

struct fire_control {
  char name[16]; // friendly control name
  uint8_t trig_pin; // MCU pin
  uint8_t trig_index; // position in the OTA control signal and debounce list
  bool state; // current open/closed state
  uint16_t sticky_timeout; // how many ms to keep it high after a button press, primatily for Spin It's short presses
  bool stuck; // true for sticky_timer duration after press
  uint8_t audio_track; // which sound to play, whrere 0x01->01.wav
} triggers[] = {
  {"Left-A-Flick", 2, 0, 0, 100, 0, 0x03},
  {"Left-B-Spin", 3, 1, 0, 1000, 0, 0x05},
  {"Right-A-Twist", 4, 2, 0, 100, 0, 0x06},
  {"Right-B-Pull", 5, 3, 0, 100, 0, 0x04},
  {"All-A-Bop", 6, 4, 0, 100, 0, 0x02},
  {"All-B-Bop", 7, 5, 0, 100, 0, 0x02}
};

// the 16 bytes of control data sent over the air, in which each bit represents
// a single relay state on the recieving end
uint16_t trigger_states;
uint16_t previous_trigger_states; // tracked exclusively for sane serial print debugging

// timer for transmit duration
Timer<1> tx_sticky_timer;
bool tx_stuck; // whether or not we should be TXing

// timer with 6 tasks for handling stickey triggers
Timer<6> sticky_timer;

// timer for throttling audio sample to prevent stutter when control is held down
Timer<1> audio_lock_timer;
bool audio_locked = false;

int control_count = sizeof(triggers)/sizeof(*triggers);

// debounce lib
Bounce * buttons = new Bounce[control_count]; // fire control buttons

void setPinModes() {
  
  Serial.println("Set pin modes:");
  
  for (uint8_t i = 0; i < control_count; i++) {
    Serial.print("Set INPUT_PULLUP debounce idx ");
    Serial.println(triggers[i].name);
    buttons[i].attach(triggers[i].trig_pin , INPUT_PULLUP);
    buttons[i].interval(25); // interval in ms
  }
  
} // /setPinModes()

// comms with mp3 module
void play_audio_track(uint8_t track) { // from DFR0534 sample code 
  audio_locked = true;
  Serial.print("Playing sample ");
  Serial.println(track);
  uint8_t play[6] = {0xAA,0x07,0x02,0x00,track,track+0xB3};
  audio_lock_timer.in(500, [](void*) -> bool { audio_locked = false; return false; });
  Serial2.write(play,6);
}

void set_audio_volume( uint8_t vol) { // from DFR0534 sample code 
  uint8_t volume[5] = {0xAA,0x13,0x01,vol,vol+0xBE};
  Serial2.write(volume,5);
}

void unstick(uint8_t i) {
  // timer callback to clear the sticky bit 
  Serial.print("Clearing sticky bit for ");
  Serial.println(triggers[i].name);
  triggers[i].stuck = false;
  /*if (strncmp(triggers[i].name, "Left-B-Spin", 11) == 0) { // manually clear bit for spin, else let collectButtonStates() clear it
    bitClear(trigger_states, triggers[i].trig_index);
  }*/
}

void onNewInteraction(uint8_t i) {
  // stuff to do when there is a new bop, spin, flick, etc
  // split out of collectButtonStates() so that spin can be handled differently from other controls
  bitSet(trigger_states, triggers[i].trig_index); // set trigger ON bit 
  triggers[i].stuck = true; // set stuck
  sticky_timer.in(triggers[i].sticky_timeout, unstick, i); // start timer to unstick
  if (!audio_locked) play_audio_track(triggers[i].audio_track);
}

void collectButtonStates() {

  for (uint8_t i = 0; i < control_count; i++) { // all buttons
    /*if (buttons[i].update()) { // poll button 
      triggers[i].state = buttons[i].read();
      Serial.print(triggers[i].name);
      Serial.print(" now ");
      Serial.println(triggers[i].state);
      if (triggers[i].state == LOW && !triggers[i].stuck) { // new button press and not stuck
        bitSet(trigger_states, triggers[i].trig_index); // set trigger ON bit 
        triggers[i].stuck = true; // set stuck
        sticky_timer.in(triggers[i].sticky_timeout, unstick, i); // start timer to unstick
      } else if (triggers[i].state == HIGH && !triggers[i].stuck) { // button not pressed and not stuck
        // set trigger OFF bit
        bitClear(trigger_states, triggers[i].trig_index);
      } // if triggers[i].stuck == true, no change will be made
    } */
    bool changed = buttons[i].update();
    if (triggers[i].stuck) { continue; } // if this control is stickied, skip it
    if (strncmp(triggers[i].name, "Left-B-Spin", 11) == 0) { 
      // spin needs to act on ANY change since the wheel may come to rest with the lever depressing the switch
      if (changed) {
        onNewInteraction(i);
      } else {
        bitClear(trigger_states, triggers[i].trig_index); // set trigger OFF bit
      }
    } else {
      // all controls aside from spin
      triggers[i].state = buttons[i].read();
      /*Serial.print(triggers[i].name);
      Serial.print(" now ");
      Serial.println(triggers[i].state);*/
      if (changed && triggers[i].state == LOW) { // pressed 
        onNewInteraction(i);
      } else if (triggers[i].state == HIGH) { // not pressed 
        bitClear(trigger_states, triggers[i].trig_index); // set trigger OFF bit
      }
    }
  }

} // /collectButtonStates()

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

void setup() {

  // init serial for debug
  Serial.begin(115200);
  Serial.println("Serial up");

  setPinModes();

  // init radio
  radio.begin();                  //Starting the Wireless communication
  radio.openWritingPipe(address); //Setting the address where we will send the data
  radio.setPALevel(RF24_PA_MAX);  //You can set it as minimum or maximum depending on the distance between the transmitter and receiver.
  radio.setDataRate(RF24_250KBPS);
  radio.stopListening();    

  // voice via DFR0534
  Serial2.begin(9600);
  set_audio_volume(0x1E); //Volume settings 0x00-0x1E

  // play random "swomp it" track 0x07-0x0A
  randomSeed(analogRead(A0)+analogRead(A6));
  byte power_up_tracks[4] = {0x07,0x08,0x09,0x0A};
  play_audio_track(power_up_tracks[random(3)]); 

} // setup()

void loop() {

  //test();
  sticky_timer.tick(); // tick the timer
  audio_lock_timer.tick(); // tick the timer
  tx_sticky_timer.tick(); // tick the timer
  previous_trigger_states = trigger_states; 
  collectButtonStates();

  // print trigger states to console on change
  if (trigger_states != previous_trigger_states) {
    Serial.print("New trigger states: ");
    for (uint8_t i = 0; i<16; i++) {
      Serial.print(bitRead(trigger_states, i));
    }
    Serial.println();
  }

  if (trigger_states != previous_trigger_states) {
    // our fire directive has changed - tell transmitter to spam the air for a while
    // doing this only for a fraction of a second prevents flapping when the bop-it is powered on
    tx_stuck = true;
    tx_sticky_timer.in(1000, [](void*) -> bool { tx_stuck = false; return false; });
  }

  if (tx_stuck) { // wiggle the air
    transmission = (start_marker << 24) |
                  (highByte(trigger_states) << 16) |
                  (lowByte(trigger_states) << 8) |
                  end_marker;
    radio.write(&transmission, sizeof(transmission));
  }
  
} // /loop()
