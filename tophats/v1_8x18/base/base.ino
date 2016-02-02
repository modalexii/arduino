#include <OctoWS2811.h>

const int ledsPerStrip = 150*2;

DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];

const int config = WS2811_GRB | WS2811_800kHz;

OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

#define DATA_PIN 6
#define PIXEL_COUNT 144 // only for rainbow cycle mode
#define SIZEOF_ARRAY(x)  (sizeof (x) / sizeof (x[0]) )

#define MATRIX_WIDTH 38
#define MATRIX_HEIGHT 11 // count from 0 (set to 4 if 5 physical rows)
//const int MATRIX_BRIGHTNESS = 65;

void setup(){
  
  leds.begin();
  //leds.setBrightness(analogRead(A6)/4);
  
  Serial.begin(9600);
  
}

int black[8][18] = {
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

int triangleBase[8][18] = {
	{2016,2016,2016,2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016,2016,2016,63519},
	{2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016},
	{2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016,2016},
	{2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016},
	{2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016,2016,2016,63519,2016,2016,2016},
	{2016,63519,63519,63519,2016,2016,2016,63519,63519,63519,2016,2016,2016,63519,63519,63519,2016,2016},
	{2016,63519,63519,63519,2016,2016,2016,63519,63519,63519,2016,2016,2016,63519,63519,63519,2016,2016},
	{63519,63519,63519,63519,63519,2016,63519,63519,63519,63519,63519,2016,63519,63519,63519,63519,63519,2016}
};

int stripeBase1[8][18] = {
	{1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616},
	{63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864},
	{64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303},
	{1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616},
	{63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864},
	{64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303},
	{1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616},
	{63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864,63616,1303,64864}
};

int lineBase[8][18] = {
	{63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000},
	{63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000},
	{63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000},
	{63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000},
	{63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000},
	{63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000},
	{63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000},
	{63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000,63488,00000}
};

int diamonds[8][18] = {
	{58976,58976,58976,58976,58976,413,58976,58976,58976,58976,58976,413,58976,58976,58976,58976,58976,413},
	{413,58976,58976,58976,413,413,413,58976,58976,58976,413,413,413,58976,58976,58976,413,413},
	{413,413,58976,413,413,413,413,413,58976,413,413,413,413,413,58976,413,413,413},
	{413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413},
	{413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413},
	{413,413,58976,413,413,413,413,413,58976,413,413,413,413,413,58976,413,413,413},
	{413,58976,58976,58976,413,413,413,58976,58976,58976,413,413,413,58976,58976,58976,413,413},
	{58976,58976,58976,58976,58976,413,58976,58976,58976,58976,58976,413,58976,58976,58976,58976,58976,413}
};

int narrow[10][38]= {
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000},
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000},
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000},
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000},
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000},
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000},
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000},
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000},
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000},
  {0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000,0xff0000,0xff0000,0xff0000,0xffff00,0xffff00,0xffff00,0x00ff00,0x00ff00,0x00ff00,0x00ffff,0x00ffff,0x00ffff,0x0000ff,0x0000ff,0x0000ff,0x000000,0x000000,0x000000,0x000000}
};


/*
void drawScreen(int pixelArray[][MATRIX_WIDTH]) {

  // given a 2d array representing the display, show it

  for(int x = 0; x <= MATRIX_WIDTH; x++) {
    for(int y = 0; y <= MATRIX_HEIGHT; y++) {
      leds.drawPixel(x,y,pixelArray[y][x]);
    }
  }

  leds.show();

}
*/

void drawScreen(int pixelArray[][MATRIX_WIDTH]) {

  // given a 2d array representing the display, show it

  int pixel = 0;

  for (int row = 0; row <= MATRIX_HEIGHT; row++) {
    for (int col = 0; col <= MATRIX_WIDTH; col++) {
      leds.setPixel(pixel, pixelArray[row][col]);
      pixel++;
    }
  }

  leds.show();

}

void drawScreenSnake(int pixelArray[][MATRIX_WIDTH]) {

  // given a 2d array representing the display, show it

  int pixel = 0;

  for (int row = 0; row <= MATRIX_HEIGHT; row++) {
    if (row % 2 == 0) {
      for (int col = 0; col <= MATRIX_WIDTH-1; col++) {
        Serial.print(row);
        Serial.print(", ");
        Serial.print(col);
        Serial.print(", ");
        Serial.print(pixel);
        Serial.println(" EVEN");
        leds.setPixel(pixel, pixelArray[row][col]);
        pixel++;
      }
    } else {
      for (int col = MATRIX_WIDTH-1; col >= 0; col--) {
        Serial.print(row);
        Serial.print(", ");
        Serial.print(col);
        Serial.print(", ");
        Serial.print(pixel);
        Serial.println(" ODD");
        leds.setPixel(pixel, pixelArray[row][col]);
        pixel++;
      }
    }
  }

  leds.show();

}

void reverse(int a[], int sz) {
  // supports rotate()
  int i, j;
  for (i = 0, j = sz; i < j; i++, j--) {
    int tmp = a[i];
    a[i] = a[j];
    a[j] = tmp;
  }
}

void rotate(int array[], int size, int amt) {
  
  if (amt < 0)
    amt = size + amt;
  reverse(array, size-amt-1);
  reverse(array+size-amt, amt-1);
  reverse(array, size-1);
  
}
/*
uint32_t Wheel(byte WheelPos) {
  
  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
  
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  
}

void rainbowCycle(uint8_t rpt) {

  strip.begin();
  strip.setBrightness(150);
  strip.show();
  
  uint16_t i, j;

  for(j=0; j<256*rpt; j++) {
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(10);
  }
  
}

void marquee(int pixelArray[][MATRIX_WIDTH], int rpt) {
  for(int iter = 0; iter < rpt; iter++) {

    drawScreen(pixelArray);

    delay(200);
    
    for(int row = 0; row <= MATRIX_HEIGHT; row++) {
      rotate(pixelArray[row], 18, 1);
    }
  
  }
}

void vMarquee(int pixelArray[][MATRIX_WIDTH], int rpt) {
   for(int iter = 0; iter < rpt; iter++) {
     
     int arraySize = SIZEOF_ARRAY(pixelArray);
     
     for(int row = 0; row <= arraySize; row++) {
       
       if(row == arraySize - 1) {
         pixelArray[row] = pixelArray[0];
       }
       else {
         pixelArray[row] = pixelArray[row + 1];
       }
       
     }
     
   }
}

void stripeMarquee1(int rpt) {
  for(int iter = 0; iter < rpt; iter++) {
    
    drawScreen(stripeBase1);

    delay(100);
    
    for(int row = 0; row <= MATRIX_HEIGHT; row++) {
      rotate(stripeBase1[row], MATRIX_WIDTH, -1);
    }
    
  }
}

void lineWipe(int rpt) {
  
  drawScreen(black);
  
  for(int iter = 0; iter < rpt; iter++) {
    
    for(int x = 0; x <= MATRIX_WIDTH; x++) {
      for(int y = 0; y <= MATRIX_HEIGHT; y++) {
        leds.drawPixel(x,y,lineBase[y][x]);
      }
      leds.show();
      delay(250);
    }
    
    for(int x = 0; x <= MATRIX_WIDTH; x++) {
      for(int y = 0; y <= MATRIX_HEIGHT; y++) {
        leds.drawPixel(x,y,black[y][x]);
      }
      delay(250);
      leds.show();
    }
   
  }
}

void breathRandomColor(int rpt) {
  for(int iter = 0; iter < rpt; iter++) {
    
    uint16_t r = random(0,255);
    uint16_t g = random(0,255);
    uint16_t b = random(0,255);
    uint16_t c = leds.Color(r,g,b);
    
    int solid[8][18] = {
	{c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c},
	{c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c},
	{c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c},
	{c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c},
	{c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c},
	{c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c},
	{c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c},
	{c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c},
    };

    for(int i = 0; i < analogRead(A6)/4; i++) {
      leds.setBrightness(i);
      drawScreen(solid);
      delay(10);
    }
    
    delay(100);
    
    for(int i = analogRead(A6)/4; i >= 0; i--) {
      leds.setBrightness(i);
      drawScreen(solid);
      delay(10);
    }
    
    delay(500);
    
  }
}
*/
void loop() {

  drawScreenSnake(narrow);
  /*
  
  if(digitalRead(A0) == LOW) {
    marquee(diamonds,12);
  }

  if(digitalRead(A1) == LOW) {
    stripeMarquee1(20);
  }

  if(digitalRead(A2) == LOW) {
    rainbowCycle(5);
  }

  if(digitalRead(A3) == LOW) {
     lineWipe(2);
  }

  if(digitalRead(A4) == LOW) {
     breathRandomColor(5);
  }

  if(digitalRead(A5) == LOW) {
     //
  }

  */

}
