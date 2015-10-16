#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define SIZEOF_ARRAY(x)  (sizeof (x) / sizeof (x[0]) )

#define DATA_PIN 6
#define PIXEL_COUNT 144 // only for rainbow cycle mode

#define MATRIX_WIDTH 18
#define MATRIX_HEIGHT 8
//const int MATRIX_WIDTH = 18;
//const int MATRIX_HEIGHT = 8;
const int MATRIX_BRIGHTNESS = 65;


Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(
  MATRIX_WIDTH,
  MATRIX_HEIGHT,
  DATA_PIN,
  NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS   + NEO_MATRIX_ZIGZAG,
  NEO_GRB           + NEO_KHZ800 
);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(
  // only for rainbow cycle mode 
  PIXEL_COUNT,
  DATA_PIN,
  NEO_GRB + NEO_KHZ800
);

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

int pumpkin[8][18] = {
	{58976,58976,58976,58976,58976,413,58976,58976,58976,58976,58976,413,58976,58976,58976,58976,58976,413},
	{413,58976,58976,58976,413,413,413,58976,58976,58976,413,413,413,58976,58976,58976,413,413},
	{413,413,58976,413,413,413,413,413,58976,413,413,413,413,413,58976,413,413,413},
	{413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413},
	{413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413,413},
	{413,413,58976,413,413,413,413,413,58976,413,413,413,413,413,58976,413,413,413},
	{413,58976,58976,58976,413,413,413,58976,58976,58976,413,413,413,58976,58976,58976,413,413},
	{58976,58976,58976,58976,58976,413,58976,58976,58976,58976,58976,413,58976,58976,58976,58976,58976,413}
};

void setup(){
  
  matrix.begin();
  
  pinMode(A1, INPUT_PULLUP);
  digitalWrite(A1, HIGH);
  pinMode(A2, INPUT_PULLUP);
  digitalWrite(A2, HIGH);
  pinMode(A3, INPUT_PULLUP);
  digitalWrite(A3, HIGH);
  pinMode(A4, INPUT_PULLUP);
  digitalWrite(A4, HIGH);
  pinMode(A5, INPUT_PULLUP);
  digitalWrite(A5, HIGH);
  
  pinMode(A6, INPUT);
  digitalWrite(A6, LOW);
  
  //Serial.begin(9600);
  
}

void drawScreen(int pixelArray[][MATRIX_WIDTH]) {

  // given a 2d array representing the display, show it

  for(int x = 0; x <= MATRIX_WIDTH; x++) {
    for(int y = 0; y <= MATRIX_HEIGHT; y++) {
      matrix.drawPixel(x,y,pixelArray[y][x]);
    }
  }

  matrix.show();

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
  
  strip.begin(); // only for rainbow cycle mode 
  strip.show(); // only for rainbow cycle mode 
  
  uint16_t i, j;

  for(j = 0; j < 256 * rpt; j++) { 
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( ( (i * 256 / strip.numPixels() ) + j) & 255) );
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
  for(int iter = 0; iter < rpt; iter++) {
    
    for(int x = 0; x <= MATRIX_WIDTH; x++) {
      for(int y = 0; y <= MATRIX_HEIGHT; y++) {
        matrix.drawPixel(x,y,lineBase[y][x]);
      }
      matrix.show();
      delay(250);
    }
    
    for(int x = 0; x <= MATRIX_WIDTH; x++) {
      for(int y = 0; y <= MATRIX_HEIGHT; y++) {
        matrix.drawPixel(x,y,black[y][x]);
      }
      delay(250);
      matrix.show();
    }
   
  }
}

void breathRandomColor(int rpt) {
  for(int iter = 0; iter < rpt; iter++) {
    
    uint16_t r = random(0,255);
    uint16_t g = random(0,255);
    uint16_t b = random(0,255);
    uint16_t c = matrix.Color(r,g,b);
    
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

    for(int i = 0; i < MATRIX_BRIGHTNESS; i++) {
      matrix.setBrightness(i);
      drawScreen(solid);
      delay(10);
    }
    
    delay(100);
    
    for(int i = MATRIX_BRIGHTNESS; i >= 0; i--) {
      matrix.setBrightness(i);
      drawScreen(solid);
      delay(10);
    }
    
    delay(500);
    
  }
}

void loop(){
  //matrix.fillScreen(0);
  
  matrix.setBrightness(analogRead(A6)/4);

  if(digitalRead(A1) == LOW) {
    marquee(pumpkin,6);
  }
  if(digitalRead(A2) == LOW) {
    stripeMarquee1(15);
  }
  if(digitalRead(A3) == LOW) {
    rainbowCycle(5);
  }
  if(digitalRead(A4) == LOW) {
     lineWipe(2);
  }
  if(digitalRead(A5) == LOW) {
     breathRandomColor(5);
  }

}
