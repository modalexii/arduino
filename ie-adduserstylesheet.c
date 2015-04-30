// adds a user stylesheet to IE. 
// as-is, all this stylesheet does is replace list bullets with nyan cat gifs

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

  delay(20000); // wait for computer to init new USB device

  Keyboard.set_modifier(0);
  Keyboard.set_key1(0);
  Keyboard.set_key2(0);
  Keyboard.set_key3(0);
  Keyboard.set_key4(0);
  Keyboard.set_key5(0);
  Keyboard.set_key6(0);
  Keyboard.send_now();
  
  Keyboard.set_modifier(MODIFIERKEY_GUI); // press Windows key
  Keyboard.set_key1(KEY_R);
  Keyboard.send_now();
  Keyboard.set_modifier(0);
  Keyboard.set_key1(0);
  Keyboard.send_now();
  delay(2000); // wait for prompt
  Keyboard.print("powershell"); 
  keystroke(KEY_ENTER, 1, 1000);
  delay(4000); // wait for shell
  keystroke(KEY_ENTER, 2, 1000); // try to init shell
  delay(1000); // omg powershell is so slow

  Keyboard.println("'"); 
  Keyboard.println("/*"); 
  Keyboard.println("This file is part of a prank. If IE is behaving oddly, delete this file and "); 
  Keyboard.println("delete the registry key \"HKCU:\\Software\\Microsoft\\Internet Explorer\\Styles\\\","); 
  Keyboard.println("then stop leaving your computer unlocked ;)"); 
  Keyboard.println("*/"); 
  Keyboard.println("ul li:not(.gsfs) { /* exclude Google suggestions ul - too obvious */"); 
  Keyboard.println("background-image: url(http://fc08.deviantart.net/fs71/f/2012/302/b/0/nyan_cat_by_epicitaly-d5jcozf.gif);"); 
  Keyboard.println("background-repeat: no-repeat;"); 
  Keyboard.println("background-position: 0px center;"); 
  Keyboard.println("padding-left: 45px;"); 
  Keyboard.println("}"); 
  Keyboard.println("' | Set-Content \"$($Env:UserProfile)\\nyanlist.css\""); 
  Keyboard.println("New-Item -Path \"HKCU:\\Software\\Microsoft\\Internet Explorer\\\" -Name \"Styles\" -Force "); 
  Keyboard.println("New-Item -Path \"HKCU:\\Software\\Microsoft\\nternet Explorer\\Styles\\\" -Name \"Use My Stylesheet\" -Force"); 
  Keyboard.println("New-ItemProperty -Path \"HKCU:\\Software\\Microsoft\\Internet Explorer\\Styles\\\" -PropertyType \"Dword\" -Name \"Use My Stylesheet\" -Value 1"); 
  Keyboard.println("New-Item -Path \"HKCU:\\Software\\Microsoft\\Internet Explorer\\Style\\\" -Name \"User Stylesheet\" -Force"); 
  Keyboard.println("New-ItemProperty -Path \"HKCU:\\Software\\Microsoft\\Internet Explorer\\Styles\\\" -PropertyType \"String\" -Name \"User Stylesheet\" -Value \"$($Env:UserProfile)\\nyanlist.css\""); 
  Keyboard.println("exit"); 
  keystroke(KEY_ENTER, 1, 1000); // make sure we exited
  delay(50000);
}

