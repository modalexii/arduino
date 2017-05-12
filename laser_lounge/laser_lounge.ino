#define minute 60000

void setup() {                
  // initialize the digital pins as an output.
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  test(); 
}

void test() {
  for(int i = 4; i <= 7; i++) {
    digitalWrite(i, HIGH);
    delay(1000);
    digitalWrite(i, LOW);
  }
  for(int i = 4; i <= 7; i++) {
    digitalWrite(i, HIGH);
  }
  delay(1000);
  for(int i = 4; i <= 7; i++) {
    digitalWrite(i, LOW);
  }
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

  // turn on #2
  digitalWrite(5, HIGH);

  // wait
  delay(3 * minute);

  // turn on #3
  digitalWrite(6, HIGH);

  // wait
  delay(2 * minute);

  // turn off #2
  digitalWrite(5, LOW);

  // wait
  delay(3 * minute);

  // turn on #4
  digitalWrite(7, HIGH);

  // wait
  delay(2 * minute);

  // turn off #3
  digitalWrite(6, LOW);

  // wait
  delay(3 * minute);

  // turn on #1
  digitalWrite(4, HIGH);

  // wait
  delay(2 * minute);

  // turn off #4
  digitalWrite(7, LOW);
 
}
