
#include "SevSeg.h"
SevSeg sevseg; 
char charsToDisplay[5] = "----";

void setup() {
  
  Serial.begin(9600); // USB
  Serial1.begin(9600); // Data in from GPS controller
  
  byte numDigits = 4;
  byte digitPins[] = {8,5,11,13};
  byte segmentPins[] = {7,6,10,3,9,4,2};
  bool resistorsOnSegments = true; // 'false' means resistors are on digit pins
  byte hardwareConfig = COMMON_CATHODE; // See README.md for options
  bool updateWithDelays = false; // Default. Recommended
  bool leadingZeros = true; // Use 'true' if you'd like to keep the leading zeros

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments, updateWithDelays, leadingZeros);
  sevseg.setBrightness(100);

}

void loop() {

  if(Serial1.available() > 0) {
    String inString = Serial1.readString();
    Serial.println(inString);
    inString.toCharArray(charsToDisplay, 5);
  }
    
  sevseg.setChars( charsToDisplay );
  sevseg.refreshDisplay(); // Must run repeatedly
  
}

