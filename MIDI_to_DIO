// Receive MIDI, trigger digital i/o 

void setup(){
  pinMode(1, OUTPUT); 
  pinMode(2, OUTPUT); 
  pinMode(3, OUTPUT); 
  pinMode(4, OUTPUT); 
  pinMode(5, OUTPUT); 
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
      
  usbMIDI.setHandleNoteOn(OnNoteOn); //Specify NoteOn handler
  usbMIDI.setHandleNoteOff(OnNoteOff); //Specify NoteOff handler
}
 
void loop(){
  usbMIDI.read(); //Start listen for MIDI events from USB
}
 
void OnNoteOn(byte channel, byte note, byte velocity){ //This is the function that handles NoteOn events
  if(note == 60){ // Note C3
    digitalWrite(1, HIGH); 
  }
  if(note == 61){ 
    digitalWrite(2, HIGH);
  }
  if(note == 62){ 
    digitalWrite(3, HIGH); 
  }
  if(note == 63){ 
    digitalWrite(4, HIGH); 
  }
  if(note == 64){ 
    digitalWrite(5, HIGH); 
  }
  if(note == 65){ 
    digitalWrite(6, HIGH); 
  }
  if(note == 66){ 
    digitalWrite(7, HIGH); 
  }
  if(note == 67){ 
    digitalWrite(8, HIGH); 
  }
  if(note == 68){ 
    digitalWrite(9, HIGH); 
  }
  if(note == 69){ 
    digitalWrite(10, HIGH); 
  }
  if(note == 70){ 
    digitalWrite(11, HIGH);
  }
  if(note == 71){ 
    digitalWrite(12, HIGH); 
  }
  if(note == 72){ 
    digitalWrite(13, HIGH); 
  }
  if(note == 73){ 
    digitalWrite(14, HIGH); 
  }
}
 
void OnNoteOff(byte channel, byte note, byte velocity){ //This is the function that handles NoteOff events
  if(note == 60){  // Note C3
    digitalWrite(1, LOW); 
  }
  if(note == 61){ 
    digitalWrite(2, LOW); 
  }
  if(note == 62){ 
    digitalWrite(3, LOW); 
  }
  if(note == 63){ 
    digitalWrite(4, LOW);
  }
  if(note == 64){ 
    digitalWrite(5, LOW);
  }
  if(note == 65){ 
    digitalWrite(6, LOW); 
  }
  if(note == 66){ 
    digitalWrite(7, LOW); 
  }
  if(note == 67){ 
    digitalWrite(8, LOW); 
  }
  if(note == 68){ 
    digitalWrite(9, LOW); 
  }
  if(note == 69){ 
    digitalWrite(10, LOW); 
  }
  if(note == 70){ 
    digitalWrite(11, LOW); 
  }
  if(note == 71){ 
    digitalWrite(12, LOW);
  }
  if(note == 72){ 
    digitalWrite(13, LOW); 
  }
  if(note == 73){ 
    digitalWrite(14, LOW); 
  }
}
