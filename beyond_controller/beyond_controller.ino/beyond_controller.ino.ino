#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define HUEPIN A0
#define RGBLUMA_PIN A1
#define WHITELUMA_PIN A2
#define BREATHRATE_PIN A3
#define CONTROLLOCK_PIN A4
#define LEDPIN0 6
#define NUM_LEDS 60

byte hue, rgb_luma, white_luma;
bool first_loop = true;

Adafruit_NeoPixel leds = Adafruit_NeoPixel(NUM_LEDS, LEDPIN0, NEO_GRBW + NEO_KHZ800);

byte neopix_gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


void setup() {
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT_PULLUP);
  leds.begin();
  leds.show(); // Initialize all pixels to 'off'
  Serial.begin(115200);
}

byte potRead(int pin) {
  int avg = (analogRead(pin) + analogRead(pin)+analogRead(pin)+analogRead(pin))/4;
  byte reduced = map(avg, 0, 1023, 0, 255);
  return max(0, reduced - 5);
}

void loop() {

  bool control_lock = digitalRead(CONTROLLOCK_PIN);
  if (first_loop or !(control_lock)) {
    first_loop = false;
    hue = potRead(HUEPIN);
    rgb_luma = potRead(RGBLUMA_PIN);
    white_luma = potRead(WHITELUMA_PIN);
  }

  fill(WWheel(hue, white_luma, rgb_luma)); 

  //delay(100); 
  
  
  // Some example procedures showing how to display to the pixels:
  /*colorWipe(leds.Color(255, 0, 0), 50); // Red
  colorWipe(leds.Color(0, 255, 0), 50); // Green
  colorWipe(leds.Color(0, 0, 255), 50); // Blue
  colorWipe(leds.Color(0, 0, 0, 255), 50); // White

  whiteOverRainbow(20,75,5);  

  pulseWhite(5); 

  // fullWhite();
  // delay(2000);

  rainbowFade2White(3,3,1);
  */
  

}

uint32_t getRGBWL(uint8_t r, uint8_t g, uint8_t b, uint8_t w, byte rgb_luma) {
  r = (r * rgb_luma) >> 8;
  g = (g * rgb_luma) >> 8;
  b = (b * rgb_luma) >> 8;
  return (w << 24) | (r << 16) | (g << 8) | b;
}

void fill(int color) {
  for(int pixel=0; pixel<=leds.numPixels(); pixel++) {
    leds.setPixelColor(pixel,color);
  }
  leds.show();
}

uint32_t WWheel(byte WheelPos, byte White, byte rgb_luma) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return getRGBWL(255 - WheelPos * 3, 0, WheelPos * 3, White, rgb_luma);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return getRGBWL(0, WheelPos * 3, 255 - WheelPos * 3, White, rgb_luma);
  }
  WheelPos -= 170;
  return getRGBWL(WheelPos * 3, 255 - WheelPos * 3, 0, White, rgb_luma);
}

