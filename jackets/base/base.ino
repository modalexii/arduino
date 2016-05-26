#include <OctoWS2811.h>

const int CUFF_R[] = {0,13};
const int COLLAR_R[] = {14,26};
const int COLLAR_L[] = {26,38};
const int CUFF_L[] = {39,52};
uint8_t brightness = 50; //0-255

int LEN_CUFF_R = CUFF_R[1] - CUFF_R[0];
const int LEN_CUFF_L = CUFF_L[1] - CUFF_L[0];
const int LEN_R = COLLAR_R[1] - CUFF_R[0];
const int LEN_L = CUFF_L[1] - COLLAR_L[0];

const int totalLEDs = 53;

// These parameters control the fire appearance
unsigned int heat = totalLEDs / 1.5;
unsigned int focus = 15;
unsigned int cool = 500; // higher = redder
// other data for fire
unsigned char canvas[totalLEDs];
extern const unsigned int fireColor[100];

elapsedMillis frameTimer, durationTimer;

DMAMEM int displayMemory[totalLEDs*6];
int drawingMemory[totalLEDs*6];
const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811 leds(totalLEDs, displayMemory, drawingMemory, config);

void setup() {
  leds.begin();
  leds.show();
}

void loop() {
  divergeRainbowCycle(30);
  //redBug(100);
  //fire(5000);
  //twinkle(10000,80);
}

void redBug(uint16_t wait) {
  for(int i=0; i<=LEN_L; i++) {
    fillSection(CUFF_R, getRGBL(77,176,209));
    fillSection(CUFF_L, getRGBL(77,176,209));
    fillSection(COLLAR_R, getRGBL(77,176,209));
    fillSection(COLLAR_L, getRGBL(77,176,209));
    leds.setPixel(COLLAR_R[1]-i,getRGBL(235,0,0));
    leds.setPixel(COLLAR_L[0]+i,getRGBL(235,0,0));
    leds.show();
    delay(wait);
  }
  for(int i=0; i<=LEN_CUFF_L; i++) {
    fillSection(CUFF_R, getRGBL(77,176,209));
    fillSection(CUFF_L, getRGBL(77,176,209));
    leds.setPixel(CUFF_R[1]-i,getRGBL(235,0,0));
    leds.setPixel(CUFF_L[0]+i,getRGBL(235,0,0));
    leds.show();
    delay(wait);
  }
}

void divergeRainbowCycle(uint8_t wait) {
  uint16_t i, j;
  for(j=0; j<256; j++) {
    for(i=CUFF_R[0]; i<= COLLAR_R[1]; i++) {
      leds.setPixel(i, Wheel(((i * 256 / LEN_R) + j) & 255));
    }
    for(i=COLLAR_L[0]; i<= CUFF_L[1]; i++) {
      leds.setPixel(i, Wheel(((LEN_L-i * 256 / LEN_L) + j) & 255));
    }
    leds.show();
    delay(wait);
  }
}

void fire(int duration) {
  // wrap animateFire() insted of doing this dierctly in loop()
  durationTimer = 0;
  while(durationTimer < duration) {
    if (frameTimer >= 100) {
      frameTimer = 0;
      animateFire();
    }
  }
}

void twinkle(int duration, int rate) {
  leds.begin(); // black out
  durationTimer = 0;
  while(durationTimer < duration) {
    if (frameTimer >= rate) {
      frameTimer = 0;
      animateTwinkle();
    }
  }
}

void animateTwinkle() {

  int pxUp, pxDown, luma, colorVal;
  pxUp = random(totalLEDs);
  luma = random(20,255);
  colorVal = getRGBL(luma, luma, luma);
  leds.setPixel(pxUp, colorVal);
  pxDown = random(totalLEDs);
  leds.setPixel(pxDown, 0);
  leds.show();
  
}

void animateFire() {
  /*
   * Adopted for 1-D from OctoWS2811 example
   * https://github.com/PaulStoffregen/OctoWS2811/tree/master/examples/Fire
   */
  unsigned int i, c, n, x, y;

  // Step 1: reset first line
  memset(canvas, 0, totalLEDs);

  // Step 2: draw random heatspots on bottom line
  i = heat;
  if (i > totalLEDs-8) i = totalLEDs-8;
  while (i > 0) {
    x = random(totalLEDs - 2) + 1;
    if (canvas[x] == 0) {
      canvas[x] = 99;
      i--;
    }
  }

  // Step 3: interpolate
  for (x=0; x < totalLEDs; x++) {
    c = canvas[y * totalLEDs + x] * focus;
    n = focus;
    if (x > 0) {
      c = c + canvas[totalLEDs + (x - 1)];
      n = n + 1;
    }
    if (x < totalLEDs-1) {
      c = c + canvas[totalLEDs + (x + 1)];
      n = n + 1;
    }
    c = (c + (n / 2)) / n;
    i = (random(1000) * cool) / 10000;
    if (c > i) {
      c = c - i;
    } else {
      c = 0;
    }
    canvas[y * totalLEDs + x] = c;
  }

  // Step 4: render canvas to LEDs
  for (x=0; x < totalLEDs; x++) {
    c = canvas[x];
    // split colors and recombine with getRGBL() so that brightness is factored in
    int palletColor, dimmedColor, r, g, b;
    palletColor = fireColor[c];
    r = (palletColor >> 16) & 0xFF; 
    g = (palletColor >> 8) & 0xFF; 
    b = palletColor & 0xFF;
    dimmedColor = getRGBL(r,g,b);
    leds.setPixel(x, dimmedColor);
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

void fillSection(const int section[], uint32_t color) {
  for (int i=section[0]; i <= section[1]; i++) {
    leds.setPixel(i,color);
  }
  leds.show();
}

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) { 
    for(i=0; i< totalLEDs; i++) {
      leds.setPixel(i, Wheel(((i * 256 / totalLEDs) + j) & 255));
    }
    leds.show();
    delay(wait);
  }
}

uint32_t getRGBL(uint8_t r, uint8_t g, uint8_t b) {
  r = (r * brightness) >> 8;
  g = (g * brightness) >> 8;
  b = (b * brightness) >> 8;
  return (r << 16) | (g << 8) | b;
}


