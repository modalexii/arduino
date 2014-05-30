// More interesting display using modulus 8 to reduce
// the entire scale to 8 DIO triggers


void setup(){
  pinMode(0, OUTPUT); 
  pinMode(1, OUTPUT); 
  pinMode(2, OUTPUT); 
  pinMode(3, OUTPUT); 
  pinMode(4, OUTPUT); 
  pinMode(5, OUTPUT); 
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);

      
  usbMIDI.setHandleNoteOn(OnNoteOn); //Specify which function to handle NoteOn events
  usbMIDI.setHandleNoteOff(OnNoteOff); //Specify which function to handle NoteOff events
}
 
void loop(){
  usbMIDI.read(); //Start listen for MIDI events from USB
}
 
void OnNoteOn(byte channel, byte note, byte velocity){ //This is the function that handles NoteOn events
  int reducedNote = note % 8;
  if(reducedNote == 0){ 
    digitalWrite(0, HIGH); 
  }
  if(reducedNote == 1){ 
    digitalWrite(1, HIGH); 
  }
  if(reducedNote == 2){ 
    digitalWrite(2, HIGH); 
  }
  if(reducedNote == 3){ 
    digitalWrite(3, HIGH); 
  }
  if(reducedNote == 4){ 
    digitalWrite(4, HIGH); 
  }
  if(reducedNote == 5){ 
    digitalWrite(5, HIGH); 
  }
  if(reducedNote == 6){ 
    digitalWrite(6, HIGH); 
  }
  if(reducedNote == 7){ 
    digitalWrite(7, HIGH); 
  }
}
 
void OnNoteOff(byte channel, byte note, byte velocity){ //This is the function that handles NoteOff events
  int reducedNote = note % 8;
  if(reducedNote == 0){ 
    digitalWrite(0, LOW); 
  }
  if(reducedNote == 1){ 
    digitalWrite(1, LOW); 
  }
  if(reducedNote == 2){ 
    digitalWrite(2, LOW); 
  }
  if(reducedNote == 3){ 
    digitalWrite(3, LOW); 
  }
  if(reducedNote == 4){ 
    digitalWrite(4, LOW); 
  }
  if(reducedNote == 5){ 
    digitalWrite(5, LOW); 
  }
  if(reducedNote == 6){ 
    digitalWrite(6, LOW); 
  }
  if(reducedNote == 7){ 
    digitalWrite(7, LOW); 
  }
}

// the following is an alternate solution
// this is more graceful but requires that ping 0-8 are used,
// whereas the above could use any 8 pins, sequential or not.

//void OnNoteOn(byte channel, byte note, byte velocity){ //This is the function that handles NoteOn events
//  int reducedNote = note % 8;
//  digitalWrite(reducedNote, HIGH); 
//}
 
//void OnNoteOff(byte channel, byte note, byte velocity){ //This is the function that handles NoteOff events
//  int reducedNote = note % 8;
//  digitalWrite(reducedNote, LOW); 
//}
