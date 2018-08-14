#include <Bounce.h>

#define potMIDIChannel 1
#define buttonMIDIChannel 1 
#define potCCIndex 14
#define buttonCCIndex 15
#define potJitterRange 7
#define potPin A0 
#define buttonPin 0
int potValue, ccValue, previousPotValue;

Bounce button = Bounce(0,5);

void setup() {
  
  Serial.begin(115200); // not sure this is the best way to set the baud
  pinMode(buttonPin, INPUT_PULLUP);
  usbMIDI.sendControlChange(44, potMIDIChannel, 2);
}

void loop() {
  
  // button stuff 
  button.update();
  if (button.fallingEdge()) {
      usbMIDI.sendControlChange(buttonCCIndex, 127, buttonMIDIChannel);
      Serial.println("falling edge");
  }
  if (button.risingEdge()) {
      usbMIDI.sendControlChange(buttonCCIndex, 0, buttonMIDIChannel);
      Serial.println("rising edge");
  } 
  
    
  potValue = analogRead(potPin);  
  Serial.print("pot: ");
  Serial.println(potValue);
  if(abs(potValue - previousPotValue) > potJitterRange) {  
    Serial.println("pot changed");
    // calc the CC value based on the raw value
    ccValue = map(potValue, 0, 1023, 0, 127);                                     
    // send the MIDI
    usbMIDI.sendControlChange(potCCIndex, ccValue, potMIDIChannel);                                  
    previousPotValue = potValue;
  }

  delay(5); // slow your roll or shitty instrument might get pissed
}
