#include <usb_keyboard.h>

void setup() { }

void repeat_single_stroke(char key, int count, int pause) {
  Keyboard.set_key1(key);
  int s;
  for ( s = 0; s < count; s++ ) {
    Keyboard.send_now();
    delay(pause);
  }
}

void loop() {
  
  delay(5000); // get past POST
  
  Keyboard.set_modifier(0);
  Keyboard.set_key1(0);
  Keyboard.set_key2(0);
  Keyboard.set_key3(0);
  Keyboard.set_key4(0);
  Keyboard.set_key5(0);
  Keyboard.set_key6(0);
  
  // enter BIOS setup
  repeat_single_stroke(KEY_F12, 8, 700);
  repeat_single_stroke(KEY_DOWN, 2, 700);
  Keyboard.set_key1(KEY_ENTER);
  Keyboard.send_now();
  delay(1000);
  
  // disable camera
  repeat_single_stroke(KEY_DOWN, 6, 500);
  Keyboard.set_key1(KEY_SPACE);
  Keyboard.send_now();
  repeat_single_stroke(KEY_DOWN, 8, 500);
  Keyboard.set_key1(KEY_TAB);
  Keyboard.send_now();
  repeat_single_stroke(KEY_DOWN, 2, 500);
  Keyboard.set_key1(KEY_SPACE);
  Keyboard.send_now();
  
  // disable radios
  repeat_single_stroke(KEY_TAB, 4, 500);
  repeat_single_stroke(KEY_DOWN, 7, 500);
  Keyboard.set_key1(KEY_SPACE);
  Keyboard.send_now();
  repeat_single_stroke(KEY_SPACE, 2, 500);
  Keyboard.set_key1(KEY_TAB);
  Keyboard.send_now();
  Keyboard.set_key1(KEY_SPACE);
  Keyboard.send_now();
  Keyboard.set_key1(KEY_DOWN);
  Keyboard.send_now();
  Keyboard.set_key1(KEY_SPACE);
  Keyboard.send_now();
  Keyboard.set_key1(KEY_RIGHT);
  Keyboard.send_now();
  Keyboard.set_key1(KEY_SPACE);
  Keyboard.send_now();
  
  // apply
  Keyboard.set_key1(KEY_TAB);
  Keyboard.send_now();
  Keyboard.set_key1(KEY_RIGHT);
  Keyboard.send_now();
  Keyboard.set_key1(KEY_ENTER);
  Keyboard.send_now();
  delay(5000);
  
  // exit
  repeat_single_stroke(KEY_TAB, 2, 500);
  Keyboard.set_key1(KEY_RIGHT);
  Keyboard.send_now();
  Keyboard.set_key1(KEY_ENTER);
  Keyboard.send_now();
  
  // wait forever, do not loop
  while(true) { }
  
}
