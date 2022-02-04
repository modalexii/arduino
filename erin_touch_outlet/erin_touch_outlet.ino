#define POT_PIN A0
#define RELAY_PIN 0
#define TOUCH_PIN 1
bool relay_on = false;

void setup() {
  Serial.begin(9600);
  pinMode(POT_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
}

bool amBeingTouched() {
  int val = touchRead(TOUCH_PIN);
  Serial.print("capacitance: ");
  Serial.println(val);
  int touchThreshold = analogRead(POT_PIN);//*1.5+1000;
  Serial.print("touchThreshold: ");
  Serial.println(touchThreshold);
  return (val > touchThreshold) ? true : false;
}

void toggleRelay() {
  if (relay_on) {
    digitalWrite(RELAY_PIN, LOW);
    relay_on = false;
  }
  else {
    digitalWrite(RELAY_PIN, HIGH);
    relay_on = true;
  }
}

void loop() {
  if(amBeingTouched()) {
    toggleRelay();
  }
  delay(200);
}
