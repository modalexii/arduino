// Run a program of digital i/o high/low, reading the delay from an amalog pin
// To use a pot, wire ends to VCC & ground, middle to analog read

void setup(){
  pinMode(1, OUTPUT); 
  pinMode(2, OUTPUT); 
  pinMode(3, OUTPUT); 
  // ctd...
}

int getVal() {
 return analogRead(0);
}

void next() {
 int s = getVal();
 delay(s);
}
 
void loop(){
  digitalWrite(1, HIGH); //5V ON
  next();
  digitalWrite(1, LOW); //5V OFF
  next();
  digitalWrite(2, HIGH); //5V ON
  next();
  digitalWrite(2, LOW); //5V OFF
  next();
  // ctd..
}

 
