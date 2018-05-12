#include <OctoWS2811.h>

#define SIZEOF_ARRAY(x)  (sizeof (x) / sizeof (x[0]) )
#define MATRIX_HEIGHT 8 
#define MATRIX_WIDTH 115*2

const int config = WS2811_GRB | WS2811_800kHz;
const int ledsPerStrip = MATRIX_WIDTH;
int drawingMemory[ledsPerStrip*6];
DMAMEM int displayMemory[ledsPerStrip*6];

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

uint8_t luma = 50; //0-255

void setup() {

  //delay(2000); // uncomment to fix serial<->bootloader handling in VMWare
  //Serial.begin(9600);
  leds.begin();
  leds.show();
  
}

uint32_t getRGBL(uint8_t r, uint8_t g, uint8_t b) {
  r = (r * luma) >> 8;
  g = (g * luma) >> 8;
  b = (b * luma) >> 8;
  return (r << 16) | (g << 8) | b;
}


void fill(int color) {
  for(int pixel=0; pixel<=leds.numPixels(); pixel++) {
    leds.setPixel(pixel,color);
  }
  leds.show();
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return getRGBL(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return getRGBL(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return getRGBL(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbowCycle(uint8_t wait) {

   uint16_t i, j;
   for(j=0; j<256; j++) { // foreach color
      
     for(i=0; i<= leds.numPixels(); i++) { // each pixel
       leds.setPixel(i, Wheel( ( (i * 256 / leds.numPixels() ) + j) & 255) );
     }
     leds.show();
     delay(wait);   
   }
   
}


void loop() {
   rainbowCycle(100);
  /*for(int j=0; j<256; j++) { 
    fill(Wheel(j)); 
    delay(200); 
  }*/
  
}
