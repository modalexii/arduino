#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

Adafruit_NeoPixel strip6 = Adafruit_NeoPixel(81, 6, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip5 = Adafruit_NeoPixel(81, 5, NEO_GRB + NEO_KHZ800);

void setup() {

  Serial.begin(9600);

  randomSeed(A0);
  
  strip6.begin();
  strip5.begin();
  
  strip6.show();
  strip5.show();
  
}
/*
boolean inArray(uint8_t array[], uint8_t element) {
  for (int i = 0; i < sizeof(array); i++) {
    if (array[i] == element) {
      return true;
    }
  }
  return false;
}
*/
int sumArray(uint16_t a[]) {
  
  uint16_t sum = 0;
  
  for (uint16_t i=0; i<sizeof(a); i++) {
    sum = sum + a[i];
  }
  Serial.print("Sum: ");
  Serial.println(sum,DEC);
  return(sum);
  
}

void randomFill(uint8_t wait, uint32_t color) {

  uint16_t pixels[strip5.numPixels()];

  delay(500);
  for(int i=0; i<strip5.numPixels(); i++) {
    pixels[i] = i;
    delay(50);
  }
  
  while( sumArray(pixels) > 0 ) {
    
    uint8_t p = random( 0, strip5.numPixels() );
    if(pixels[p] != 0) {
      strip5.setPixelColor(p, color);
      strip5.show();
      pixels[p] = 0;
      delay(wait);
    }
    
  }
  
}

void randomClear(uint8_t wait) {

  uint16_t pixels[strip5.numPixels()];

  delay(500);
  for(int i=0; i<strip5.numPixels(); i++) {
    pixels[i] = i;
    delay(50);
  }
  
  while( sumArray(pixels) > 0 ) {
    
    uint8_t p = random( 0, strip5.numPixels() );
    if(pixels[p] != 0) {
      strip5.setPixelColor(p, 0);
      strip5.show();
      pixels[p] = 0;
      delay(wait);
    }
    
  }
  
}

void rainbowCycle(uint8_t wait) {
  
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip6.numPixels(); i++) {
      strip6.setPixelColor(i, Wheel(((i * 256 / strip6.numPixels()) + j) & 255));
    }
    strip6.show();
    delay(wait);
  }
  
}

uint32_t Wheel(byte WheelPos) {

  // Input a value 0 to 255 to get a color value
  
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip6.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip6.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip6.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  
}

void loop() {
  
  rainbowCycle(20);
  //randomFill(50, strip5.Color(100,100,100));
  //delay(2000);
  //randomClear(50);
  
}
