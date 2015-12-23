#include <Wire.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

void setup() {
  
  //Serial.begin(9600);

  alpha4.begin(0x70);  // pass in the address
  alpha4.setBrightness(8);
  alpha4.clear();
  alpha4.writeDisplay();

  randomSeed(analogRead(A0));

  prgmIntro();

}

void scrollText(char text[], int rate) {
  
  char displaybuffer[4] = {' ', ' ', ' ', ' '};
  
  for(int c=0; c<strlen(text); c++) {
    
    displaybuffer[0] = displaybuffer[1];
    displaybuffer[1] = displaybuffer[2];
    displaybuffer[2] = displaybuffer[3];
    displaybuffer[3] = text[c];

    // set every digit to the buffer
    alpha4.writeDigitAscii(0, displaybuffer[0]);
    alpha4.writeDigitAscii(1, displaybuffer[1]);
    alpha4.writeDigitAscii(2, displaybuffer[2]);
    alpha4.writeDigitAscii(3, displaybuffer[3]);
  
    alpha4.writeDisplay();
    delay(rate);
    
  }
  
}

void prgmIntro() {

  scrollText("KYLE", 300);
  delay(500);
  alpha4.clear();
  alpha4.writeDisplay();
  delay(200);
  scrollText("<3    ", 300);
  alpha4.clear();
  alpha4.writeDigitAscii(0, 'E');
  alpha4.writeDigitAscii(1, 'R');
  alpha4.writeDigitAscii(2, 'I');
  alpha4.writeDigitAscii(3, 'N');
  alpha4.writeDisplay();
  delay(2000);
  //alpha4.clear();
  //alpha4.writeDisplay();
  //delay(1000);
  
}

void showRandomChars(int rate) {

  // pick how many chars to change
  int randNumChars = rand() % 4;

  for(int i=randNumChars; i>-1; i--) {

    // pick a character at random
    char randomchar = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-*"[random () % 65];

    // pick a digit at random
    // this could result in fewer than randNumChars being changed - that's okay
    int randomdigit = rand() % 4;
  
    // show it
    alpha4.writeDigitAscii(randomdigit, randomchar);
    
  }
  
  alpha4.writeDisplay();
  delay(rate);
  
}

void loop() {

  for(int i=0; i<300; i++) {
    showRandomChars(300);
  }

  // aftr a while, show the nice message again
  prgmIntro();
  
}


