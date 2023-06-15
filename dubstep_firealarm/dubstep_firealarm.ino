//#include <Servo.h>
#include <PWMServo.h>
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "FastLED.h"
#include <arduino-timer.h>
FASTLED_USING_NAMESPACE

#define NUM_STRIPS 1
#define NUM_ANNOUNCIATOR_LEDS 6
#define BRIGHTNESS 250
#define FRAMES_PER_SECOND 60
CRGB leds[NUM_STRIPS][NUM_ANNOUNCIATOR_LEDS];
unsigned long ann_blink_previous_timer = 0;
const unsigned long ann_blink_interval = 1000 * 30;
unsigned long ann_blink_duration = 50;
uint8_t gHue = 0;

bool fire = false;

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))


#define pin_mp3_act_status 11
#define servo_out 9
#define alarm_pull_button 7
#define pin_mp3_play_next 4
#define led_announciator_out 3
#define pin_amp_relay 2


uint8_t servo_angle = 75; // unlocked position, higher value = closer to vertical
PWMServo myservo;
int pos = servo_angle;

Timer<10> timer; 

void setup() {
  Serial.begin(9600);
  Serial.println("~~~~~Dubstep Fire Alarm~~~~~");
  myservo.attach(servo_out);
  Serial.println("Set initial servo angle");
  myservo.write(servo_angle);
  delay(1000); // give cartirdge a moment to get home and depress button
  FastLED.addLeds<NEOPIXEL, led_announciator_out>(leds[0], NUM_ANNOUNCIATOR_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  pinMode(alarm_pull_button, INPUT_PULLUP);
  pinMode(pin_mp3_play_next, OUTPUT);
  digitalWrite(pin_mp3_play_next, HIGH); 
  pinMode(pin_mp3_act_status, INPUT_PULLUP);
  pinMode(pin_amp_relay, OUTPUT);
  digitalWrite(pin_amp_relay, LOW);
  FastLED.clear();
  FastLED.show();
}

bool playing() {
  return !digitalRead(pin_mp3_act_status); // goes low while playing
}

void mp3_play_next() {
  Serial.println("Triggering Audio");
  digitalWrite(pin_mp3_play_next, LOW); 
  // "release" trigger "button" by writing high in a moment
  timer.in(250, [](void*) -> void { return digitalWrite(pin_mp3_play_next, HIGH); });
}

void switch_amp_relay(uint8_t state) {
  Serial.print("Powering amp ");
  Serial.println(state);
  digitalWrite(pin_amp_relay, state);
}

void loop() {
  if (fire) {
    while (playing()) {
      announciator_frame();
      FastLED.show();
      gHue = gHue < 255 ? gHue + 1 : 0;
      FastLED.delay(FRAMES_PER_SECOND);
    }
    Serial.println("Done playing");
    switch_amp_relay(LOW);
    fire = false;
    FastLED.clear();
    FastLED.show();
    delay(1000); // just for effect
    servo_unlock();
    delay(1000); // give cartridge time to return and depress the buttom
    ann_blink_previous_timer = 0;
  }
  else if (digitalRead(alarm_pull_button) == HIGH) { // button is depressed (LOW) in the "unactivated" position
    Serial.println("Pulled!");
    fire = true;
    switch_amp_relay(HIGH); // turn on amp now so it can spend the next few ms becoming operational
    delay(200); // gives time for cartridge to actually get pulled down by user
    servo_lock();
    mp3_play_next();
    delay(500); // time for mp3 player to respond and pull activity pin low
  } else {
    announciator_blink(); 
  }
  timer.tick();
}

void servo_unlock() {
  Serial.println("Unlocking puller");
  for (pos = 20; pos < servo_angle; pos++) {
    myservo.write(pos);
    delay(5);
  }
}

void servo_lock() {
  Serial.println("Locking puller");
  for (pos = servo_angle; pos > 20; pos--) {
    myservo.write(pos);
    delay(5);
  }
}

void announciator_blink() {
  unsigned long now = millis();
  if (now - ann_blink_previous_timer > ann_blink_interval) {
    FastLED.clear();
    FastLED.show();
    ann_blink_previous_timer = now;
  }
  else if (now - ann_blink_previous_timer > ann_blink_interval - ann_blink_duration) {
    //Serial.println("Idle Blink");
    fill_solid(leds[0], NUM_ANNOUNCIATOR_LEDS, CHSV(0, 255, 255));
    FastLED.show();
  }
}

void announciator_frame() {
  uint8_t starthue = beatsin8(30, 0, 255);
  uint8_t endhue = beatsin8(60, 0, 255);
  if (starthue < endhue) {
    fill_gradient(leds[0], NUM_ANNOUNCIATOR_LEDS, CHSV(starthue, 255, 255), CHSV(endhue, 255, 255), FORWARD_HUES);
  } else {
    fill_gradient(leds[0], NUM_ANNOUNCIATOR_LEDS, CHSV(starthue, 255, 255), CHSV(endhue, 255, 255), BACKWARD_HUES);
  }
}
