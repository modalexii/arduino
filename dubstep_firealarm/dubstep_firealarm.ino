#include <Servo.h>
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "FastLED.h"
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
int fire_timer = 0;
#define fire_timeout_s 5

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define alarm_pull_button 7
#define servo_out 5
#define led_announciator_out 9
#define led_stand_out 3

uint8_t servo_angle = 42;
Servo myservo;
int pos = servo_angle;

SoftwareSerial mp3Link(15, 16); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  Serial.begin(9600);
  mp3Link.begin(9600);
  while(!Serial) {}
  if (!myDFPlayer.begin(mp3Link)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to communicate with MP3 Player"));
    while (true);
  }
  Serial.println(F("MP3 Player Online"));
  myDFPlayer.volume(20);  //Set volume value. From 0 to 30
  myDFPlayer.EQ(DFPLAYER_EQ_BASS);
  FastLED.addLeds<NEOPIXEL, led_announciator_out>(leds[0], NUM_ANNOUNCIATOR_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();
  myservo.attach(servo_out);
  myservo.write(servo_angle);
  pinMode(alarm_pull_button, INPUT_PULLUP);
}

void loop() {
  if (fire) {
    while (fire_timer > 0) {
      announciator_frame();
      FastLED.show();
      gHue = gHue < 255 ? gHue + 1 : 0;
      fire_timer -= 1 * FRAMES_PER_SECOND;
      FastLED.delay(FRAMES_PER_SECOND);
    }
    fire = false;
    myDFPlayer.pause();
    FastLED.clear();
    FastLED.show();
    delay(1000); // just for effect
    servo_unlock();
    ann_blink_previous_timer = 0;
  }
  else if (digitalRead(alarm_pull_button) == HIGH) {
    Serial.println("pulled!");
    fire = true;
    fire_timer = fire_timeout_s * 1000;
    delay(30); // gives time for cartridge to actually get pulled down
    servo_lock();
    myDFPlayer.next();
  }
  announciator_blink();
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

void announciator_blink() {
  unsigned long now = millis();
  if (now - ann_blink_previous_timer > ann_blink_interval) {
    FastLED.clear();
    FastLED.show();
    ann_blink_previous_timer = now;
  }
  else if (now - ann_blink_previous_timer > ann_blink_interval - ann_blink_duration) {
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
