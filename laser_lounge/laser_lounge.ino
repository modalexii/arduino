#define minute 60000
#define RL1 7
#define RL2 6
#define RL3 5
#define RL4 4
#define MODE 8

void setup() {                
  // initialize the digital pins as an output.
  pinMode(RL1, OUTPUT);
  pinMode(RL2, OUTPUT);
  pinMode(RL3, OUTPUT);
  pinMode(RL4, OUTPUT);
  pinMode(MODE, INPUT_PULLUP);
  test(); 
}

void test() {
  // if pin 8 is grounded, turn all relays on forever
  if (digitalRead(MODE) == LOW ) {
    digitalWrite(RL1, HIGH);
    digitalWrite(RL2, HIGH);
    digitalWrite(RL3, HIGH);
    digitalWrite(RL4, HIGH);
    while(true) {}
  }
  // else just delay a moment
  delay(1000);
}

void loop() {
  
  unsigned long timer = millis();

  /*
  // turn on #1
  digitalWrite(4, HIGH);

  // wait
  delay(3 * minute);

  // turn on #2
  digitalWrite(5, HIGH);

  // wait
  delay(2 * minute);

  // turn off #1
  digitalWrite(4, LOW);
  */

  // turn on #1
  digitalWrite(RL1, HIGH);

  // wait
  delay(3 * minute);

  // turn on #2
  digitalWrite(RL2, HIGH);

  // wait
  delay(2 * minute);

  // turn off #1
  digitalWrite(RL1, LOW);

  // wait
  delay(3 * minute);

  // turn on #3
  digitalWrite(RL3, HIGH);

  // wait
  delay(2 * minute);

  // turn off #2
  digitalWrite(RL2, LOW);

  // wait
  delay(3 * minute);

  // turn on #1
  digitalWrite(RL1, HIGH);

  // wait
  delay(2 * minute);

  // turn off #3
  digitalWrite(RL3, LOW);

 
}
