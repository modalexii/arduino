#include <usb_keyboard.h>

void setup() { }

void keystroke(char key, int repeats, unsigned long pause) {
  // press & immediately release KEY
  // loop REPEATS times
  // wait PAUSE ms after each keyup
  int n;
  unsigned long end_pause;
  for ( n = 0; n < repeats; n++ ) {
    Keyboard.set_key1(key);
    Keyboard.send_now();
    Keyboard.set_key1(0);
    Keyboard.send_now();
    end_pause = millis() + pause;
    while(millis() < end_pause) { }
  }
}

void loop() {
  
  delay(5000); // wait for computer to init usb
  
  Keyboard.set_modifier(0);
  Keyboard.set_key1(0);
  Keyboard.set_key2(0);
  Keyboard.set_key3(0);
  Keyboard.set_key4(0);
  Keyboard.set_key5(0);
  Keyboard.set_key6(0);
  
  // enter BIOS setup
  keystroke(KEY_F12, 8, 700);
  keystroke(KEY_DOWN, 2, 100);
  keystroke(KEY_ENTER, 1, 2000);
  
  // disable camera
  keystroke(KEY_DOWN, 5, 80);
  keystroke(KEY_SPACE, 1, 80);
  keystroke(KEY_DOWN, 8, 80);
  keystroke(KEY_TAB, 1, 80);
  keystroke(KEY_DOWN, 2, 80);
  keystroke(KEY_SPACE, 1, 80);;
  
  // disable radios
  keystroke(KEY_TAB, 4, 80);
  keystroke(KEY_DOWN, 7, 80);
  keystroke(KEY_SPACE, 1, 80);
  keystroke(KEY_DOWN, 2, 80);
  keystroke(KEY_TAB, 1, 80);
  keystroke(KEY_SPACE, 1, 80);
  keystroke(KEY_DOWN, 1, 80);
  keystroke(KEY_SPACE, 1, 80);
  keystroke(KEY_RIGHT, 1, 80);
  keystroke(KEY_SPACE, 1, 80);
  
  // apply
  keystroke(KEY_TAB, 1, 80);
  keystroke(KEY_RIGHT, 1, 80);
  keystroke(KEY_ENTER, 1, 2500);
  
  // exit
  keystroke(KEY_TAB, 2, 80);
  keystroke(KEY_RIGHT, 1, 80);
  keystroke(KEY_ENTER, 1, 80);
  
  // wait forever, do not loop
  while(true) { }
  
}
