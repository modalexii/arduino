// Switches between decoding/rerucing MIDI and playing patterns
// per switch in Digital I/O 8
// Reads pattern speed from 100k Pot in Analog Read 0

void setup(){
  Serial.begin(9600);
  
  pinMode(0, OUTPUT);   // lights
  pinMode(1, OUTPUT); 
  pinMode(2, OUTPUT); 
  pinMode(3, OUTPUT); 
  pinMode(4, OUTPUT); 
  pinMode(5, OUTPUT); 
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, INPUT_PULLUP);   // mode switch
  pinMode(9, INPUT_PULLUP);   // pattern select 1
  pinMode(10, INPUT_PULLUP);  // pattern select 2
      
  usbMIDI.setHandleNoteOn(OnNoteOn); // MIDI NoteOn event handler
  usbMIDI.setHandleNoteOff(OnNoteOff); // MIDI NoteOff event handler
}
 
void OnNoteOn(byte channel, byte note, byte velocity){
  int reducedNote = note % 8;
  digitalWrite(reducedNote, HIGH); // 5v on corresponding pin
}
 
void OnNoteOff(byte channel, byte note, byte velocity){
  int reducedNote = note % 8;
  digitalWrite(reducedNote, LOW); // 5v off corresponding pin
}

int getVal() {
 return analogRead(0);
}

void next() {
  // read delay from pot, then start delay
 int s = getVal();
 delay(s);
}

void runPatternA(){
  // strobe (all lights)
  
  digitalWrite(0, HIGH);
  digitalWrite(1, HIGH);
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  next();
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  next();
}

void runPatternB(){
  // chase 
  
  digitalWrite(0, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(6, HIGH);
  next();
  digitalWrite(0, LOW);
  digitalWrite(3, LOW);
  digitalWrite(6, LOW);
  digitalWrite(1, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(7, HIGH);
  next();
  digitalWrite(1, LOW);
  digitalWrite(4, LOW);
  digitalWrite(7, LOW);
  digitalWrite(2, HIGH);
  digitalWrite(5, HIGH);
  next();
  digitalWrite(2, LOW);
  digitalWrite(5, LOW);
  digitalWrite(3, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(0, HIGH);
  next();
  digitalWrite(3, LOW);
  digitalWrite(6, LOW);
  digitalWrite(0, LOW);
  digitalWrite(4, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(1, HIGH);
  next();
  digitalWrite(4, LOW);
  digitalWrite(7, LOW);
  digitalWrite(1, LOW);
  digitalWrite(5, HIGH);
  digitalWrite(2, HIGH);
  next();
  digitalWrite(5, LOW);
  digitalWrite(2, LOW);
  digitalWrite(6, HIGH);
  digitalWrite(0, HIGH);
  digitalWrite(3, HIGH);
  next();
  digitalWrite(6, LOW);
  digitalWrite(0, LOW);
  digitalWrite(3, LOW);
  digitalWrite(7, HIGH);
  digitalWrite(1, HIGH);
  digitalWrite(4, HIGH);
  next();
  digitalWrite(7, LOW);
  digitalWrite(1, LOW);
  digitalWrite(4, LOW);
}

void runPatternC(){
  // alternate
  digitalWrite(0, HIGH);
  digitalWrite(2, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(6, HIGH);
  next();
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);
  digitalWrite(4, LOW);
  digitalWrite(6, LOW);
  digitalWrite(1, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(7, HIGH);
  next();
  digitalWrite(1, LOW);
  digitalWrite(3, LOW);
  digitalWrite(5, LOW);
  digitalWrite(7, LOW);
}

void runPatternD(){
  // circle
  digitalWrite(0, HIGH);
  next();
  digitalWrite(0, LOW);
  digitalWrite(1, HIGH);
  next();
  digitalWrite(1, LOW);
  digitalWrite(2, HIGH);
  next();
  digitalWrite(2, LOW);
  digitalWrite(3, HIGH);
  next();
  digitalWrite(3, LOW);
  digitalWrite(4, HIGH);
  next();
  digitalWrite(4, LOW);
  digitalWrite(5, HIGH);
  next();
  digitalWrite(5, LOW);
  digitalWrite(6, HIGH);
  next();
  digitalWrite(6, LOW);
  digitalWrite(7, HIGH);
  next();
  digitalWrite(7, LOW);
}

void runPatternError(){
  Serial.println("Launch Pattern ERROR");
  digitalWrite(6, HIGH);
  delay(200);
  digitalWrite(6, LOW);
  delay(200);
  digitalWrite(6, HIGH);
  delay(200);
  digitalWrite(6, LOW);
}

void selectAndRunPattern(){
  // read switch positions to select pattern
  Serial.println("New Pattern Poll");
  
  boolean patternSelect1 = digitalRead(9);
  boolean patternSelect2 = digitalRead(10);
  
   Serial.println(patternSelect1);
   Serial.println(patternSelect2);
  
  if (patternSelect1 && patternSelect2){
    runPatternA();
  } else if (patternSelect1 && !(patternSelect2)){
    runPatternB();
  } else if (!(patternSelect1) && patternSelect2){
    runPatternC();
  } else if (!(patternSelect1) && !(patternSelect2)){
    runPatternD();
  } else {
    runPatternError();
  }
}

void loop(){
  // If switch is closed, go in to MIDI mode, else go in to pattern mode
  if (digitalRead(8)){
    Serial.println("Enter MIDI Mode");
    usbMIDI.read();
  } else {
    Serial.println("Enter PATTERN Mode");
    selectAndRunPattern();
  }
}
