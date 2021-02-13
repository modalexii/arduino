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
uint8_t gHue = 0;

bool fire = false;
int fire_timer = 0;
#define fire_timeout_s 5

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define alarm_pull_button 1
#define mp3_pp_button 2
#define mp3_ff_button 3
#define servo_out 5
#define led_announciator_out 6

uint8_t servo_angle = 42;
Servo myservo;
int pos = servo_angle;  

SoftwareSerial mp3Link(15, 16); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  mp3Link.begin(9600);
  Serial.begin(115200);
  if (!myDFPlayer.begin(mp3Link)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to communicate with MP3 Player"));
    while(true);
  }
  Serial.println(F("MP3 Player Online"));
  myDFPlayer.volume(20);  //Set volume value. From 0 to 30
  myDFPlayer.EQ(DFPLAYER_EQ_BASS);
  FastLED.addLeds<NEOPIXEL, led_announciator_out>(leds[0], NUM_ANNOUNCIATOR_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.show();
  myservo.attach(servo_out);
  myservo.write(servo_angle);
  pinMode(alarm_pull_button, INPUT_PULLUP);
  pinMode(mp3_pp_button, OUTPUT);
  pinMode(mp3_ff_button, OUTPUT);
  digitalWrite(mp3_pp_button,HIGH);
  digitalWrite(mp3_ff_button,HIGH);
}

void loop() {
  if(fire) {
    while(fire_timer > 0) {
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
  }
  else if(digitalRead(alarm_pull_button) == HIGH) {
    Serial.println("pulled!");
    fire = true;
    fire_timer = fire_timeout_s * 1000;
    delay(50); // gives time for cartridge to actually get pulled down
    servo_lock();
    myDFPlayer.next();
  }
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

void announciator_frame() {
  uint8_t starthue = beatsin8(30, 0, 255);
  uint8_t endhue = beatsin8(60, 0, 255);
  if (starthue < endhue) {
    fill_gradient(leds[0], NUM_ANNOUNCIATOR_LEDS, CHSV(starthue,255,255), CHSV(endhue,255,255), FORWARD_HUES);
  } else {
    fill_gradient(leds[0], NUM_ANNOUNCIATOR_LEDS, CHSV(starthue,255,255), CHSV(endhue,255,255), BACKWARD_HUES);
  }
} 
