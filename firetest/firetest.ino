// Animated Fire Example - OctoWS2811 Library
//  http://www.pjrc.com/teensy/td_libs_OctoWS2811.html
// 
// Based on the simple algorithm explained here:
//  http://caraesnaur.github.io/fire/
//
// This example code is in the public domain.

#include <OctoWS2811.h>

// The display size and color to use
const unsigned int width = 12+12+24;
uint8_t brightness = 50; //0-255

// These parameters control the fire appearance
// (try controlling these with knobs / analogRead....)
unsigned int heat = width / 1.5;
unsigned int focus = 15;
unsigned int cool = 500; // higher = redder

// Arrays for fire animation
unsigned char canvas[width];
extern const unsigned int fireColor[100];

// OctoWS2811 objects
const int totalLEDs = 47;


DMAMEM int displayMemory[totalLEDs*6];
int drawingMemory[totalLEDs*6];
const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811 leds(totalLEDs, displayMemory, drawingMemory, config);


// Run setup once
void setup() {
  // turn on the display
  leds.begin();
  leds.show();
}

elapsedMillis msec;

// Run repetitively
void loop() {
  if (msec >= 100) {
    msec = 0;
    animateFire();
  }
}


void animateFire() {
  unsigned int i, c, n, x, y;

  // Step 1: reset first line
  memset(canvas, 0, width);

  // Step 2: draw random heatspots on bottom line
  i = heat;
  if (i > width-8) i = width-8;
  while (i > 0) {
    x = random(width - 2) + 1;
    if (canvas[x] == 0) {
      canvas[x] = 99;
      i--;
    }
  }

  // Step 3: interpolate
  for (x=0; x < width; x++) {
    c = canvas[y * width + x] * focus;
    n = focus;
    if (x > 0) {
      c = c + canvas[width + (x - 1)];
      n = n + 1;
    }
    if (x < width-1) {
      c = c + canvas[width + (x + 1)];
      n = n + 1;
    }
    c = (c + (n / 2)) / n;
    i = (random(1000) * cool) / 10000;
    if (c > i) {
      c = c - i;
    } else {
      c = 0;
    }
    canvas[y * width + x] = c;
  }

  // Step 4: render canvas to LEDs
  for (x=0; x < width; x++) {
    c = canvas[x];
    int palletColor, dimmedColor, r, g, b;
    palletColor = fireColor[c];
    g = (palletColor >> 16) & 0xFF; 
    r = (palletColor >> 8) & 0xFF; 
    b = palletColor & 0xFF;
    r = (r * brightness) >> 8;
    g = (g * brightness) >> 8;
    b = (b * brightness) >> 8;
    dimmedColor = (g << 16) | (r << 8) | b;
    leds.setPixel(x, dimmedColor);
  }
  
  leds.show();
}


