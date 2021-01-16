#include <Servo.h>

uint8_t servo_angle = 42;

Servo myservo;

int pos = servo_angle;  
int button = 1;

void setup() {
  myservo.attach(6);  // attaches the servo on pin 9 to the servo object
  myservo.write(servo_angle);
  pinMode(button, INPUT_PULLUP);
}

void loop() {
  if(digitalRead(button) == HIGH) {
    delay(50);
    servo_lock();
    delay(5000);
    servo_unlock();
    delay(1000);
  }
  delay(10);
}

void servo_unlock() {
  for (pos = 0; pos < servo_angle; pos++) {
    myservo.write(pos);
    delay(5);
  }
}

void servo_lock() {
  for (pos = servo_angle; pos > 0; pos--) {
    myservo.write(pos);
    delay(5);
  }
}
